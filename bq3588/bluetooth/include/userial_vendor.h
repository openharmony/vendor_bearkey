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
 *  Filename:      userial_vendor.h
 *
 *  Description:   Contains vendor-specific definitions used in serial port
 *                 controls
 *
 ******************************************************************************/

#ifndef USERIAL_VENDOR_H
#define USERIAL_VENDOR_H

#include "bt_vendor_brcm.h"
#include "userial.h"

/******************************************************************************
**  Constants & Macros
******************************************************************************/

/**** baud rates ****/
#define USERIAL_BAUD_300 0
#define USERIAL_BAUD_600 1
#define USERIAL_BAUD_1200 2
#define USERIAL_BAUD_2400 3
#define USERIAL_BAUD_9600 4
#define USERIAL_BAUD_19200 5
#define USERIAL_BAUD_57600 6
#define USERIAL_BAUD_115200 7
#define USERIAL_BAUD_230400 8
#define USERIAL_BAUD_460800 9
#define USERIAL_BAUD_921600 10
#define USERIAL_BAUD_1M 11
#define USERIAL_BAUD_1_5M 12
#define USERIAL_BAUD_2M 13
#define USERIAL_BAUD_3M 14
#define USERIAL_BAUD_4M 15
#define USERIAL_BAUD_AUTO 16

#define USERIAL_LINESPEED_600 600
#define USERIAL_LINESPEED_1200 1200
#define USERIAL_LINESPEED_2400 2400
#define USERIAL_LINESPEED_9600 9600
#define USERIAL_LINESPEED_19200 19200
#define USERIAL_LINESPEED_57600 57600
#define USERIAL_LINESPEED_115200 115200
#define USERIAL_LINESPEED_230400 230400
#define USERIAL_LINESPEED_460800 460800
#define USERIAL_LINESPEED_921600 921600
#define USERIAL_LINESPEED_1M 1000000
#define USERIAL_LINESPEED_1_5M 1500000
#define USERIAL_LINESPEED_2M 2000000
#define USERIAL_LINESPEED_3M 3000000
#define USERIAL_LINESPEED_4M 4000000

/**** Data Format ****/
/* Stop Bits */
#define USERIAL_STOPBITS_1 1
#define USERIAL_STOPBITS_1_5 (1 << 1)
#define USERIAL_STOPBITS_2 (1 << 2)

/* Parity Bits */
#define USERIAL_PARITY_NONE (1 << 3)
#define USERIAL_PARITY_EVEN (1 << 4)
#define USERIAL_PARITY_ODD (1 << 5)

/* Data Bits */
#define USERIAL_DATABITS_5 (1 << 6)
#define USERIAL_DATABITS_6 (1 << 7)
#define USERIAL_DATABITS_7 (1 << 8)
#define USERIAL_DATABITS_8 (1 << 9)

#if (BT_WAKE_VIA_USERIAL_IOCTL == TRUE)
/* These are the ioctl values used for bt_wake ioctl via UART driver. you may
 * need to redefine them on you platform!
 * Logically they need to be unique and not colide with existing uart ioctl's.
 */
#ifndef USERIAL_IOCTL_BT_WAKE_ASSERT
#define USERIAL_IOCTL_BT_WAKE_ASSERT 0x8003
#endif
#ifndef USERIAL_IOCTL_BT_WAKE_DEASSERT
#define USERIAL_IOCTL_BT_WAKE_DEASSERT 0x8004
#endif
#ifndef USERIAL_IOCTL_BT_WAKE_GET_ST
#define USERIAL_IOCTL_BT_WAKE_GET_ST 0x8005
#endif
#endif // (BT_WAKE_VIA_USERIAL_IOCTL==TRUE)

/******************************************************************************
**  Type definitions
******************************************************************************/

/* Structure used to configure serial port during open */
typedef struct {
    uint16_t fmt; /* Data format */
    uint8_t baud; /* Baud rate */
} TUserialCfg;

typedef enum {
#if (BT_WAKE_VIA_USERIAL_IOCTL == TRUE)
    USERIAL_OP_ASSERT_BT_WAKE,
    USERIAL_OP_DEASSERT_BT_WAKE,
    USERIAL_OP_GET_BT_WAKE_STATE,
#endif
    USERIAL_OP_NOP,
} UserialVendorIoctlOpT;

/******************************************************************************
**  Extern variables and functions
******************************************************************************/

/******************************************************************************
**  Functions
******************************************************************************/

/*******************************************************************************
**
** Function        UserialVendorInit
**
** Description     Initialize userial vendor-specific control block
**
** Returns         None
**
*******************************************************************************/
void UserialVendorInit(void);

/*******************************************************************************
**
** Function        UserialVendorOpen
**
** Description     Open the serial port with the given configuration
**
** Returns         device fd
**
*******************************************************************************/
int UserialVendorOpen(TUserialCfg *pCfg);

/*******************************************************************************
**
** Function        UserialVendorClose
**
** Description     Conduct vendor-specific close work
**
** Returns         None
**
*******************************************************************************/
void UserialVendorClose(void);

/*******************************************************************************
**
** Function        UserialVendorSetBaud
**
** Description     Set new baud rate
**
** Returns         None
**
*******************************************************************************/
void UserialVendorSetBaud(uint8_t userialBaud);

/*******************************************************************************
**
** Function        UserialVendorIoctl
**
** Description     ioctl inteface
**
** Returns         None
**
*******************************************************************************/
void UserialVendorIoctl(UserialVendorIoctlOpT op, void *pData);

#endif /* USERIAL_VENDOR_H */
