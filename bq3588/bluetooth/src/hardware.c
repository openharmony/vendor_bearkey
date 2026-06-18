/******************************************************************************
 *
 *  Copyright © 2015-2026 Xiamen Bearkey Technology Co., Ltd.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at:
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 ******************************************************************************/

/******************************************************************************
 *
 *  Filename:      hardware.c
 *
 *  Description:   Contains controller-specific functions, like
 *                      firmware patch download
 *                      low power mode operations
 *
 ******************************************************************************/

#define LOG_TAG "bt_hwcfg"

#include <utils/Log.h>
#include <sys/types.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <signal.h>
#include <time.h>
#include <errno.h>
#include <fcntl.h>
#include <dirent.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include "bt_hci_bdroid.h"
#include "bt_vendor_brcm.h"
#include "esco_parameters.h"
#include "userial.h"
#include "userial_vendor.h"
#include "upio.h"

/******************************************************************************
**  Constants & Macros
******************************************************************************/

#ifndef BTHW_DBG
#define BTHW_DBG FALSE
#endif

#define FW_PATCHFILE_EXTENSION ".hcd"
#define FW_PATCHFILE_EXTENSION_LEN 4
#define FW_PATCHFILE_PATH_MAXLEN 248 /* Local_Name length of return of \
                                        HCI_Read_Local_Name */

#define HCI_CMD_MAX_LEN 258

#define HCI_RESET 0x0C03
#define HCI_VSC_WRITE_UART_CLOCK_SETTING 0xFC45
#define HCI_VSC_UPDATE_BAUDRATE 0xFC18
#define HCI_READ_LOCAL_NAME 0x0C14
#define HCI_VSC_DOWNLOAD_MINIDRV 0xFC2E
#define HCI_VSC_WRITE_FIRMWARE 0xFC4C
#define HCI_VSC_WRITE_BD_ADDR 0xFC01
#define HCI_VSC_WRITE_SLEEP_MODE 0xFC27
#define HCI_VSC_WRITE_SCO_PCM_INT_PARAM 0xFC1C
#define HCI_VSC_WRITE_PCM_DATA_FORMAT_PARAM 0xFC1E
#define HCI_VSC_WRITE_I2SPCM_INTERFACE_PARAM 0xFC6D
#define HCI_VSC_ENABLE_WBS 0xFC7E
#define HCI_VSC_LAUNCH_RAM 0xFC4E
#define HCI_READ_LOCAL_BDADDR 0x1009

#define HCI_EVT_CMD_CMPL_STATUS_RET_BYTE 5
#define HCI_EVT_CMD_CMPL_LOCAL_NAME_STRING 6
#define HCI_EVT_CMD_CMPL_LOCAL_BDADDR_ARRAY 6
#define HCI_EVT_CMD_CMPL_OPCODE 3
#define LPM_CMD_PARAM_SIZE 12
#define UPDATE_BAUDRATE_CMD_PARAM_SIZE 6
#define HCI_CMD_PREAMBLE_SIZE 3
#define HCD_REC_PAYLOAD_LEN_BYTE 2
#define LOCAL_NAME_BUFFER_LEN 32
#define LOCAL_BDADDR_PATH_BUFFER_LEN 256
#define BYTE_SHIFT_1  8
#define BYTE_SHIFT_2  16
#define BYTE_SHIFT_3  24
void streamToUint16(uint16_t *u16, uint8_t **p)
{
    size_t offset = 1;
    size_t offset2 = 2;
    do {
        *u16 = ((uint16_t)(*(*(p))) + (((uint16_t)(*((*(p)) + offset))) << BYTE_SHIFT_1));
        (*p) += offset2;
    } while (0);
}

void uint16ToStream(uint8_t **p, uint16_t u16)
{
    do {
        *((*(p))++) = (uint8_t)(u16);
        *((*(p))++) = (uint8_t)((u16) >> BYTE_SHIFT_1);
    } while (0);
}

void uint32ToStream(uint8_t **p, uint32_t u32)
{
    do {
        *((*(p))++) = (uint8_t)(u32);
        *((*(p))++) = (uint8_t)((u32) >> BYTE_SHIFT_1);
        *((*(p))++) = (uint8_t)((u32) >> BYTE_SHIFT_2);
        *((*(p))++) = (uint8_t)((u32) >> BYTE_SHIFT_3);
    } while (0);
}

#define SCO_INTERFACE_PCM 0
#define SCO_INTERFACE_I2S 1

/* one byte is for enable/disable
      next 2 bytes are for codec type */
#define SCO_CODEC_PARAM_SIZE 3

#define BT_VENDOR_CFG_TIMEDELAY_ 40
#define BT_VENDOR_LDM_DEFAULT_IDLE 300
/******************************************************************************
**  Local type definitions
******************************************************************************/

/* Hardware Configuration State */
enum {
    HW_CFG_START = 1,
    HW_CFG_SET_UART_CLOCK,
    HW_CFG_SET_UART_BAUD_1,
    HW_CFG_READ_LOCAL_NAME,
    HW_CFG_DL_MINIDRIVER,
    HW_CFG_DL_FW_PATCH,
    HW_CFG_SET_UART_BAUD_2,
    HW_CFG_SET_BD_ADDR,
    HW_CFG_READ_BD_ADDR
};

/* h/w config control block */
typedef struct {
    uint8_t state;        /* Hardware configuration state */
    int fwFd;            /* FW patch file fd */
    uint8_t f_set_baud_2; /* Baud rate switch state */
    char localChipName[LOCAL_NAME_BUFFER_LEN];
} BtHwCfgCbT;

/* low power mode parameters */
typedef struct {
    uint8_t sleep_mode;                     /* 0(disable),1(UART),9(H5) */
    uint8_t host_stack_idle_threshold;      /* Unit scale 300ms/25ms */
    uint8_t host_controller_idle_threshold; /* Unit scale 300ms/25ms */
    uint8_t bt_wake_polarity;               /* 0=Active Low, 1= Active High */
    uint8_t host_wake_polarity;             /* 0=Active Low, 1= Active High */
    uint8_t allow_host_sleep_during_sco;
    uint8_t combine_sleep_mode_and_lpm;
    uint8_t enable_uart_txd_tri_state; /* UART_TXD Tri-State */
    uint8_t sleep_guard_time;          /* sleep guard time in 12.5ms */
    uint8_t wakeup_guard_time;         /* wakeup guard time in 12.5ms */
    uint8_t txd_config;                /* TXD is high in sleep state */
    uint8_t pulsed_host_wake;          /* pulsed host wake if mode = 1 */
} BtLpmParamT;

/* Firmware re-launch settlement time */
typedef struct {
    const char *chipsetName;
    const uint32_t delay_time;
} FwSettlementEntryT;

#if (FW_AUTO_DETECTION == TRUE)
/* AMPAK FW auto detection table */
typedef struct {
    char *chipId;
    char *updatedChipId;
} FwAutoDetectionEntryT;
#endif

/******************************************************************************
**  Externs
******************************************************************************/

void HwConfigCback(void *pMem);

/******************************************************************************
**  Static variables
******************************************************************************/

static char g_fwPatchfilePath[256] = FW_PATCHFILE_LOCATION;
static char g_fwPatchfileName[128] = {0};
#if (VENDOR_LIB_RUNTIME_TUNING_ENABLED == TRUE)
static int g_fwPatchSettlementDelay = -1;
#endif

static int g_wbsSampleRate = SCO_WBS_SAMPLE_RATE;
static BtHwCfgCbT g_hwCfgCb;

static BtLpmParamT g_lpmParam = {
    LPM_SLEEP_MODE,
    LPM_IDLE_THRESHOLD,
    LPM_HC_IDLE_THRESHOLD,
    LPM_BT_WAKE_POLARITY,
    LPM_HOST_WAKE_POLARITY,
    LPM_ALLOW_HOST_SLEEP_DURING_SCO,
    LPM_COMBINE_SLEEP_MODE_AND_LPM,
    LPM_ENABLE_UART_TXD_TRI_STATE,
    0, /* not applicable */
    0, /* not applicable */
    0, /* not applicable */
    LPM_PULSED_HOST_WAKE
};

/* need to update the bt_sco_i2spcm_param as well
   bt_sco_i2spcm_param will be used for WBS setting
   update the bt_sco_param and bt_sco_i2spcm_param */
static uint8_t bt_sco_param[SCO_PCM_PARAM_SIZE] = {
    SCO_PCM_ROUTING,
    SCO_PCM_IF_CLOCK_RATE,
    SCO_PCM_IF_FRAME_TYPE,
    SCO_PCM_IF_SYNC_MODE,
    SCO_PCM_IF_CLOCK_MODE
};

static uint8_t bt_pcm_data_fmt_param[PCM_DATA_FORMAT_PARAM_SIZE] = {
    PCM_DATA_FMT_SHIFT_MODE,
    PCM_DATA_FMT_FILL_BITS,
    PCM_DATA_FMT_FILL_METHOD,
    PCM_DATA_FMT_FILL_NUM,
    PCM_DATA_FMT_JUSTIFY_MODE
};

static uint8_t bt_sco_i2spcm_param[SCO_I2SPCM_PARAM_SIZE] = {
    SCO_I2SPCM_IF_MODE,
    SCO_I2SPCM_IF_ROLE,
    SCO_I2SPCM_IF_SAMPLE_RATE,
    SCO_I2SPCM_IF_CLOCK_RATE
};

/*
 * The look-up table of recommended firmware settlement delay (milliseconds) on
 * known chipsets.
 */
static const FwSettlementEntryT FW_SETTLEMENT_TABLE[] = {
    {"BCM43241", 200},
    {"BCM43341", 100},
    {(const char *)NULL, 200} // Giving the generic fw settlement delay setting.
};

/*
 * NOTICE:
 *     If the platform plans to run I2S interface bus over I2S/PCM port of the
 *     BT Controller with the Host AP, explicitly set "SCO_USE_I2S_INTERFACE = TRUE"
 *     in the correspodning include/vnd_<target>.txt file.
 *     Otherwise, leave SCO_USE_I2S_INTERFACE undefined in the vnd_<target>.txt file.
 *     And, PCM interface will be set as the default bus format running over I2S/PCM
 *     port.
 */
#if (defined(SCO_USE_I2S_INTERFACE) && SCO_USE_I2S_INTERFACE == TRUE)
static uint8_t sco_bus_interface = SCO_INTERFACE_I2S;
#else
static uint8_t sco_bus_interface = SCO_INTERFACE_PCM;
#endif

#define INVALID_SCO_CLOCK_RATE 0xFF
static uint8_t sco_bus_clock_rate = INVALID_SCO_CLOCK_RATE;
static uint8_t sco_bus_wbs_clock_rate = INVALID_SCO_CLOCK_RATE;

#if (FW_AUTO_DETECTION == TRUE)
#define FW_TABLE_VERSION "v1.1 20161117"
static const FwAutoDetectionEntryT FW_AUTO_DETECTION_TABLE[] = {
    {"4343A0", "BCM43438A0"},     // AP6212
    {"BCM43430A1", "BCM43438A1"}, // AP6212A
    {"BCM20702A", "BCM20710A1"},  // AP6210B
    {"BCM4335C0", "BCM4339A0"},   // AP6335
    {"BCM4330B1", "BCM40183B2"},  // AP6330
    {"BCM4324B3", "BCM43241B4"},  // AP62X2
    {"BCM4350C0", "BCM4354A1"},   // AP6354
    {"BCM4354A2", "BCM4356A2"},   // AP6356
    {"BCM4345C0", "BCM4345C0"},   // AP6255
    {"BCM4345C5", "BCM4345C5"},   // AP6256
    {"BCM43430B0", "BCM4343B0"},  // AP6236
    {"BCM4359C0", "BCM4359C0"},   // AP6359
    {"BCM4349B1", "BCM4359B1"},   // AP6359
    {NULL, NULL}
};
#endif

/******************************************************************************
**  Static functions
******************************************************************************/
static void HwScoI2spcmConfig(uint16_t codec);
static void HwScoI2spcmConfigFromCommand(void *pMem, uint16_t codec);

/******************************************************************************
**  Controller Initialization Static Functions
******************************************************************************/

/*******************************************************************************
**
** Function        look_up_fw_settlement_delay
**
** Description     If FW_PATCH_SETTLEMENT_DELAY_MS has not been explicitly
**                 re-defined in the platform specific build-time configuration
**                 file, we will search into the look-up table for a
**                 recommended firmware settlement delay value.
**
**                 Although the settlement time might be also related to board
**                 configurations such as the crystal clocking speed.
**
** Returns         Firmware settlement delay
**
*******************************************************************************/
uint32_t look_up_fw_settlement_delay(void)
{
    uint32_t ret_value;
    FwSettlementEntryT *pEntry;

    if (FW_PATCH_SETTLEMENT_DELAY_MS > 0) {
        ret_value = FW_PATCH_SETTLEMENT_DELAY_MS;
#if (VENDOR_LIB_RUNTIME_TUNING_ENABLED == TRUE)
    } else if (g_fwPatchSettlementDelay >= 0) {
        ret_value = g_fwPatchSettlementDelay;
#endif
    } else {
        pEntry = (FwSettlementEntryT *)FW_SETTLEMENT_TABLE;

        while (pEntry->chipsetName != NULL) {
            if (strstr(g_hwCfgCb.localChipName, pEntry->chipsetName) != NULL) {
                break;
            }

            pEntry++;
        }

        ret_value = pEntry->delay_time;
    }

    HILOGE("Settlement delay -- %d ms", ret_value);

    return (ret_value);
}

/*******************************************************************************
**
** Function        ms_delay
**
** Description     sleep unconditionally for timeout milliseconds
**
** Returns         None
**
*******************************************************************************/
void ms_delay(uint32_t timeout)
{
    struct timespec delay;
    int err;

    if (timeout == 0) {
        return;
    }
    delay.tv_sec = timeout / BT_VENDOR_TIME_RAIDX;
    delay.tv_nsec = BT_VENDOR_TIME_RAIDX * BT_VENDOR_TIME_RAIDX * (timeout % BT_VENDOR_TIME_RAIDX);

    /* [u]sleep can't be used because it uses SIGALRM */
    do {
        err = nanosleep(&delay, &delay);
    } while (err < 0 && errno == EINTR);
}

/*******************************************************************************
**
** Function        LineSpeedToUserialBaud
**
** Description     helper function converts line speed number into USERIAL baud
**                 rate symbol
**
** Returns         unit8_t (USERIAL baud symbol)
**
*******************************************************************************/
uint8_t LineSpeedToUserialBaud(uint32_t line_speed)
{
    uint8_t baud;

    if (line_speed == USERIAL_LINESPEED_4M) {
        baud = USERIAL_BAUD_4M;
    } else if (line_speed == USERIAL_LINESPEED_3M) {
        baud = USERIAL_BAUD_3M;
    } else if (line_speed == USERIAL_LINESPEED_2M) {
        baud = USERIAL_BAUD_2M;
    } else if (line_speed == USERIAL_LINESPEED_1_5M) {
        baud = USERIAL_BAUD_1_5M;
    } else if (line_speed == USERIAL_LINESPEED_1M) {
        baud = USERIAL_BAUD_1M;
    } else if (line_speed == USERIAL_LINESPEED_921600) {
        baud = USERIAL_BAUD_921600;
    } else if (line_speed == USERIAL_LINESPEED_460800) {
        baud = USERIAL_BAUD_460800;
    } else if (line_speed == USERIAL_LINESPEED_230400) {
        baud = USERIAL_BAUD_230400;
    } else if (line_speed == USERIAL_LINESPEED_115200) {
        baud = USERIAL_BAUD_115200;
    } else if (line_speed == USERIAL_LINESPEED_57600) {
        baud = USERIAL_BAUD_57600;
    } else if (line_speed == USERIAL_LINESPEED_19200) {
        baud = USERIAL_BAUD_19200;
    } else if (line_speed == USERIAL_LINESPEED_9600) {
        baud = USERIAL_BAUD_9600;
    } else if (line_speed == USERIAL_LINESPEED_1200) {
        baud = USERIAL_BAUD_1200;
    } else if (line_speed == USERIAL_LINESPEED_600) {
        baud = USERIAL_BAUD_600;
    } else {
        HILOGE("userial vendor: unsupported baud speed %d", line_speed);
        baud = USERIAL_BAUD_115200;
    }

    return baud;
}

/*******************************************************************************
**
** Function         HwStrncmp
**
** Description      Used to compare two strings in caseless
**
** Returns          0: match, otherwise: not match
**
*******************************************************************************/
static int HwStrncmp(const char *pStr1, const char *pStr2, const int len)
{
    int i;
    size_t offset = 1;

    if (!pStr1 || !pStr2) {
        return (1);
    }

    for (i = 0; i < len; i++) {
        if (toupper(pStr1[i]) != toupper(pStr2[i])) {
            return (i + offset);
        }
    }

    return 0;
}

/*******************************************************************************
**
** Function         HwConfigSetBdaddr
**
** Description      Program controller's Bluetooth Device Address
**
** Returns          xmit bytes
**
*******************************************************************************/
static ssize_t HwConfigSetBdaddr(HcBtHdrT *pBuf)
{
    uint8_t retval = FALSE;
    size_t offset = 1;
    uint8_t *p = (uint8_t *)(pBuf + offset);
    int i = BD_ADDR_LEN;

    uint16ToStream(&p, HCI_VSC_WRITE_BD_ADDR);
    *p++ = BD_ADDR_LEN; /* parameter length */
    *p++ = g_vndLocalBdAddr[--i];
    *p++ = g_vndLocalBdAddr[--i];
    *p++ = g_vndLocalBdAddr[--i];
    *p++ = g_vndLocalBdAddr[--i];
    *p++ = g_vndLocalBdAddr[--i];
    *p = g_vndLocalBdAddr[--i];

    pBuf->len = HCI_CMD_PREAMBLE_SIZE + BD_ADDR_LEN;
    g_hwCfgCb.state = HW_CFG_SET_BD_ADDR;

    retval = g_btVendorCbacks->xmitCb(HCI_VSC_WRITE_BD_ADDR, pBuf);

    return (retval);
}

#if (USE_CONTROLLER_BDADDR == TRUE)
/*******************************************************************************
**
** Function         HwConfigReadBdaddr
**
** Description      Read controller's Bluetooth Device Address
**
** Returns          xmit bytes
**
*******************************************************************************/
static ssize_t HwConfigReadBdaddr(HcBtHdrT *pBuf)
{
    uint8_t retval = FALSE;
    size_t offset = 1;
    uint8_t *p = (uint8_t *)(pBuf + offset);

    uint16ToStream(&p, HCI_READ_LOCAL_BDADDR);
    *p = 0; /* parameter length */

    pBuf->len = HCI_CMD_PREAMBLE_SIZE;
    g_hwCfgCb.state = HW_CFG_READ_BD_ADDR;

    retval = g_btVendorCbacks->xmitCb(HCI_READ_LOCAL_BDADDR, pBuf);

    return (retval);
}
#endif // (USE_CONTROLLER_BDADDR == TRUE)

typedef void (*TTimerHandleCback)(union sigval sigvalValue);

static timer_t OsAllocateTimer(TTimerHandleCback timerCallback)
{
    struct sigevent sigev;
    timer_t timerid;

    (void)memset_s(&sigev, sizeof(struct sigevent), 0, sizeof(struct sigevent));
    // Create the POSIX timer to generate signo
    sigev.sigev_notify = SIGEV_THREAD;
    sigev.sigev_notify_function = timerCallback;
    sigev.sigev_value.sival_ptr = &timerid;

    // Create the Timer using timer_create signal

    if (timer_create(CLOCK_REALTIME, &sigev, &timerid) == 0) {
        return timerid;
    } else {
        HILOGE("timer_create error!");
        return (timer_t)-1;
    }
}

int OsFreeTimer(timer_t timerid)
{
    int ret = 0;
    ret = timer_delete(timerid);
    if (ret != 0) {
        HILOGE("timer_delete fail with errno(%d)", errno);
    }

    return ret;
}

static int OsStartTimer(timer_t timerid, int msec, int mode)
{
    struct itimerspec itval;

    itval.it_value.tv_sec = msec / BT_VENDOR_TIME_RAIDX;
    itval.it_value.tv_nsec = (long)(msec % BT_VENDOR_TIME_RAIDX) * (BT_VENDOR_TIME_RAIDX * BT_VENDOR_TIME_RAIDX);

    if (mode == 1) {
        itval.it_interval.tv_sec = itval.it_value.tv_sec;
        itval.it_interval.tv_nsec = itval.it_value.tv_nsec;
    } else {
        itval.it_interval.tv_sec = 0;
        itval.it_interval.tv_nsec = 0;
    }

    // Set the Timer when to expire through timer_settime

    if (timer_settime(timerid, 0, &itval, NULL) != 0) {
        HILOGE("time_settime error!");
        return -1;
    }

    return 0;
}

static timer_t g_localtimer = 0;
static void LocalTimerHandler(union sigval sigevValue)
{
    g_btVendorCbacks->initCb(BTC_OP_RESULT_SUCCESS);
    OsFreeTimer(g_localtimer);
}
static void StartFwcfgCbtimer(void)
{
    if (g_localtimer == 0) {
        g_localtimer = OsAllocateTimer(LocalTimerHandler);
    }
    OsStartTimer(g_localtimer, BT_VENDOR_CFG_TIMEDELAY_, 0);
}

void HwScoConfig(void);

int HwReadLocalName(uint8_t *p, HcBtHdrT *pBuf)
{
    ssize_t xmitBytes = 0;
    char *pName = FW_PATCHFILE_LOCATION "BCM4362A2.hcd";
    if ((g_hwCfgCb.fwFd = open(pName, O_RDONLY)) == -1) {
        HILOGE("vendor lib preload failed to open [%s]", pName);
    } else {
        /* vsc_download_minidriver */
        uint16ToStream(&p, HCI_VSC_DOWNLOAD_MINIDRV);
        *p = 0; /* parameter length */

        pBuf->len = HCI_CMD_PREAMBLE_SIZE;
        g_hwCfgCb.state = HW_CFG_DL_MINIDRIVER;

        xmitBytes = g_btVendorCbacks->xmitCb(HCI_VSC_DOWNLOAD_MINIDRV, pBuf);
    }
    if (xmitBytes <= 0) {
        HILOGE("vendor lib preload failed to locate firmware patch file and set bdaddr");
        xmitBytes = HwConfigSetBdaddr(pBuf);
    }

    return xmitBytes;
}

int HwCfgDlFwPatch(uint8_t *p, HcBtHdrT *pBuf, uint16_t opcode)
{
    int delay = 100;
    ssize_t xmitBytes = 0;
    pBuf->len = read(g_hwCfgCb.fwFd, p, HCI_CMD_PREAMBLE_SIZE);
    if (pBuf->len > 0) {
        if ((pBuf->len < HCI_CMD_PREAMBLE_SIZE) ||
            (opcode == HCI_VSC_LAUNCH_RAM)) {
            HILOGW("firmware patch file might be altered!");
        } else {
            pBuf->len += read(g_hwCfgCb.fwFd, p + HCI_CMD_PREAMBLE_SIZE, *(p + HCD_REC_PAYLOAD_LEN_BYTE));
            streamToUint16(&opcode, &p);
            xmitBytes = g_btVendorCbacks->xmitCb(opcode, pBuf);
            return xmitBytes;
        }
    }

    close(g_hwCfgCb.fwFd);
    g_hwCfgCb.fwFd = -1;

    /* Normally the firmware patch configuration file
        * sets the new starting baud rate at 115200.
        * So, we need update host's baud rate accordingly.
        */
    HILOGI("bt vendor lib: set UART baud 115200");
    UserialVendorSetBaud(USERIAL_BAUD_115200);

    /* Next, we would like to boost baud rate up again
    * to desired working speed.
    */
    g_hwCfgCb.f_set_baud_2 = TRUE;

    /* Check if we need to pause a few hundred milliseconds
    * before sending down any HCI command.
    */
    delay = look_up_fw_settlement_delay();
    HILOGI("Setting fw settlement delay to %d ", delay);
    ms_delay(delay);

    pBuf->len = HCI_CMD_PREAMBLE_SIZE;
    uint16ToStream(&p, HCI_RESET);
    *p = 0; /* parameter length */
    g_hwCfgCb.state = HW_CFG_START;
    xmitBytes = g_btVendorCbacks->xmitCb(HCI_RESET, pBuf);

    return xmitBytes;
}

int HwCfgReadBdAddr(HcBtHdrT *pEvtBuf, HcBtHdrT *pBuf)
{
    char *pTmp;
    ssize_t xmitBytes = 0;
    size_t offset = 1;
    const uint8_t null_bdaddr[BD_ADDR_LEN] = {0, 0, 0, 0, 0, 0};
    pTmp = (char *)(pEvtBuf + offset) + HCI_EVT_CMD_CMPL_LOCAL_BDADDR_ARRAY;
    HILOGI("entering HW_CFG_READ_BD_ADDR");
    if (memcmp(pTmp, null_bdaddr, BD_ADDR_LEN) == 0) {
        HILOGI("entering HW_CFG_READ_BD_ADDR");
        // Controller does not have a valid OTP BDADDR!
        // Set the BTIF initial BDADDR instead.
        if ((xmitBytes = HwConfigSetBdaddr(pBuf)) > 0) {
            return xmitBytes;
        }
    } else {
        int index0 = 0;
        int index1 = 1;
        int index2 = 2;
        int index3 = 3;
        int index4 = 4;
        int index5 = 5;
        HILOGI("Controller OTP bdaddr %02X:%02X:%02X:%02X:%02X:%02X",
               *(pTmp + index5), *(pTmp + index4), *(pTmp + index3),
               *(pTmp + index2), *(pTmp + index1), *pTmp);
    }

    HILOGI("vendor lib fwcfg completed2");
    HwScoConfig();
    StartFwcfgCbtimer();

    g_hwCfgCb.state = 0;

    if (g_hwCfgCb.fwFd != -1) {
        close(g_hwCfgCb.fwFd);
        g_hwCfgCb.fwFd = -1;
    }

    xmitBytes = 1;
    return xmitBytes;
}

int SetUartDefaultClock(uint8_t *p, HcBtHdrT *pBuf)
{
    size_t offset = 1;
    ssize_t xmitBytes = 0;
    /* set UART clock to 48MHz */
    uint16ToStream(&p, HCI_VSC_WRITE_UART_CLOCK_SETTING);
    *p++ = 1; /* parameter length */
    *p = 1;   /* (1,"UART CLOCK 48 MHz")(2,"UART CLOCK 24 MHz") */

    pBuf->len = HCI_CMD_PREAMBLE_SIZE + offset;
    g_hwCfgCb.state = HW_CFG_SET_UART_CLOCK;

    xmitBytes = g_btVendorCbacks->xmitCb(HCI_VSC_WRITE_UART_CLOCK_SETTING, pBuf);

    return xmitBytes;
}

int SetUartClock(uint8_t *p, HcBtHdrT *pBuf)
{
    ssize_t xmitBytes = 0;
    /* set controller's UART baud rate to 3M */
    uint16ToStream(&p, HCI_VSC_UPDATE_BAUDRATE);
    *p++ = UPDATE_BAUDRATE_CMD_PARAM_SIZE; /* parameter length */
    *p++ = 0;                              /* encoded baud rate */
    *p++ = 0;                              /* use encoded form */
    uint32ToStream(&p, UART_TARGET_BAUD_RATE);

    pBuf->len = HCI_CMD_PREAMBLE_SIZE + UPDATE_BAUDRATE_CMD_PARAM_SIZE;
    g_hwCfgCb.state = (g_hwCfgCb.f_set_baud_2) ? HW_CFG_SET_UART_BAUD_2 : HW_CFG_SET_UART_BAUD_1;

    xmitBytes = g_btVendorCbacks->xmitCb(HCI_VSC_UPDATE_BAUDRATE, pBuf);
    return xmitBytes;
}

int HwCfgSetBdAddr()
{
    ssize_t xmitBytes = 0;
    HILOGI("vendor lib fwcfg completed");
    HwScoConfig();
    StartFwcfgCbtimer();

    g_hwCfgCb.state = 0;

    if (g_hwCfgCb.fwFd != -1) {
        close(g_hwCfgCb.fwFd);
        g_hwCfgCb.fwFd = -1;
    }

    xmitBytes = 1;

    return xmitBytes;
}

int HwConfigSet(HcBtHdrT *pEvtBuf, uint8_t *p, HcBtHdrT *pBuf)
{
    int delay50 = 50;
    uint16_t opcode = 0;
    ssize_t xmitBytes = 0;
    switch (g_hwCfgCb.state) {
        case HW_CFG_SET_UART_BAUD_1:
            UserialVendorSetBaud(LineSpeedToUserialBaud(UART_TARGET_BAUD_RATE));
        case HW_CFG_READ_LOCAL_NAME:
            xmitBytes = HwReadLocalName(p, pBuf);
            break;
        case HW_CFG_DL_MINIDRIVER:
            ms_delay(delay50);
            g_hwCfgCb.state = HW_CFG_DL_FW_PATCH;
        case HW_CFG_DL_FW_PATCH:
            xmitBytes = HwCfgDlFwPatch(p, pBuf, opcode);
            break;
        case HW_CFG_START:
            if (UART_TARGET_BAUD_RATE > 3000000) {  /* 3000000 */
                xmitBytes = SetUartDefaultClock(p, pBuf);
                break;
            }
            /* fall through intentionally */
        case HW_CFG_SET_UART_CLOCK:
            xmitBytes = SetUartClock(p, pBuf);
            break;
        case HW_CFG_SET_UART_BAUD_2:
            UserialVendorSetBaud(LineSpeedToUserialBaud(UART_TARGET_BAUD_RATE));
#if (USE_CONTROLLER_BDADDR == TRUE)
            if ((xmitBytes = HwConfigReadBdaddr(pBuf)) > 0) {
                break;
            }
#else
            if ((xmitBytes = HwConfigSetBdaddr(pBuf)) > 0) {
                break;
            }
#endif
            /* fall through intentionally */
        case HW_CFG_SET_BD_ADDR:
            xmitBytes = HwCfgSetBdAddr();
            break;
#if (USE_CONTROLLER_BDADDR == TRUE)
        case HW_CFG_READ_BD_ADDR:
            xmitBytes = HwCfgReadBdAddr(pEvtBuf, pBuf);
            break;
#endif
    }
    return xmitBytes;
}

/*******************************************************************************
**
** Function         HwConfigCback
**
** Description      Callback function for controller configuration
**
** Returns          None
**
*******************************************************************************/
void HwConfigCback(void *pMem)
{
    HcBtHdrT *pEvtBuf = (HcBtHdrT *)pMem;
    uint8_t *p;
    uint8_t status;
    uint16_t opcode = 0;
    HcBtHdrT *pBuf = NULL;
    size_t offset = 1;
    ssize_t xmitBytes = 0;
    int i;

    status = *((uint8_t *)(pEvtBuf + offset) + HCI_EVT_CMD_CMPL_STATUS_RET_BYTE);
    p = (uint8_t *)(pEvtBuf + offset) + HCI_EVT_CMD_CMPL_OPCODE;
    streamToUint16(&opcode, &p);

    /* Ask a new buffer big enough to hold any HCI commands sent in here */
    if ((status == 0) && g_btVendorCbacks) {
        pBuf = (HcBtHdrT *)g_btVendorCbacks->alloc(BT_HC_HDR_SIZE + HCI_CMD_MAX_LEN);
    }
    if (pBuf == NULL) {
        goto ALLOC_FAIL;
    }
    pBuf->event = MSG_STACK_TO_HC_HCI_CMD;
    pBuf->offset = 0;
    pBuf->len = 0;
    pBuf->layerSpecific = 0;

    p = (uint8_t *)(pBuf + offset);
    xmitBytes = HwConfigSet(pEvtBuf, p, pBuf);
    g_btVendorCbacks->dealloc(pBuf);

ALLOC_FAIL:
    if (xmitBytes <= 0) {
        HILOGE("vendor lib fwcfg aborted!!!");
        if (g_btVendorCbacks) {
            g_btVendorCbacks->initCb(BTC_OP_RESULT_FAIL);
        }

        if (g_hwCfgCb.fwFd != -1) {
            close(g_hwCfgCb.fwFd);
            g_hwCfgCb.fwFd = -1;
        }

        g_hwCfgCb.state = 0;
    }
}

/******************************************************************************
**   LPM Static Functions
******************************************************************************/

/*******************************************************************************
**
** Function         HwLpmCtrlCback
**
** Description      Callback function for lpm enable/disable request
**
** Returns          None
**
*******************************************************************************/
void HwLpmCtrlCback(void *pMem)
{
    HcBtHdrT *pEvtBuf = (HcBtHdrT *)pMem;
    BtOpResultT status = BTC_OP_RESULT_FAIL;
    size_t offset = 1;

    if (*((uint8_t *)(pEvtBuf + offset) + HCI_EVT_CMD_CMPL_STATUS_RET_BYTE) == 0) {
        status = BTC_OP_RESULT_SUCCESS;
    }
}

#if (SCO_CFG_INCLUDED == TRUE)
/*****************************************************************************
**   SCO Configuration Static Functions
*****************************************************************************/

static void HwScoI2spcmProcInterfaceParam(void)
{
    BtOpResultT status = BTC_OP_RESULT_FAIL;
    uint8_t ret = FALSE;
    uint8_t *p;
    HcBtHdrT *pBuf = NULL;
    size_t offset = 1;

    /* Ask a new buffer to hold WRITE_SCO_PCM_INT_PARAM command */
    if (g_btVendorCbacks) {
        pBuf = (HcBtHdrT *)g_btVendorCbacks->alloc(BT_HC_HDR_SIZE
            + HCI_CMD_PREAMBLE_SIZE + SCO_PCM_PARAM_SIZE);
    }
    if (pBuf) {
        pBuf->event = MSG_STACK_TO_HC_HCI_CMD;
        pBuf->offset = 0;
        pBuf->layerSpecific = 0;
        pBuf->len = HCI_CMD_PREAMBLE_SIZE + SCO_PCM_PARAM_SIZE;
        p = (uint8_t *)(pBuf + offset);

        /* do we need this VSC for I2S??? */
        uint16ToStream(&p, HCI_VSC_WRITE_SCO_PCM_INT_PARAM);
        *p++ = SCO_PCM_PARAM_SIZE;
        (void)memcpy_s(p, SCO_PCM_PARAM_SIZE, &bt_sco_param, SCO_PCM_PARAM_SIZE);
        ret = g_btVendorCbacks->xmitCb(HCI_VSC_WRITE_SCO_PCM_INT_PARAM, pBuf);
        g_btVendorCbacks->dealloc(pBuf);
        if (ret) {
            return;
        }
    }
    status = BTC_OP_RESULT_FAIL;

    HILOGI("sco I2S/PCM config interface result %d [0-Success, 1-Fail]", status);
}

static void HwScoI2spcmProcIntParam(void)
{
    BtOpResultT status = BTC_OP_RESULT_FAIL;
    uint8_t ret = FALSE;
    uint8_t *p;
    HcBtHdrT *pBuf = NULL;
    size_t offset = 1;
    
    /* Ask a new buffer to hold WRITE_PCM_DATA_FORMAT_PARAM command */
    if (g_btVendorCbacks) {
        pBuf = (HcBtHdrT *)g_btVendorCbacks->alloc(
            BT_HC_HDR_SIZE + HCI_CMD_PREAMBLE_SIZE + PCM_DATA_FORMAT_PARAM_SIZE);
    }
    if (pBuf) {
        pBuf->event = MSG_STACK_TO_HC_HCI_CMD;
        pBuf->offset = 0;
        pBuf->layerSpecific = 0;
        pBuf->len = HCI_CMD_PREAMBLE_SIZE + PCM_DATA_FORMAT_PARAM_SIZE;

        p = (uint8_t *)(pBuf + offset);
        uint16ToStream(&p, HCI_VSC_WRITE_PCM_DATA_FORMAT_PARAM);
        *p++ = PCM_DATA_FORMAT_PARAM_SIZE;
        (void)memcpy_s(p, PCM_DATA_FORMAT_PARAM_SIZE, &bt_pcm_data_fmt_param, PCM_DATA_FORMAT_PARAM_SIZE);

        ret = g_btVendorCbacks->xmitCb(HCI_VSC_WRITE_PCM_DATA_FORMAT_PARAM, pBuf);
        g_btVendorCbacks->dealloc(pBuf);
        if (ret) {
            return;
        }
    }
    status = BTC_OP_RESULT_FAIL;
    
    HILOGI("sco I2S/PCM config int result %d [0-Success, 1-Fail]", status);
}

/*******************************************************************************
**
** Function         HwScoI2spcmCfgCback
**
** Description      Callback function for SCO I2S/PCM configuration request
**
** Returns          None
**
*******************************************************************************/
static void HwScoI2spcmCfgCback(void *pMem)
{
    HcBtHdrT *pEvtBuf = (HcBtHdrT *)pMem;
    uint8_t *p;
    uint16_t opcode = 0;
    HcBtHdrT *pBuf = NULL;
    BtOpResultT status = BTC_OP_RESULT_FAIL;
    size_t offset = 1;

    p = (uint8_t *)(pEvtBuf + offset) + HCI_EVT_CMD_CMPL_OPCODE;
    streamToUint16(&opcode, &p);

    if (*((uint8_t *)(pEvtBuf + offset) + HCI_EVT_CMD_CMPL_STATUS_RET_BYTE) == 0) {
        status = BTC_OP_RESULT_SUCCESS;
    }

    if (status != BTC_OP_RESULT_SUCCESS) {
        return;
    }
    
    if ((opcode == HCI_VSC_WRITE_I2SPCM_INTERFACE_PARAM) &&
        (sco_bus_interface == SCO_INTERFACE_PCM)) {
        HwScoI2spcmProcInterfaceParam();
    } else if ((opcode == HCI_VSC_WRITE_SCO_PCM_INT_PARAM) &&
             (sco_bus_interface == SCO_INTERFACE_PCM)) {
        HwScoI2spcmProcIntParam();
    }
}

/*******************************************************************************
**
** Function         HwSetMsbcCodecCback
**
** Description      Callback function for setting WBS codec
**
** Returns          None
**
*******************************************************************************/
static void HwSetMsbcCodecCback(void *pMem)
{
    /* whenever update the codec enable/disable, need to update I2SPCM */
    HILOGI("SCO I2S interface change the sample rate to 16K");
    HwScoI2spcmConfigFromCommand(pMem, SCO_CODEC_MSBC);
}

/*******************************************************************************
**
** Function         HwSetCvsdCodecCback
**
** Description      Callback function for setting NBS codec
**
** Returns          None
**
*******************************************************************************/
static void HwSetCvsdCodecCback(void *pMem)
{
    /* whenever update the codec enable/disable, need to update I2SPCM */
    HILOGI("SCO I2S interface change the sample rate to 8K");
    HwScoI2spcmConfigFromCommand(pMem, SCO_CODEC_CVSD);
}

#endif // SCO_CFG_INCLUDED

/*****************************************************************************
**   Hardware Configuration Interface Functions
*****************************************************************************/

/*******************************************************************************
**
** Function        HwConfigStart
**
** Description     Kick off controller initialization process
**
** Returns         None
**
*******************************************************************************/
void HwConfigStart(void)
{
    HcBtHdrT *pBuf = NULL;
    uint8_t *p;
    size_t offset = 1;

    g_hwCfgCb.state = 0;
    g_hwCfgCb.fwFd = -1;
    g_hwCfgCb.f_set_baud_2 = FALSE;

    //    Start from sending HCI_RESET

    if (g_btVendorCbacks) {
        pBuf = (HcBtHdrT *)g_btVendorCbacks->alloc(BT_HC_HDR_SIZE + HCI_CMD_PREAMBLE_SIZE);
    }

    if (pBuf) {
        pBuf->event = MSG_STACK_TO_HC_HCI_CMD;
        pBuf->offset = 0;
        pBuf->layerSpecific = 0;
        pBuf->len = HCI_CMD_PREAMBLE_SIZE;

        p = (uint8_t *)(pBuf + offset);
        uint16ToStream(&p, HCI_RESET);
        *p = 0;

        g_hwCfgCb.state = HW_CFG_START;
        g_btVendorCbacks->xmitCb(HCI_RESET, pBuf);
        g_btVendorCbacks->dealloc(pBuf);
    } else {
        if (g_btVendorCbacks) {
            HILOGE("vendor lib fw conf aborted [no buffer]");
            g_btVendorCbacks->initCb(BTC_OP_RESULT_FAIL);
        }
    }
}

/*******************************************************************************
**
** Function        HwLpmEnable
**
** Description     Enalbe/Disable LPM
**
** Returns         TRUE/FALSE
**
*******************************************************************************/
uint8_t HwLpmEnable(uint8_t turn_on)
{
    HILOGE("entering hw_lpm_enable11");
    HcBtHdrT *pBuf = NULL;
    uint8_t *p;
    uint8_t ret = FALSE;
    size_t offset = 1;

    if (g_btVendorCbacks)
        pBuf = (HcBtHdrT *)g_btVendorCbacks->alloc(BT_HC_HDR_SIZE +
                                                     HCI_CMD_PREAMBLE_SIZE +
                                                     LPM_CMD_PARAM_SIZE);

    if (pBuf) {
        pBuf->event = MSG_STACK_TO_HC_HCI_CMD;
        pBuf->offset = 0;
        pBuf->layerSpecific = 0;
        pBuf->len = HCI_CMD_PREAMBLE_SIZE + LPM_CMD_PARAM_SIZE;

        p = (uint8_t *)(pBuf + offset);
        uint16ToStream(&p, HCI_VSC_WRITE_SLEEP_MODE);
        *p++ = LPM_CMD_PARAM_SIZE; /* parameter length */

        if (turn_on) {
            (void)memcpy_s(p, LPM_CMD_PARAM_SIZE, &g_lpmParam, LPM_CMD_PARAM_SIZE);
            UpioSet(UPIO_LPM_MODE, UPIO_ASSERT, 0);
        } else {
            (void)memset_s(p, LPM_CMD_PARAM_SIZE, 0, LPM_CMD_PARAM_SIZE);
            UpioSet(UPIO_LPM_MODE, UPIO_DEASSERT, 0);
        }

        ret = g_btVendorCbacks->xmitCb(HCI_VSC_WRITE_SLEEP_MODE, pBuf);
        g_btVendorCbacks->dealloc(pBuf);
    }

    HILOGE("HwLpmEnable ret:%d", ret);
    return ret;
}

/*******************************************************************************
**
** Function        HwLpmGetIdleTimeout
**
** Description     Calculate idle time based on host stack idle threshold
**
** Returns         idle timeout value
**
*******************************************************************************/
uint32_t HwLpmGetIdleTimeout(void)
{
    uint32_t timeout_ms;

    /* set idle time to be LPM_IDLE_TIMEOUT_MULTIPLE times of
     * host stack idle threshold (in 300ms/25ms)
     */
    timeout_ms = (uint32_t)g_lpmParam.host_stack_idle_threshold * LPM_IDLE_TIMEOUT_MULTIPLE;
    timeout_ms *= BT_VENDOR_LDM_DEFAULT_IDLE;
    return timeout_ms;
}

/*******************************************************************************
**
** Function        HwLpmSetWakeState
**
** Description     Assert/Deassert BT_WAKE
**
** Returns         None
**
*******************************************************************************/
void HwLpmSetWakeState(uint8_t wake_assert)
{
    uint8_t state = (wake_assert) ? UPIO_ASSERT : UPIO_DEASSERT;

    UpioSet(UPIO_BT_WAKE, state, g_lpmParam.bt_wake_polarity);
}

#if (SCO_CFG_INCLUDED == TRUE)
/*******************************************************************************
**
** Function         HwScoConfig
**
** Description      Configure SCO related hardware settings
**
** Returns          None
**
*******************************************************************************/
void HwScoConfig(void)
{
    if (sco_bus_interface == SCO_INTERFACE_I2S) {
        /* 'Enable' I2S mode */
        bt_sco_i2spcm_param[SCO_I2SPCM_PARAM_IF_MODE] = 1;

        /* set nbs clock rate as the value in SCO_I2SPCM_IF_CLOCK_RATE field */
        sco_bus_clock_rate = bt_sco_i2spcm_param[SCO_I2SPCM_PARAM_IF_CLOCK_RATE];
    } else {
        /* 'Disable' I2S mode */
        bt_sco_i2spcm_param[SCO_I2SPCM_PARAM_IF_MODE] = 0;

        /* set nbs clock rate as the value in SCO_PCM_IF_CLOCK_RATE field */
        sco_bus_clock_rate = bt_sco_param[SCO_PCM_PARAM_IF_CLOCK_RATE];

        /* sync up clock mode setting */
        bt_sco_i2spcm_param[SCO_I2SPCM_PARAM_IF_MODE] = bt_sco_param[SCO_PCM_PARAM_IF_CLOCK_MODE];
    }

    if (sco_bus_wbs_clock_rate == INVALID_SCO_CLOCK_RATE) {
        /* set default wbs clock rate */
        sco_bus_wbs_clock_rate = SCO_I2SPCM_IF_CLOCK_RATE4WBS;

        if (sco_bus_wbs_clock_rate < sco_bus_clock_rate) {
            sco_bus_wbs_clock_rate = sco_bus_clock_rate;
        }
    }

    /*
     *  To support I2S/PCM port multiplexing signals for sharing Bluetooth audio
     *  and FM on the same PCM pins, we defer Bluetooth audio (SCO/eSCO)
     *  configuration till SCO/eSCO is being established;
     *  i.e. in HwSetAudioState() call.
     *  When configured as I2S only, Bluetooth audio configuration is executed
     *  immediately with SCO_CODEC_CVSD by default.
     */

    if (sco_bus_interface == SCO_INTERFACE_I2S) {
        HwScoI2spcmConfig(SCO_CODEC_CVSD);
    } else {
        HwScoI2spcmConfig(SCO_CODEC_NONE);
    }
}

static void HwScoI2spcmConfigFromCommand(void *pMem, uint16_t codec)
{
    HcBtHdrT *pEvtBuf = (HcBtHdrT *)pMem;
    size_t offset = 1;
    bool commandSuccess = *((uint8_t *)(pEvtBuf + offset) + HCI_EVT_CMD_CMPL_STATUS_RET_BYTE) == 0;

    if (commandSuccess) {
        HwScoI2spcmConfig(codec);
    }
}

/*******************************************************************************
**
** Function         HwScoI2spcmConfig
**
** Description      Configure SCO over I2S or PCM
**
** Returns          None
**
*******************************************************************************/
static void HwScoI2spcmConfig(uint16_t codec)
{
    HcBtHdrT *pBuf = NULL;
    uint8_t *p;
    uint8_t ret;
    uint16_t cmd_u16 = HCI_CMD_PREAMBLE_SIZE + SCO_I2SPCM_PARAM_SIZE;
    size_t offset = 1;

    if (g_btVendorCbacks) {
        pBuf = (HcBtHdrT *)g_btVendorCbacks->alloc(BT_HC_HDR_SIZE + cmd_u16);
    }

    if (pBuf) {
        pBuf->event = MSG_STACK_TO_HC_HCI_CMD;
        pBuf->offset = 0;
        pBuf->layerSpecific = 0;
        pBuf->len = cmd_u16;

        p = (uint8_t *)(pBuf + offset);

        uint16ToStream(&p, HCI_VSC_WRITE_I2SPCM_INTERFACE_PARAM);
        *p++ = SCO_I2SPCM_PARAM_SIZE;
        if (codec == SCO_CODEC_CVSD) {
            bt_sco_i2spcm_param[SCO_I2SPCM_PARAM_IF_SAMPLE_RATE] = 0; /* SCO_I2SPCM_IF_SAMPLE_RATE  8k */
            bt_sco_i2spcm_param[SCO_I2SPCM_PARAM_IF_CLOCK_RATE] =
                bt_sco_param[SCO_PCM_PARAM_IF_CLOCK_RATE] = sco_bus_clock_rate;
        } else if (codec == SCO_CODEC_MSBC) {
            bt_sco_i2spcm_param[SCO_I2SPCM_PARAM_IF_SAMPLE_RATE] = g_wbsSampleRate; /* SCO_I2SPCM_IF_SAMPLE_RATE 16K */
            bt_sco_i2spcm_param[SCO_I2SPCM_PARAM_IF_CLOCK_RATE] =
                bt_sco_param[SCO_PCM_PARAM_IF_CLOCK_RATE] = sco_bus_wbs_clock_rate;
        } else {
            bt_sco_i2spcm_param[SCO_I2SPCM_PARAM_IF_SAMPLE_RATE] = 0; /* SCO_I2SPCM_IF_SAMPLE_RATE  8k */
            bt_sco_i2spcm_param[SCO_I2SPCM_PARAM_IF_CLOCK_RATE] =
                bt_sco_param[SCO_PCM_PARAM_IF_CLOCK_RATE] = sco_bus_clock_rate;
            HILOGE("wrong codec is use in HwScoI2spcmConfig, goes default NBS");
        }
        (void)memcpy_s(p, SCO_I2SPCM_PARAM_SIZE, &bt_sco_i2spcm_param, SCO_I2SPCM_PARAM_SIZE);
        cmd_u16 = HCI_VSC_WRITE_I2SPCM_INTERFACE_PARAM;
        HILOGI("I2SPCM config {0x%x, 0x%x, 0x%x, 0x%x}",
            bt_sco_i2spcm_param[SCO_I2SPCM_PARAM_IF_MODE], bt_sco_i2spcm_param[SCO_I2SPCM_PARAM_IF_ROLE],
            bt_sco_i2spcm_param[SCO_I2SPCM_PARAM_IF_SAMPLE_RATE],
            bt_sco_i2spcm_param[SCO_I2SPCM_PARAM_IF_CLOCK_RATE]);

        g_btVendorCbacks->xmitCb(cmd_u16, pBuf);
        g_btVendorCbacks->dealloc(pBuf);
    }
}

/*******************************************************************************
**
** Function         HwSetScoCodec
**
** Description      This functgion sends command to the controller to setup
**                              WBS/NBS codec for the upcoming eSCO connection.
**
** Returns          -1 : Failed to send VSC
**                   0 : Success
**
*******************************************************************************/
static int HwSetScoCodec(uint16_t codec)
{
    HcBtHdrT *pBuf = NULL;
    uint8_t *p;
    uint8_t ret;
    int retVal = 0;
    return retVal;
}

/*******************************************************************************
**
** Function         HwSetAudioState
**
** Description      This function configures audio base on provided audio state
**
** Paramters        pointer to audio state structure
**
** Returns          0: ok, -1: error
**
*******************************************************************************/
int HwSetAudioState(BtVendorOpAudioStateT *pState)
{
    int retVal = -1;

    if (!g_btVendorCbacks) {
        return retVal;
    }
    
    retVal = HwSetScoCodec(pState->peerCodec);
    return retVal;
}
#endif
/*******************************************************************************
**
** Function        HwSetPatchFilePath
**
** Description     Set the location of firmware patch file
**
** Returns         0 : Success
**                 Otherwise : Fail
**
*******************************************************************************/
int HwSetPatchFilePath(char *pConfName, char *pConfValue, int param)
{
    if (strcpy_s(g_fwPatchfilePath, sizeof(g_fwPatchfilePath), pConfValue) != 0) {
        return -1;
    }
    return 0;
}

/*******************************************************************************
**
** Function        HwSetPatchFileName
**
** Description     Give the specific firmware patch filename
**
** Returns         0 : Success
**                 Otherwise : Fail
**
*******************************************************************************/
int HwSetPatchFileName(char *pConfName, char *pConfValue, int param)
{
    if (strcpy_s(g_fwPatchfileName, sizeof(g_fwPatchfileName), pConfValue) != 0) {
        return -1;
    }
    return 0;
}

#if (VENDOR_LIB_RUNTIME_TUNING_ENABLED == TRUE)
/*******************************************************************************
**
** Function        HwSetPatchSettlementDelay
**
** Description     Give the specific firmware patch settlement time in milliseconds
**
** Returns         0 : Success
**                 Otherwise : Fail
**
*******************************************************************************/
int HwSetPatchSettlementDelay(char *pConfName, char *pConfValue, int param)
{
    g_fwPatchSettlementDelay = atoi(pConfValue);
    return 0;
}
#endif // VENDOR_LIB_RUNTIME_TUNING_ENABLED

/*****************************************************************************
**   Sample Codes Section
*****************************************************************************/

#if (HW_END_WITH_HCI_RESET == TRUE)
/*******************************************************************************
**
** Function         HwEpilogCback
**
** Description      Callback function for Command Complete Events from HCI
**                  commands sent in epilog process.
**
** Returns          None
**
*******************************************************************************/
void HwEpilogCback(void *pMem)
{
    HcBtHdrT *pEvtBuf = (HcBtHdrT *)pMem;
    uint8_t *p;
    uint8_t status;
    uint16_t opcode = 0;
    size_t offset = 1;

    status = *((uint8_t *)(pEvtBuf + offset) + HCI_EVT_CMD_CMPL_STATUS_RET_BYTE);
    p = (uint8_t *)(pEvtBuf + offset) + HCI_EVT_CMD_CMPL_OPCODE;
    streamToUint16(&opcode, &p);

    HILOGE("%s Opcode:0x%04X Status: %d", __FUNCTION__, opcode, status);
}

/*******************************************************************************
**
** Function         HwEpilogProcess
**
** Description      Sample implementation of epilog process
**
** Returns          None
**
*******************************************************************************/
void HwEpilogProcess(void)
{
    HcBtHdrT *pBuf = NULL;
    uint8_t *p;
    size_t offset = 1;

    HILOGE("HwEpilogProcess");

    /* Sending a HCI_RESET */
    if (g_btVendorCbacks) {
        /* Must allocate command buffer via HC's alloc API */
        pBuf = (HcBtHdrT *)g_btVendorCbacks->alloc(BT_HC_HDR_SIZE + HCI_CMD_PREAMBLE_SIZE);
    }

    if (pBuf) {
        pBuf->event = MSG_STACK_TO_HC_HCI_CMD;
        pBuf->offset = 0;
        pBuf->layerSpecific = 0;
        pBuf->len = HCI_CMD_PREAMBLE_SIZE;

        p = (uint8_t *)(pBuf + offset);
        uint16ToStream(&p, HCI_RESET);
        *p = 0; /* parameter length */

        /* Send command via HC's xmitCb API */
        g_btVendorCbacks->xmitCb(HCI_RESET, pBuf);
        g_btVendorCbacks->dealloc(pBuf);
    } else {
        if (g_btVendorCbacks) {
            HILOGE("vendor lib epilog process aborted [no buffer]");
        }
    }
}
#endif

void HwProcessEvent(HcBtHdrT *pBuf)
{
    uint16_t opcode = 0;
    size_t offset = 1;
    uint8_t *p = (uint8_t *)(pBuf + offset) + HCI_EVT_CMD_CMPL_OPCODE;
    streamToUint16(&opcode, &p);

    HILOGI("%s, opcode:0x%04x", __FUNCTION__, opcode);
    switch (opcode) {
        case HCI_VSC_WRITE_BD_ADDR:
#if (USE_CONTROLLER_BDADDR == TRUE)
        case HCI_READ_LOCAL_BDADDR:
#endif
        case HCI_READ_LOCAL_NAME:
        case HCI_VSC_DOWNLOAD_MINIDRV:
        case HCI_VSC_WRITE_FIRMWARE:
        case HCI_VSC_LAUNCH_RAM:
        case HCI_RESET:
        case HCI_VSC_WRITE_UART_CLOCK_SETTING:
        case HCI_VSC_UPDATE_BAUDRATE:
            HwConfigCback(pBuf);
            break;
        case HCI_VSC_WRITE_SCO_PCM_INT_PARAM:
        case HCI_VSC_WRITE_PCM_DATA_FORMAT_PARAM:
        case HCI_VSC_WRITE_I2SPCM_INTERFACE_PARAM:
            HwScoI2spcmCfgCback(pBuf);
            break;
        case HCI_VSC_WRITE_SLEEP_MODE:
            HwLpmCtrlCback(pBuf);
            break;
        case HCI_VSC_ENABLE_WBS:
            break;
        default:
            break;
    }

    HILOGI("%s, Complete", __FUNCTION__);
}
