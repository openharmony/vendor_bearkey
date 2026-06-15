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
 *  Filename:      bt_vendor_brcm.c
 *
 *  Description:   Broadcom vendor specific library implementation
 *
 ******************************************************************************/

#define LOG_TAG "bt_vendor"

#include <utils/Log.h>
#include <string.h>
#include "upio.h"
#include "userial_vendor.h"
#include "bt_vendor_brcm.h"

#ifndef BTVND_DBG
#define BTVND_DBG FALSE
#endif

/******************************************************************************
**  Externs
******************************************************************************/

void HwConfigStart(void);
uint8_t HwLpmEnable(uint8_t turn_on);
uint32_t HwLpmGetIdleTimeout(void);
void HwLpmSetWakeState(uint8_t wake_assert);
#if (SCO_CFG_INCLUDED == TRUE)
void HwScoConfig(void);
#endif
void VndLoadConf(const char *pPath);
#if (HW_END_WITH_HCI_RESET == TRUE)
void HwEpilogProcess(void);
#endif

/******************************************************************************
**  Variables
******************************************************************************/

BtVendorCallbacksT *g_btVendorCbacks = NULL;
uint8_t g_vndLocalBdAddr[BD_ADDR_LEN] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

/******************************************************************************
**  Local type definitions
******************************************************************************/

/******************************************************************************
**  Static Variables
******************************************************************************/

static const TUserialCfg userial_init_cfg = {
    (USERIAL_DATABITS_8 | USERIAL_PARITY_NONE | USERIAL_STOPBITS_1),
    USERIAL_BAUD_115200
};

/******************************************************************************
**  Functions
******************************************************************************/

/*****************************************************************************
**
**   BLUETOOTH VENDOR INTERFACE LIBRARY FUNCTIONS
**
*****************************************************************************/
/** LPM disable/enable request */
typedef enum {
    BT_VND_LPM_DISABLE,
    BT_VND_LPM_ENABLE,
} BtVendorLpmModeT;


static int init(const BtVendorCallbacksT *pCb, unsigned char *localBdaddr)
{
    int index0 = 0;
    int index1 = 1;
    int index2 = 2;
    int index3 = 3;
    int index4 = 4;
    int index5 = 5;
    HILOGI("init, bdaddr:%02x%02x:%02x%02x:%02x%02x", localBdaddr[index0], localBdaddr[index1],
           localBdaddr[index2], localBdaddr[index3], localBdaddr[index4], localBdaddr[index5]);

    if (pCb == NULL) {
        HILOGE("init failed with no user callbacks!");
        return -1;
    }

#if (VENDOR_LIB_RUNTIME_TUNING_ENABLED == TRUE)
    HILOGW("*****************************************************************");
    HILOGW("*****************************************************************");
    HILOGW("** Warning - BT Vendor Lib is loaded in debug tuning mode!");
    HILOGW("**");
    HILOGW("** If this is not intentional, rebuild libbt-vendor.so ");
    HILOGW("** with VENDOR_LIB_RUNTIME_TUNING_ENABLED=FALSE and ");
    HILOGW("** check if any run-time tuning parameters needed to be");
    HILOGW("** carried to the build-time configuration accordingly.");
    HILOGW("*****************************************************************");
    HILOGW("*****************************************************************");
#endif

    UserialVendorInit();
    UpioInit();

    VndLoadConf(VENDOR_LIB_CONF_FILE);

    /* store reference to user callbacks */
    g_btVendorCbacks = (BtVendorCallbacksT *)pCb;

#if (BRCM_A2DP_OFFLOAD == TRUE)
    brcm_vnd_a2dp_init(g_btVendorCbacks);
#endif

    /* This is handed over from the stack */
    return memcpy_s(g_vndLocalBdAddr, BD_ADDR_LEN, localBdaddr, BD_ADDR_LEN);
}

/** Requested operations */
static int op(BtOpcodeT opcode, void *param)
{
    int retval = 0;
    uint32_t *timeout_ms = (uint32_t *)param;
    switch (opcode) {
        case BT_OP_POWER_ON: // BT_VND_OP_POWER_CTRL
            UpioSetBluetoothPower(UPIO_BT_POWER_OFF);
            UpioSetBluetoothPower(UPIO_BT_POWER_ON);
            break;
        case BT_OP_POWER_OFF: // BT_VND_OP_POWER_CTRL
            UpioSetBluetoothPower(UPIO_BT_POWER_OFF);
            HwLpmSetWakeState(false);
            break;
        case BT_OP_HCI_CHANNEL_OPEN: { // BT_VND_OP_USERIAL_OPEN
            int(*fdArray)[] = (int(*)[])param;
            int fd = UserialVendorOpen((TUserialCfg *)&userial_init_cfg);
            if (fd != -1) {
                for (int idx = 0; idx < HCI_MAX_CHANNEL; idx++) {
                    (*fdArray)[idx] = fd;
                }
                retval = 1;
            }
            break;
        }
        case BT_OP_HCI_CHANNEL_CLOSE: // BT_VND_OP_USERIAL_CLOSE
            UserialVendorClose();
            break;
        case BT_OP_INIT: // BT_VND_OP_FW_CFG
            HwConfigStart();
            break;
        case BT_OP_GET_LPM_TIMER: // BT_VND_OP_GET_LPM_IDLE_TIMEOUT
            *timeout_ms = HwLpmGetIdleTimeout();
            break;
        case BT_OP_LPM_ENABLE:
            retval = HwLpmEnable(BT_VND_LPM_ENABLE);
            break;
        case BT_OP_LPM_DISABLE:
            retval = HwLpmEnable(BT_VND_LPM_DISABLE);
            break;
        case BT_OP_WAKEUP_LOCK:
            HwLpmSetWakeState(TRUE);
            break;
        case BT_OP_WAKEUP_UNLOCK:
            HwLpmSetWakeState(FALSE);
            break;
        case BT_OP_EVENT_CALLBACK:
            HwProcessEvent((HcBtHdrT *)param);
            break;
    }
    return retval;
}

/** Closes the interface */
static void Cleanup(void)
{
    HILOGE("Cleanup");
    UpioCleanup();
    g_btVendorCbacks = NULL;
}

// Entry point of DLib
const BtVendorInterfaceT BLUETOOTH_VENDOR_LIB_INTERFACE = {
    sizeof(BtVendorInterfaceT),
    init,
    op,
    Cleanup
};
