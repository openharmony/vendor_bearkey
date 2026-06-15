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
 *  Filename:      upio.c
 *
 *  Description:   Contains I/O functions, like
 *                      rfkill control
 *                      BT_WAKE/HOST_WAKE control
 *
 ******************************************************************************/

#define LOG_TAG "bt_upio"

#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <utils/Log.h>
#include "bt_vendor_brcm.h"
#include "userial_vendor.h"
#include "upio.h"

/******************************************************************************
**  Constants & Macros
******************************************************************************/

#ifndef UPIO_DBG
#define UPIO_DBG FALSE
#endif

/******************************************************************************
**  Local type definitions
******************************************************************************/

#if (BT_WAKE_VIA_PROC == TRUE)

/* proc fs node for enable/disable lpm mode */
#ifndef VENDOR_LPM_PROC_NODE
#define VENDOR_LPM_PROC_NODE "/proc/bluetooth/sleep/lpm"
#endif

/* proc fs node for notifying write request */
#ifndef VENDOR_BTWRITE_PROC_NODE
#define VENDOR_BTWRITE_PROC_NODE "/proc/bluetooth/sleep/btwrite"
#endif

/*
 * Maximum btwrite assertion holding time without consecutive btwrite kicking.
 * This value is correlative(shorter) to the in-working timeout period set in
 * the bluesleep LPM code. The current value used in bluesleep is 10sec.
 */
#ifndef PROC_BTWRITE_TIMER_TIMEOUT_MS
#define PROC_BTWRITE_TIMER_TIMEOUT_MS 8000
#endif

/* lpm proc control block */
typedef struct {
    uint8_t btwriteActive;
    uint8_t timer_created;
    timer_t timerId;
    uint32_t timeout_ms;
} VndLpmProcCbT;

static VndLpmProcCbT g_lpmProcCb;
#endif

/******************************************************************************
**  Static variables
******************************************************************************/

static uint8_t upio_state[UPIO_MAX_COUNT];
static int g_rfkillId = -1;
static int g_btEmulEnable = 0;
static char g_rfkillStatePath[128];

/******************************************************************************
**  Static functions
******************************************************************************/

/* for friendly debugging outpout string */
static char *g_lpmMode[] = {
    "UNKNOWN",
    "disabled",
    "enabled"
};

static char *g_lpmState[] = {
    "UNKNOWN",
    "de-asserted",
    "asserted"
};

/*****************************************************************************
**   Bluetooth On/Off Static Functions
*****************************************************************************/
static int IsEmulatorContext(void)
{
    return 0;
}

static int IsRfkillDisabled(void)
{
    return UPIO_BT_POWER_OFF;
}

static int InitRfkill(void)
{
    char path[64];
    char buf[16];
    int fd;
    int sz;
    int id;

    for (id = 0;; id++) {
        if (snprintf_s(path, sizeof(path), sizeof(path), "/sys/class/rfkill/rfkill%d/type", id) < 0) {
            return -1;
        }
        
        fd = open(path, O_RDONLY);
        if (fd < 0) {
            HILOGE("InitRfkill : open(%s) failed: %s (%d)\n", path, strerror(errno), errno);
            return -1;
        }

        sz = read(fd, &buf, sizeof(buf));
        close(fd);

        if (sz >= (int)strlen("bluetooth") && memcmp(buf, "bluetooth", strlen("bluetooth")) == 0) {
            g_rfkillId = id;
            break;
        }
    }

    (void)sprintf_s(g_rfkillStatePath, sizeof(g_rfkillStatePath), "/sys/class/rfkill/rfkill%d/state", g_rfkillId);
    return 0;
}

/*****************************************************************************
**   LPM Static Functions
*****************************************************************************/

#if (BT_WAKE_VIA_PROC == TRUE)
/*******************************************************************************
**
** Function        ProcBtwriteTimeout
**
** Description     Timeout thread of proc/.../btwrite assertion holding timer
**
** Returns         None
**
*******************************************************************************/
static void ProcBtwriteTimeout(union sigval arg)
{
    HILOGE("..%s..", __FUNCTION__);
    g_lpmProcCb.btwriteActive = FALSE;
    /* drive LPM down; this timer should fire only when BT is awake; */
    UpioSet(UPIO_BT_WAKE, UPIO_DEASSERT, 1);
}

/******************************************************************************
 **
 ** Function      UpioStartStopTimer
 **
 ** Description   Arm user space timer in case lpm is left asserted
 **
 ** Returns       None
 **
 *****************************************************************************/
void UpioStartStopTimer(int action)
{
    struct itimerspec ts;

    if (action == UPIO_ASSERT) {
        g_lpmProcCb.btwriteActive = TRUE;
        if (g_lpmProcCb.timer_created == TRUE) {
            ts.it_value.tv_sec = PROC_BTWRITE_TIMER_TIMEOUT_MS / BT_VENDOR_TIME_RAIDX;
            ts.it_value.tv_nsec = BT_VENDOR_TIME_RAIDX * BT_VENDOR_TIME_RAIDX *
                (PROC_BTWRITE_TIMER_TIMEOUT_MS % BT_VENDOR_TIME_RAIDX);
            ts.it_interval.tv_sec = 0;
            ts.it_interval.tv_nsec = 0;
        }
    } else {
        /* unarm timer if writing 0 to lpm; reduce unnecessary user space wakeup */
        (void)memset_s(&ts, sizeof(ts), 0, sizeof(ts));
    }

    if (timer_settime(g_lpmProcCb.timerId, 0, &ts, 0) == 0) {
        HILOGE("%s : timer_settime success", __FUNCTION__);
    } else {
        HILOGE("%s : timer_settime failed", __FUNCTION__);
    }
}
#endif

/*****************************************************************************
**   UPIO Interface Functions
*****************************************************************************/

/*******************************************************************************
**
** Function        UpioInit
**
** Description     Initialization
**
** Returns         None
**
*******************************************************************************/
void UpioInit(void)
{
    (void)memset_s(upio_state, sizeof(upio_state), UPIO_UNKNOWN, UPIO_MAX_COUNT);
#if (BT_WAKE_VIA_PROC == TRUE)
    (void)memset_s(&g_lpmProcCb, sizeof(VndLpmProcCbT), 0, sizeof(VndLpmProcCbT));
#endif
}

/*******************************************************************************
**
** Function        UpioCleanup
**
** Description     Clean up
**
** Returns         None
**
*******************************************************************************/
void UpioCleanup(void)
{
#if (BT_WAKE_VIA_PROC == TRUE)
    if (g_lpmProcCb.timer_created == TRUE) {
        timer_delete(g_lpmProcCb.timerId);
    }
    g_lpmProcCb.timer_created = FALSE;
#endif
}

/*******************************************************************************
**
** Function        UpioSetBluetoothPower
**
** Description     Interact with low layer driver to set Bluetooth power
**                 on/off.
**
** Returns         0  : SUCCESS or Not-Applicable
**                 <0 : ERROR
**
*******************************************************************************/
int UpioSetBluetoothPower(int on)
{
    int sz;
    int fd = -1;
    int ret = -1;
    char buffer = '0';

    switch (on) {
        case UPIO_BT_POWER_OFF:
            buffer = '0';
            break;
        case UPIO_BT_POWER_ON:
            buffer = '1';
            break;
        default:
            return 0;
    }

    if (IsEmulatorContext()) {
        /* if new value is same as current, return -1 */
        if (g_btEmulEnable == on) {
            return ret;
        }

        HILOGE("set_bluetooth_power [emul] %d", on);
        g_btEmulEnable = on;
        return 0;
    }

    /* check if we have rfkill interface */
    if (IsRfkillDisabled()) {
        return 0;
    }

    if (g_rfkillId == -1) {
        if (InitRfkill()) {
            return ret;
        }
    }

    fd = open(g_rfkillStatePath, O_WRONLY);
    if (fd < 0) {
        HILOGE("set_bluetooth_power:open(%s) for write failed: %s (%d)", g_rfkillStatePath, strerror(errno), errno);
        return ret;
    }

    sz = write(fd, &buffer, 1);
    if (sz < 0) {
        HILOGE("set_bluetooth_power:write(%s) failed: %s (%d)", g_rfkillStatePath, strerror(errno), errno);
    } else {
        ret = 0;
    }

    if (fd >= 0) {
        close(fd);
    }

    HILOGE("UpioSetBluetoothPower ret %d", ret);
    return ret;
}

void UpioSetLpmMode(uint8_t pio, uint8_t action, uint8_t polarity)
{
#if (BT_WAKE_VIA_PROC == TRUE)
    int fd = -1;
    char buffer;
    int ret = 0;
#endif
    if (upio_state[UPIO_LPM_MODE] == action) {
        HILOGE("LPM is %s already", g_lpmMode[action]);
        return;
    }
    upio_state[UPIO_LPM_MODE] = action;
#if (BT_WAKE_VIA_PROC == TRUE)
    fd = open(VENDOR_LPM_PROC_NODE, O_WRONLY);
    if (fd < 0) {
        HILOGE("UpioSet : open(%s) for write failed: %s (%d)", VENDOR_LPM_PROC_NODE, strerror(errno), errno);
        return;
    }
    if (action == UPIO_ASSERT) {
        buffer = '1';
    } else {
        buffer = '0';
        if (g_lpmProcCb.timer_created == TRUE) {
            timer_delete(g_lpmProcCb.timerId);
            g_lpmProcCb.timer_created = FALSE;
        }
    }
    if ((ret = write(fd, &buffer, 1)) < 0) {
        HILOGE("UpioSet : write(%s) failed: %s (%d)", VENDOR_LPM_PROC_NODE, strerror(errno), errno);
    }
#if (PROC_BTWRITE_TIMER_TIMEOUT_MS != 0)
    if (ret >= 0 && action == UPIO_ASSERT && g_lpmProcCb.timer_created == FALSE) {
        int status;
        struct sigevent se;
        se.sigev_notify = SIGEV_THREAD;
        se.sigevValue.sival_ptr = &g_lpmProcCb.timerId;
        se.sigev_notify_function = ProcBtwriteTimeout;
        se.sigev_notify_attributes = NULL;
        status = timer_create(CLOCK_MONOTONIC, &se, &g_lpmProcCb.timerId);
        if (status == 0) {
            g_lpmProcCb.timer_created = TRUE;
        }
    }
#endif
    if (fd >= 0) {
        close(fd);
    }
#endif
}

void UpioSetBtWake(uint8_t pio, uint8_t action, uint8_t polarity)
{
    int ret = 0;
    if (upio_state[UPIO_BT_WAKE] == action) {
        HILOGE("BT_WAKE is %s already", g_lpmState[action]);
#if (BT_WAKE_VIA_PROC == TRUE)
        if (g_lpmProcCb.btwriteActive == TRUE) {
            return;
        }
#else
        return;
#endif
    }
    upio_state[UPIO_BT_WAKE] = action;
#if (BT_WAKE_VIA_USERIAL_IOCTL == TRUE)
    UserialVendorIoctl(((action==UPIO_ASSERT) ? USERIAL_OP_ASSERT_BT_WAKE : USERIAL_OP_DEASSERT_BT_WAKE), NULL);
#elif (BT_WAKE_VIA_PROC == TRUE)
#if (BT_WAKE_VIA_PROC_NOTIFY_DEASSERT == FALSE)
    if (action == UPIO_DEASSERT) {
        return;
    }
#endif
    int fd = open(VENDOR_BTWRITE_PROC_NODE, O_WRONLY);
    if (fd < 0) {
        HILOGE("UpioSet:open(%s) for write failed: %s (%d)", VENDOR_BTWRITE_PROC_NODE, strerror(errno), errno);
        return;
    }
    char buffer = '1';
#if (BT_WAKE_VIA_PROC_NOTIFY_DEASSERT == TRUE)
    if (action == UPIO_DEASSERT) {
        buffer = '0';
    }
#endif
    if ((ret = write(fd, &buffer, 1)) < 0) {
        HILOGE("UpioSet:write(%s) failed: %s (%d)", VENDOR_BTWRITE_PROC_NODE, strerror(errno), errno);
    }
#if (PROC_BTWRITE_TIMER_TIMEOUT_MS != 0)
    if (ret >= 0) {
        UpioStartStopTimer(action);
    }
#endif
#if (BT_WAKE_VIA_PROC_NOTIFY_DEASSERT == TRUE)
    g_lpmProcCb.btwriteActive = TRUE;
#endif
    HILOGE("%s: proc btwrite assertion, buffer: %c, timer_armed %d %d",
           __FUNCTION__, buffer, g_lpmProcCb.btwriteActive, g_lpmProcCb.timer_created);
    if (fd >= 0) {
        close(fd);
    }
#endif
}

/*******************************************************************************
**
** Function        UpioSet
**
** Description     Set i/o based on polarity
**
** Returns         None
**
*******************************************************************************/
void UpioSet(uint8_t pio, uint8_t action, uint8_t polarity)
{
    HILOGE("%s : pio %d action %d, polarity %d", __FUNCTION__, pio, action, polarity);
    switch (pio) {
        case UPIO_LPM_MODE:
            UpioSetLpmMode(pio, action, polarity);
            break;
        case UPIO_BT_WAKE:
            UpioSetBtWake(pio, action, polarity);
            break;
        case UPIO_HOST_WAKE:
            HILOGE("UpioSet: UPIO_HOST_WAKE");
            break;
    }
}
