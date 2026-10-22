/******************************************************************************
 *
 *  Copyright (C) 2009-2012 Broadcom Corporation
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
 *  Filename:      userial_vendor.c
 *
 *  Description:   Contains vendor-specific userial functions
 *
 ******************************************************************************/

#define LOG_TAG "bt_userial_vendor"

#include <termios.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <utils/Log.h>
#include "bt_vendor_brcm.h"
#include "userial.h"
#include "userial_vendor.h"

/******************************************************************************
**  Constants & Macros
******************************************************************************/

#ifndef VNDUSERIAL_DBG
#define VNDUSERIAL_DBG FALSE
#endif


#define VND_PORT_NAME_MAXLEN 256

/******************************************************************************
**  Local type definitions
******************************************************************************/

/* vendor serial control block */
typedef struct {
    int fd;                 /* fd to Bluetooth device */
    struct termios termios; /* serial terminal of BT port */
    char portName[VND_PORT_NAME_MAXLEN];
} VndUserialCbT;

/******************************************************************************
**  Static variables
******************************************************************************/

static VndUserialCbT g_vndUserial;

/*****************************************************************************
**   Helper Functions
*****************************************************************************/

/*******************************************************************************
**
** Function        UserialToTcioBaud
**
** Description     helper function converts USERIAL baud rates into TCIO
**                  conforming baud rates
**
** Returns         TRUE/FALSE
**
*******************************************************************************/
uint8_t UserialToTcioBaud(uint8_t cfg_baud, uint32_t *baud)
{
    if (cfg_baud == USERIAL_BAUD_115200) {
        *baud = B115200;
    } else if (cfg_baud == USERIAL_BAUD_4M) {
        *baud = B4000000;
    } else if (cfg_baud == USERIAL_BAUD_3M) {
        *baud = B3000000;
    } else if (cfg_baud == USERIAL_BAUD_2M) {
        *baud = B2000000;
    } else if (cfg_baud == USERIAL_BAUD_1_5M) {
        *baud = B1500000;
    } else if (cfg_baud == USERIAL_BAUD_1M) {
        *baud = B1000000;
    } else if (cfg_baud == USERIAL_BAUD_921600) {
        *baud = B921600;
    } else if (cfg_baud == USERIAL_BAUD_460800) {
        *baud = B460800;
    } else if (cfg_baud == USERIAL_BAUD_230400) {
        *baud = B230400;
    } else if (cfg_baud == USERIAL_BAUD_57600) {
        *baud = B57600;
    } else if (cfg_baud == USERIAL_BAUD_19200) {
        *baud = B19200;
    } else if (cfg_baud == USERIAL_BAUD_9600) {
        *baud = B9600;
    } else if (cfg_baud == USERIAL_BAUD_1200) {
        *baud = B1200;
    } else if (cfg_baud == USERIAL_BAUD_600) {
        *baud = B600;
    } else {
        HILOGE("userial vendor open: unsupported baud idx %i", cfg_baud);
        *baud = B115200;
        return FALSE;
    }

    return TRUE;
}

#if (BT_WAKE_VIA_USERIAL_IOCTL == TRUE)
/*******************************************************************************
**
** Function        UserialIoctlInitBtWake
**
** Description     helper function to set the open state of the bt_wake if ioctl
**                  is used. it should not hurt in the rfkill case but it might
**                  be better to compile it out.
**
** Returns         none
**
*******************************************************************************/
void UserialIoctlInitBtWake(int fd)
{
    uint32_t btWakeState;

#if (BT_WAKE_USERIAL_LDISC == TRUE)
    int ldisc = N_BRCM_HCI; /* brcm sleep mode support line discipline */

    /* attempt to load enable discipline driver */
    if (ioctl(g_vndUserial.fd, TIOCSETD, &ldisc) < 0) {
        HILOGE("USERIAL_Open():fd %d, TIOCSETD failed: error %d for ldisc: %d", fd, errno, ldisc);
    }
#endif

    /* assert BT_WAKE through ioctl */
    ioctl(fd, USERIAL_IOCTL_BT_WAKE_ASSERT, NULL);
    ioctl(fd, USERIAL_IOCTL_BT_WAKE_GET_ST, &btWakeState);
    HILOGE("UserialIoctlInitBtWake read back BT_WAKE state=%i", btWakeState);
}
#endif // (BT_WAKE_VIA_USERIAL_IOCTL==TRUE)

/*****************************************************************************
**   Userial Vendor API Functions
*****************************************************************************/

/*******************************************************************************
**
** Function        UserialVendorInit
**
** Description     Initialize userial vendor-specific control block
**
** Returns         None
**
*******************************************************************************/
void UserialVendorInit(void)
{
    g_vndUserial.fd = -1;
    (void)snprintf_s(g_vndUserial.portName, VND_PORT_NAME_MAXLEN, VND_PORT_NAME_MAXLEN, "%s",
                     BLUETOOTH_UART_DEVICE_PORT);
}

/*******************************************************************************
**
** Function        UserialVendorOpen
**
** Description     Open the serial port with the given configuration
**
** Returns         device fd
**
*******************************************************************************/
int UserialVendorOpen(TUserialCfg *pCfg)
{
    uint32_t baud;
    uint8_t dataBits;
    uint16_t parity;
    uint8_t stopBits;
    g_vndUserial.fd = -1;
    if (!UserialToTcioBaud(pCfg->baud, &baud)) {
        return -1;
    }
    if (pCfg->fmt & USERIAL_DATABITS_8) {
        dataBits = CS8;
    } else if (pCfg->fmt & USERIAL_DATABITS_7) {
        dataBits = CS7;
    } else if (pCfg->fmt & USERIAL_DATABITS_6) {
        dataBits = CS6;
    } else if (pCfg->fmt & USERIAL_DATABITS_5) {
        dataBits = CS5;
    }
    if (pCfg->fmt & USERIAL_PARITY_NONE) {
        parity = 0;
    } else if (pCfg->fmt & USERIAL_PARITY_EVEN) {
        parity = PARENB;
    } else if (pCfg->fmt & USERIAL_PARITY_ODD) {
        parity = (PARENB | PARODD);
    }
    if (pCfg->fmt & USERIAL_STOPBITS_1) {
        stopBits = 0;
    } else if (pCfg->fmt & USERIAL_STOPBITS_2) {
        stopBits = CSTOPB;
    }
    if ((g_vndUserial.fd = open(g_vndUserial.portName, O_RDWR)) == -1) {
        return -1;
    }
    tcflush(g_vndUserial.fd, TCIOFLUSH);
    tcgetattr(g_vndUserial.fd, &g_vndUserial.termios);
    cfmakeraw(&g_vndUserial.termios);
    g_vndUserial.termios.c_cflag |= (CRTSCTS | stopBits);
    tcsetattr(g_vndUserial.fd, TCSANOW, &g_vndUserial.termios);
    tcflush(g_vndUserial.fd, TCIOFLUSH);
    tcsetattr(g_vndUserial.fd, TCSANOW, &g_vndUserial.termios);
    tcflush(g_vndUserial.fd, TCIOFLUSH);
    tcflush(g_vndUserial.fd, TCIOFLUSH);
    cfsetospeed(&g_vndUserial.termios, baud);
    cfsetispeed(&g_vndUserial.termios, baud);
    tcsetattr(g_vndUserial.fd, TCSANOW, &g_vndUserial.termios);
#if (BT_WAKE_VIA_USERIAL_IOCTL == TRUE)
    UserialIoctlInitBtWake(g_vndUserial.fd);
#endif
    return g_vndUserial.fd;
}

/*******************************************************************************
**
** Function        UserialVendorClose
**
** Description     Conduct vendor-specific close work
**
** Returns         None
**
*******************************************************************************/
void UserialVendorClose(void)
{
    int result;

    if (g_vndUserial.fd == -1) {
        return;
    }

#if (BT_WAKE_VIA_USERIAL_IOCTL == TRUE)
    /* de-assert bt_wake BEFORE closing port */
    ioctl(g_vndUserial.fd, USERIAL_IOCTL_BT_WAKE_DEASSERT, NULL);
#endif

    HILOGI("device fd = %d close", g_vndUserial.fd);
    // flush Tx before close to make sure no chars in buffer
    tcflush(g_vndUserial.fd, TCIOFLUSH);
    if ((result = close(g_vndUserial.fd)) < 0) {
        HILOGE("close(fd:%d) FAILED result:%d", g_vndUserial.fd, result);
    }

    g_vndUserial.fd = -1;
}

/*******************************************************************************
**
** Function        UserialVendorSetBaud
**
** Description     Set new baud rate
**
** Returns         None
**
*******************************************************************************/
void UserialVendorSetBaud(uint8_t userialBaud)
{
    uint32_t tcio_baud;

    if (USERIAL_VENDOR_SET_BAUD_DELAY_US > 0) {
        usleep(USERIAL_VENDOR_SET_BAUD_DELAY_US);
    }

    UserialToTcioBaud(userialBaud, &tcio_baud);

    cfsetospeed(&g_vndUserial.termios, tcio_baud);
    cfsetispeed(&g_vndUserial.termios, tcio_baud);
    tcsetattr(g_vndUserial.fd, TCSANOW, &g_vndUserial.termios);
}

/*******************************************************************************
**
** Function        UserialVendorIoctl
**
** Description     ioctl inteface
**
** Returns         None
**
*******************************************************************************/
void UserialVendorIoctl(UserialVendorIoctlOpT op, void *pData)
{
    switch (op) {
#if (BT_WAKE_VIA_USERIAL_IOCTL == TRUE)
        case USERIAL_OP_ASSERT_BT_WAKE:
            HILOGD("## UserialVendorIoctl: Asserting BT_Wake ##");
            ioctl(g_vndUserial.fd, USERIAL_IOCTL_BT_WAKE_ASSERT, NULL);
            break;
        case USERIAL_OP_DEASSERT_BT_WAKE:
            HILOGD("## UserialVendorIoctl: De-asserting BT_Wake ##");
            ioctl(g_vndUserial.fd, USERIAL_IOCTL_BT_WAKE_DEASSERT, NULL);
            break;
        case USERIAL_OP_GET_btWakeState:
            ioctl(g_vndUserial.fd, USERIAL_IOCTL_BT_WAKE_GET_ST, pData);
            break;
#endif //  (BT_WAKE_VIA_USERIAL_IOCTL==TRUE)
        default:
            break;
    }
}

/*******************************************************************************
**
** Function        UserialSetPort
**
** Description     Configure UART port name
**
** Returns         0 : Success
**                 Otherwise : Fail
**
*******************************************************************************/
int UserialSetPort(char *pConfName, char *pConfValue, int param)
{
    (void)strcpy_s(g_vndUserial.portName, VND_PORT_NAME_MAXLEN, pConfValue);

    return 0;
}
