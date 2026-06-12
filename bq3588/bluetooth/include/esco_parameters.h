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
#ifndef BT_VENDOR_ESCO_PARAMETER_H
#define BT_VENDOR_ESCO_PARAMETER_H

#include <stdint.h>
/*******************
 * SCO Codec Types
 *******************/
typedef enum {
    SCO_CODEC_NONE = 0x0000,
    SCO_CODEC_CVSD = 0x0001,
    SCO_CODEC_MSBC = 0x0002,
} ScoCodecT;

typedef enum {
    ESCO_CODEC_CVSD = 0,
    ESCO_CODEC_MSBC_T1,
    ESCO_CODEC_MSBC_T2,
} EscoCodecT;

#define ESCO_NUM_CODECS 3

// Coding Formats (BT 4.1 or later Assigned numbers)
#define ESCO_CODING_FORMAT_ULAW ((uint8_t)0x00)     /* u-Law log    */
#define ESCO_CODING_FORMAT_ALAW ((uint8_t)0x01)     /* A-Law log    */
#define ESCO_CODING_FORMAT_CVSD ((uint8_t)0x02)     /* CVSD         */
#define ESCO_CODING_FORMAT_TRANSPNT ((uint8_t)0x03) /* Transparent  */
#define ESCO_CODING_FORMAT_LINEAR ((uint8_t)0x04)   /* Linear PCM   */
#define ESCO_CODING_FORMAT_MSBC ((uint8_t)0x05)     /* MSBC PCM   */
#define ESCO_CODING_FORMAT_VS ((uint8_t)0xFF)       /* Specifies VSC used */

// PCM Data Formats (BT 4.1 or later Assigned numbers)
#define ESCO_PCM_DATA_FORMAT_NA ((uint8_t)0x00)                                   /* N/A to coding format in use */
#define ESCO_PCM_DATA_FORMAT_1_COMP ((uint8_t)0x01) /* 1's complement   */
#define ESCO_PCM_DATA_FORMAT_2_COMP ((uint8_t)0x02) /* 2's complement   */
#define ESCO_PCM_DATA_FORMAT_SIGN ((uint8_t)0x03)   /* Sign-magnitude   */
#define ESCO_PCM_DATA_FORMAT_UNSIGN ((uint8_t)0x04) /* Unsigned         */

// SCO Data Path
#define ESCO_DATA_PATH_PCM 1                /* 0x01-0xFE (PCM Chan) */
#define ESCO_DATA_PATH_HCI ((uint8_t)0x00)  /* HCI-0, 0x01-0xFE (PCM Chan) */
#define ESCO_DATA_PATH_TEST ((uint8_t)0xFF) /* 0xFF-Audio Test */

// eSCO constants
#define TXRX_64KBITS_RATE 0x00001f40  /* 64 kbits/sec data rate */
#define TXRX_128KBITS_RATE 0x00003E80 /* 128 kbits/sec data rate */

#define INPUT_OUTPUT_64K_RATE 0x00003E80  /* 16000 Bytes/sec over transport */
#define INPUT_OUTPUT_128K_RATE 0x00007D00 /* 32000 Bytes/sec over transport */

// Retransmission effort
#define ESCO_RETRANSMISSION_OFF 0
#define ESCO_RETRANSMISSION_POWER 1
#define ESCO_RETRANSMISSION_QUALITY 2
#define ESCO_RETRANSMISSION_DONTCARE 0xff

// Definitions for eSCO packet type masks (BT1.2 and BT2.0 definitions)
#define ESCO_PKT_TYPES_MASK_HV1 0x0001
#define ESCO_PKT_TYPES_MASK_HV2 0x0002
#define ESCO_PKT_TYPES_MASK_HV3 0x0004
#define ESCO_PKT_TYPES_MASK_EV3 0x0008
#define ESCO_PKT_TYPES_MASK_EV4 0x0010
#define ESCO_PKT_TYPES_MASK_EV5 0x0020
#define ESCO_PKT_TYPES_MASK_NO_2_EV3 0x0040
#define ESCO_PKT_TYPES_MASK_NO_3_EV3 0x0080
#define ESCO_PKT_TYPES_MASK_NO_2_EV5 0x0100
#define ESCO_PKT_TYPES_MASK_NO_3_EV5 0x0200

// type definition
typedef struct {
    uint8_t codingFormat;             /* Coding Format */
    uint16_t companyId;               /* Company ID from BT SIG Assigned Numbers */
    uint16_t vendorSpecificCodecId; /* Vendor Specific Codec ID */
} EscoCodingIdFormatT;

// Enhanced setup/accept synchronous connection See BT 4.1 or later HCI spec for
// details
typedef struct {
    uint32_t transmitBandwidth;                    /* Transmit Bandwidth (in octets/second) */
    uint32_t receiveBandwidth;                     /* RX BW (# of octets/second) */
    EscoCodingIdFormatT transmitCodingFormat; /* TX coding format */
    EscoCodingIdFormatT receiveCodingFormat;  /* RX coding format */
    uint16_t transmitCodecFrameSize;             /* TX CODEC frame size (OTA frame size) */
    uint16_t receiveCodecFrameSize;              /* RX CODEC frame size (OTA frame size) */
    uint32_t inputBandwidth;                       /* Input BW (nominal rate octets/sec) */
    uint32_t outputBandwidth;                      /* Output BW (nominal rate octets/sec) */
    EscoCodingIdFormatT inputCodingFormat;    /* Input coding format */
    EscoCodingIdFormatT outputCodingFormat;   /* Output coding format */
    uint16_t inputCodedDataSize;                 /* Input coded data size (in bits) */
    uint16_t outputCodedDataSize;                /* Output coded data size (in bits) */
    uint8_t inputPcmDataFormat;                  /* Input PCM data format (see hcidefs.h) */
    uint8_t outputPcmDataFormat;                 /* Output PCM data format (see hcidefs.h) */
    uint8_t inputPcmPayloadMsbPosition;         /* Input PCM sample payload MSB position */
    uint8_t outputPcmPayloadMsbPosition;        /* Output PCM sample payload MSB position */
    uint8_t inputDataPath;                        /* 0x00 - HCI, or 0x01-0xFE for VS) */
    uint8_t outputDataPath;                       /* 0x00 - HCI, or 0x01-0xFE for VS) */
    uint8_t inputTransportUnitSize;              /* Input transport unit size */
    uint8_t outputTransportUnitSize;             /* Output transport unit size */
    uint16_t maxLatencyMs;                        /* Maximum latency (0x4-0xFFFE in msecs) */
    uint16_t packetTypes;                          /* Packet Types */
    uint8_t retransmissionEffort;                  /* 0x00-0x02, 0xFF don't care */
} EnhEscoParamsT;

// Get the enhanced eSCO configuration parameters for the provided |codec|
EnhEscoParamsT EscoParametersForCodec(EscoCodecT codec);
#endif