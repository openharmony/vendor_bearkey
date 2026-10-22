/******************************************************************************
 *
 *  Copyright (C) 1999-2012 Broadcom Corporation
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
 *  This file contains sync message over UIPC
 *
 ******************************************************************************/

#ifndef BT_VENDOR_UIPC_MSG_H
#define BT_VENDOR_UIPC_MSG_H

#include "bt_types.h"
#include "opcode.h

/****************************************************************************/
/*                            UIPC version number: 1.0                      */
/****************************************************************************/
#define UIPC_VERSION_MAJOR 0x0001
#define UIPC_VERSION_MINOR 0x0000

/********************************

    UIPC Management Messages

********************************/

/* tUIPC_STATUS codes */
enum {
    UIPC_STATUS_SUCCESS,
    UIPC_STATUS_FAIL
};

#pragma pack(push) /* push current alignment to stack */
#pragma pack(1)    /* set alignment to 1 byte boundary to allow for offset mappings */

/* Structure of UIPC_OPEN_REQ message */
typedef struct {
    uint8_t opcode; /* UIPC_OPEN_REQ */
} TUipcOpenReq;
#define UIPC_OPEN_REQ_MSGLEN (1)

/* Structure of UIPC_OPEN_RSP message */
typedef struct {
    uint8_t opcode;         /* UIPC_OPEN_RESP */
    uint8_t status;         /* UIPC_STATUS */
    uint16_t version_major; /* UIPC_VERSION_MAJOR */
    uint16_t version_minor; /* UIPC_VERSION_MINOR */
    uint8_t num_streams;    /* Number of simultaneous streams supported by the light stack */
} TUipcOpenRsp;
#define UIPC_OPEN_RSP_MSGLEN (7)

/* Structure of UIPC_CLOSE_REQ message */
typedef struct t_uipc_close_req {
    uint8_t opcode; /* UIPC_CLOSE_REQ */
} TUipcCloseReq;
#define UIPC_CLOSE_REQ_MSGLEN (1)

/* Structure of UIPC_CLOSE_RSP message, only for BTC, full stack may ignore it */
typedef struct t_uipc_close_rsp {
    uint8_t opcode; /* UIPC_CLOSE_RSP */
} TUipcCloseRsp;
#define UIPC_CLOSE_RSP_MSGLEN (1)

/* UIPC management message structures */
typedef union {
    uint8_t opcode;
    TUipcOpenReq openReq;
    TUipcOpenRsp openResp;
    TUipcCloseReq closeReq;
} TUipcMsg;

#define UIPC_MGMT_MSG_MAXLEN (sizeof(TUipcMsg))

#define IPC_LOG_MSG_LEN 100
typedef struct t_uipc_log_msg {
    uint32_t traceSetMask;
    uint8_t msg[IPC_LOG_MSG_LEN];
} TUipcLogMsg;
#define UIPC_LOG_MSGLEN (IPC_LOG_MSG_LEN + 4)

/********************************

    H5 Sync Message

********************************/

/* status */
#define SLIP_SYNC_SUCCESS 0
#define SLIP_SYNC_FAILURE 1

typedef struct {
    uint8_t opCode;
    uint8_t status;
    uint16_t aclPktSize;
    uint8_t state;
    uint8_t lp_state;            /* Low Power state */
    uint8_t nextSeqno;          /* next send seq */
    uint8_t ack;                 /* next ack seq, expected seq from peer */
    uint8_t sentAck;            /* last sent ack */
    uint8_t slidingWindowSize; /* window size */
    bool oofFlowControl;       /* Out of Frame SW Flow Control */
    bool dataIntegrityType;    /* Level of Data Integrity Check */
    uint8_t rx_state;            /* rx state for incoming packet processing */
} TSlipSyncInfo;

/********************************

    L2CAP Sync Message

********************************/

/* status */
#define L2C_SYNC_SUCCESS 0
#define L2C_SYNC_FAILURE 1

typedef struct t_l2c_stream_info {
    uint16_t localCid;       /* Local CID                        */
    uint16_t remoteCid;      /* Remote CID                       */
    uint16_t outMtu;         /* Max MTU we will send             */
    uint16_t handle;          /* The handle used with LM          */
    uint16_t linkXmitQuota; /* Num outstanding pkts allowed     */
    bool isFlushable;        /* TRUE if flushable channel        */
} TL2CStreamInfo;

typedef struct t_l2c_sync_to_lite_req {
    uint8_t opCode;                    /* L2C_SYNC_TO_LITE_REQ */
    uint16_t lightXmitQuota;          /* Total quota for light stack    */
    uint16_t aclDataSize;             /* Max ACL data size across HCI transport    */
    uint16_t nonFlushablePbf;         /* L2CAP_PKT_START_NON_FLUSHABLE if controller supports */
                                        /* Otherwise, L2CAP_PKT_START */
    uint8_t multiAvDataCongStart;   /* Multi-AV queue size to start congestion */
    uint8_t multiAvDataCongEnd;     /* Multi-AV queue size to end congestion */
    uint8_t multiAvDataCongDiscard; /* Multi-AV queue size to discard */
    uint8_t numStream;
    TL2CStreamInfo stream;
} TL2CSyncToLiteReq;

typedef struct t_l2c_sync_to_lite_resp_stream {
    uint16_t lcid;
    uint8_t status;
} TL2CSyncToLiteRespStream;

typedef struct t_l2c_sync_to_lite_resp {
    uint8_t opCode;             /* L2C_SYNC_TO_LITE_RESP */
    uint16_t lightXmitUnacked; /* unacked packet more than quota in light stack    */
    uint8_t numStream;
    TL2CSyncToLiteRespStream stream;
} TL2CSyncToLiteResp;

typedef struct t_l2c_remove_to_lite_req {
    uint8_t opCode;           /* L2C_REMOVE_TO_LITE_REQ */
    uint16_t lightXmitQuota; /* Total quota for light stack    */
    uint8_t numStream;
    uint16_t lcid;
} TL2CRemoveToLiteReq;

typedef TL2CSyncToLiteResp TL2CRemoveToLiteResp;
typedef TL2CRemoveToLiteReq TL2CFlushToFullInd;

typedef union t_l2c_sync_msg {
    uint8_t opCode;
    TL2CSyncToLiteReq syncReq;
    TL2CSyncToLiteResp syncResp;
    TL2CRemoveToLiteReq removeReq;
    TL2CRemoveToLiteResp removeResp;
    TL2CFlushToFullInd flushInd;
} TL2CSyncMsg;

/********************************

    AVDTP Sync Message

********************************/

/* status */
#define AVDT_SYNC_SUCCESS 0
#define AVDT_SYNC_FAILURE 1

typedef struct {
    uint16_t lcid;
    uint32_t ssrc;
} TAvdtSyncToBtcLiteReqStream;

typedef struct {
    uint8_t opcode; /* AVDT_SYNC_TO_BTC_LITE_REQ */
    uint8_t numStream;
    TAvdtSyncToBtcLiteReqStream stream;
} TAvdtSyncToBtcLiteReq;

typedef struct {
    uint8_t opcode; /* AVDT_SYNC_TO_BTC_LITE_RESP */
    uint8_t status;
} TAvdtSyncToBtcLiteResp;

typedef struct t_avdt_scb_sync_info {
    uint8_t handle;         /* SCB handle */
    BD_ADDR peerAddr;      /* BD address of peer */
    uint16_t localCid;     /* Local CID                        */
    uint16_t peerMtu;      /* L2CAP mtu of the peer device */
    uint8_t muxTsidMedia; /* TSID for media transport session */
    uint16_t mediaSeq;     /* media packet sequence number */
} TAvdtScbSyncInfo;

typedef struct t_avdt_sync_info {
    uint8_t opCode;
    uint8_t status;
    TAvdtScbSyncInfo scbInfo;
} TAvdtSyncInfo;

typedef union t_avdt_sync_msg {
    uint8_t opCode;
    TAvdtSyncInfo syncInfo;
    TAvdtSyncToBtcLiteReq btcSyncReq;
    TAvdtSyncToBtcLiteResp btcSyncResp;
} TAvdtSyncMsg;

/********************************

    BTA AV Sync Message

********************************/

/* For TIVO, only applicable for I2S -> DAC */
#define AUDIO_SUB_ROUTE_REQ 51
#define AUDIO_SUB_ROUTE_RESP 52

typedef struct {
    uint8_t opcode; /* A2DP_START_REQ */
    uint16_t lcid;
    uint16_t currMtu;
} TA2DpStartReq;

typedef struct {
    uint8_t opcode; /* A2DP_STOP_REQ */
    uint16_t lcid;
} TA2DpStopReq;

typedef struct {
    uint8_t opcode; /* A2DP_SUSPEND_REQ */
    uint16_t lcid;
} TA2DpSuspendReq;

typedef struct {
    uint8_t opcode; /* A2DP_CLEANUP_REQ */
    uint16_t lcid;
    uint16_t currMtu;
} TA2DpCleanupReq;

typedef struct {
    uint8_t opcode; /* A2DP_START_RESP, A2DP_STOP_RESP, A2DP_CLEANUP_RESP, A2DP_SUSPEND_RESP */
    uint16_t lcid;
} TA2DpGenericResp;

#define AUDIO_CODEC_NONE 0x0000
#define AUDIO_CODEC_SBC_ENC 0x0001
#define AUDIO_CODEC_SBC_DEC 0x0002
#define AUDIO_CODEC_MP3_ENC 0x0004
#define AUDIO_CODEC_MP3_DEC 0x0008
#define AUDIO_CODEC_AAC_ENC 0x0010
#define AUDIO_CODEC_AAC_DEC 0x0020
#define AUDIO_CODEC_AAC_PLUS_ENC 0x0040
#define AUDIO_CODEC_AAC_PLUS_DEC 0x0080
#define AUDIO_CODEC_MP2_ENC 0x0100
#define AUDIO_CODEC_MP2_DEC 0x0200
#define AUDIO_CODEC_MP2_5_ENC 0x0400
#define AUDIO_CODEC_MP2_5_DEC 0x0800

/* SBC CODEC Parameters */

#define CODEC_INFO_SBC_SF_16K 0x00
#define CODEC_INFO_SBC_SF_32K 0x01
#define CODEC_INFO_SBC_SF_44K 0x02
#define CODEC_INFO_SBC_SF_48K 0x03

#define CODEC_INFO_SBC_BLOCK_4 0x00
#define CODEC_INFO_SBC_BLOCK_8 0x01
#define CODEC_INFO_SBC_BLOCK_12 0x02
#define CODEC_INFO_SBC_BLOCK_16 0x03

#define CODEC_INFO_SBC_CH_MONO 0x00
#define CODEC_INFO_SBC_CH_DUAL 0x01
#define CODEC_INFO_SBC_CH_STEREO 0x02
#define CODEC_INFO_SBC_CH_JS 0x03

#define CODEC_INFO_SBC_ALLOC_LOUDNESS 0x00
#define CODEC_INFO_SBC_ALLOC_SNR 0x01

#define CODEC_INFO_SBC_SUBBAND_4 0x00
#define CODEC_INFO_SBC_SUBBAND_8 0x01

/* MPEG audio version ID */
#define CODEC_INFO_MP25_ID 0x00
#define CODEC_INFO_RESERVE 0x01
#define CODEC_INFO_MP2_ID 0x02
#define CODEC_INFO_MP3_ID 0x03

#define CODEC_INFO_MP3_PROTECTION_ON 0x00
#define CODEC_INFO_MP3_PROTECTION_OFF 0x01

#define CODEC_INFO_MP3_BR_IDX_FREE 0x00
#define CODEC_INFO_MP3_BR_IDX_32K 0x01
#define CODEC_INFO_MP3_BR_IDX_40K 0x02
#define CODEC_INFO_MP3_BR_IDX_48K 0x03
#define CODEC_INFO_MP3_BR_IDX_56K 0x04
#define CODEC_INFO_MP3_BR_IDX_64K 0x05
#define CODEC_INFO_MP3_BR_IDX_80K 0x06
#define CODEC_INFO_MP3_BR_IDX_96K 0x07
#define CODEC_INFO_MP3_BR_IDX_112K 0x08
#define CODEC_INFO_MP3_BR_IDX_128K 0x09
#define CODEC_INFO_MP3_BR_IDX_160K 0x0A
#define CODEC_INFO_MP3_BR_IDX_192K 0x0B
#define CODEC_INFO_MP3_BR_IDX_224K 0x0C
#define CODEC_INFO_MP3_BR_IDX_256K 0x0D
#define CODEC_INFO_MP3_BR_IDX_320K 0x0E

#define CODEC_INFO_MP3_SF_44K 0x00
#define CODEC_INFO_MP3_SF_48K 0x01
#define CODEC_INFO_MP3_SF_32K 0x02

#define CODEC_INFO_MP3_MODE_STEREO 0x00
#define CODEC_INFO_MP3_MODE_JS 0x01
#define CODEC_INFO_MP3_MODE_DUAL 0x02
#define CODEC_INFO_MP3_MODE_SINGLE 0x03

/* layer 3, type of joint stereo coding method (intensity and ms) */
#define CODEC_INFO_MP3_MODE_EXT_OFF_OFF 0x00
#define CODEC_INFO_MP3_MODE_EXT_ON_OFF 0x01
#define CODEC_INFO_MP3_MODE_EXT_OFF_ON 0x02
#define CODEC_INFO_MP3_MODE_EXT_ON_ON 0x03

#define CODEC_INFO_MP2_PROTECTION_ON 0x00
#define CODEC_INFO_MP2_PROTECTION_OFF 0x01

#define CODEC_INFO_MP2_BR_IDX_FREE 0x00
#define CODEC_INFO_MP2_BR_IDX_8K 0x01
#define CODEC_INFO_MP2_BR_IDX_16K 0x02
#define CODEC_INFO_MP2_BR_IDX_24K 0x03
#define CODEC_INFO_MP2_BR_IDX_32K 0x04
#define CODEC_INFO_MP2_BR_IDX_40K 0x05
#define CODEC_INFO_MP2_BR_IDX_48K 0x06
#define CODEC_INFO_MP2_BR_IDX_56K 0x07
#define CODEC_INFO_MP2_BR_IDX_64K 0x08
#define CODEC_INFO_MP2_BR_IDX_80K 0x09
#define CODEC_INFO_MP2_BR_IDX_96K 0x0A
#define CODEC_INFO_MP2_BR_IDX_112K 0x0B
#define CODEC_INFO_MP2_BR_IDX_128K 0x0C
#define CODEC_INFO_MP2_BR_IDX_144K 0x0D
#define CODEC_INFO_MP2_BR_IDX_160K 0x0E

#define CODEC_INFO_MP2_SF_22K 0x00
#define CODEC_INFO_MP2_SF_24K 0x01
#define CODEC_INFO_MP2_SF_16K 0x02

#define CODEC_INFO_MP2_MODE_STEREO 0x00
#define CODEC_INFO_MP2_MODE_JS 0x01
#define CODEC_INFO_MP2_MODE_DUAL 0x02
#define CODEC_INFO_MP2_MODE_SINGLE 0x03

/* layer 3, type of joint stereo coding method (intensity and ms) */
#define CODEC_INFO_MP2_MODE_EXT_OFF_OFF 0x00
#define CODEC_INFO_MP2_MODE_EXT_ON_OFF 0x01
#define CODEC_INFO_MP2_MODE_EXT_OFF_ON 0x02
#define CODEC_INFO_MP2_MODE_EXT_ON_ON 0x03

#define CODEC_INFO_MP2_SAMPLE_PER_FRAME 576

/* mpeg 2.5 layer 3 decoder */

#define CODEC_INFO_MP25_PROTECTION_ON 0x00
#define CODEC_INFO_MP25_PROTECTION_OFF 0x01

#define CODEC_INFO_MP25_BR_IDX_FREE 0x00
#define CODEC_INFO_MP25_BR_IDX_8K 0x01
#define CODEC_INFO_MP25_BR_IDX_16K 0x02
#define CODEC_INFO_MP25_BR_IDX_24K 0x03
#define CODEC_INFO_MP25_BR_IDX_32K 0x04
#define CODEC_INFO_MP25_BR_IDX_40K 0x05
#define CODEC_INFO_MP25_BR_IDX_48K 0x06
#define CODEC_INFO_MP25_BR_IDX_56K 0x07
#define CODEC_INFO_MP25_BR_IDX_64K 0x08
#define CODEC_INFO_MP25_BR_IDX_80K 0x09
#define CODEC_INFO_MP25_BR_IDX_96K 0x0A
#define CODEC_INFO_MP25_BR_IDX_112K 0x0B
#define CODEC_INFO_MP25_BR_IDX_128K 0x0C
#define CODEC_INFO_MP25_BR_IDX_144K 0x0D
#define CODEC_INFO_MP25_BR_IDX_160K 0x0E

#define CODEC_INFO_MP25_SF_11K 0x00
#define CODEC_INFO_MP25_SF_12K 0x01
#define CODEC_INFO_MP25_SF_8K 0x02

#define CODEC_INFO_MP25_MODE_STEREO 0x00
#define CODEC_INFO_MP25_MODE_JS 0x01
#define CODEC_INFO_MP25_MODE_DUAL 0x02
#define CODEC_INFO_MP25_MODE_SINGLE 0x03

/* layer 3, type of joint stereo coding method (intensity and ms) */
#define CODEC_INFO_MP25_MODE_EXT_OFF_OFF 0x00
#define CODEC_INFO_MP25_MODE_EXT_ON_OFF 0x01
#define CODEC_INFO_MP25_MODE_EXT_OFF_ON 0x02
#define CODEC_INFO_MP25_MODE_EXT_ON_ON 0x03

#define CODEC_INFO_MP25_SAMPLE_PER_FRAME 576

/* AAC/AAC+ CODEC Parameters */
#define CODEC_INFO_AAC_SF_IDX_96K 0x0
#define CODEC_INFO_AAC_SF_IDX_88K 0x1
#define CODEC_INFO_AAC_SF_IDX_64K 0x2
#define CODEC_INFO_AAC_SF_IDX_48K 0x3
#define CODEC_INFO_AAC_SF_IDX_44K 0x4
#define CODEC_INFO_AAC_SF_IDX_32K 0x5
#define CODEC_INFO_AAC_SF_IDX_24K 0x6
#define CODEC_INFO_AAC_SF_IDX_22K 0x7
#define CODEC_INFO_AAC_SF_IDX_16K 0x8
#define CODEC_INFO_AAC_SF_IDX_12K 0x9
#define CODEC_INFO_AAC_SF_IDX_11K 0xA
#define CODEC_INFO_AAC_SF_IDX_08K 0xB
#define CODEC_INFO_AAC_SF_IDX_RESERVE 0xC

#define CODEC_INFO_AAC_BR_RATE_48K 288000
#define CODEC_INFO_AAC_BR_RATE_44K 264600
#define CODEC_INFO_AAC_BR_RATE_32K 192000

#define CODEC_INFO_AAC_1_CH 1 /* center front speaker */
#define CODEC_INFO_AAC_2_CH 2 /* left, right front speaker */
#define CODEC_INFO_AAC_3_CH 3 /* center front speaker, left right front speaker */
#define CODEC_INFO_AAC_4_CH 4 /* center/rear front speaker, left/right front speaker */
#define CODEC_INFO_AAC_5_CH 5 /* center, left, right front speaker, left/right surround */
#define CODEC_INFO_AAC_6_CH 6 /* center, left, right front speaker, left/right surround, LFE */
#define CODEC_INFO_AAC_7_CH 7 /* (left, right)center/left,right front speaker, left/right surround, LFE */

typedef struct {
    uint8_t samplingFreq;
    uint8_t channelMode;
    uint8_t blockLength;
    uint8_t numSubbands;
    uint8_t allocMethod;
    uint8_t bitpoolSize; /* 2 - 250 */
} TCodecInfoSbc;

typedef struct {
    uint8_t chMode;
    uint8_t samplingFreq;
    uint8_t bitrateIndex; /* 0 - 14 */
} TCodecInfoMP3;

typedef struct {
    uint8_t chMode;
    uint8_t samplingFreq;
    uint8_t bitrateIndex; /* 0 - 14 */
} TCodecInfoMP2;

typedef struct {
    uint8_t chMode;
    uint8_t samplingFreq;
    uint8_t bitrateIndex; /* 0 - 14 */
} TCodecInfoMP25;

typedef struct {
    uint16_t samplingFreq;
    uint8_t channelMode; /* 0x02:mono, 0x01:dual */
    uint32_t bitrate;     /* 0 - 320K */
    uint32_t sbrProfile; /* 1: ON, 0: OFF */
} TCodecInfoAac;

typedef union {
    TCodecInfoSbc sbc;
    TCodecInfoMP3 mp3;
    TCodecInfoMP2 mp2;
    TCodecInfoMP25 mp25;
    TCodecInfoAac aac;
} TCodecInfo;

typedef struct {
    uint8_t opcode; /* AUDIO_CODEC_CONFIG_REQ */
    uint16_t codecType;
    TCodecInfo codecInfo;
} TAudioCodecConfigReq;

#define AUDIO_CONFIG_SUCCESS 0x00
#define AUDIO_CONFIG_NOT_SUPPORTED 0x01
#define AUDIO_CONFIG_FAIL_OUT_OF_MEMORY 0x02
#define AUDIO_CONFIG_FAIL_CODEC_USED 0x03
#define AUDIO_CONFIG_FAIL_ROUTE 0x04

typedef struct {
    uint8_t opcode; /* AUDIO_CODEC_CONFIG_RESP */
    uint8_t status;
} TAudioCodecConfigResp;

typedef struct {
    uint8_t opcode; /* AUDIO_CODEC_SET_BITRATE_REQ */
    uint16_t codecType;
    union {
        uint8_t sbc;
        uint8_t mp3;
        uint32_t aac;
    } codecBitrate;
} TAudioCodecSetBitrateReq;


typedef struct {
    uint8_t opcode; /* AUDIO_ROUTE_CONFIG_REQ */
    uint8_t src;
    uint8_t src_sf;
    uint8_t out;
    uint8_t out_codec_sf;
    uint8_t out_i2s_sf;
    uint8_t eq_mode;
} TAudioRouteConfigReq;

typedef struct {
    uint8_t opcode; /* AUDIO_ROUTE_CONFIG_RESP */
    uint8_t status;
} TAudioRouteConfigResp;

typedef struct {
    uint16_t amp[2];   /* left/right 15 bit amplitude value        */
    uint16_t tone[2];  /* left/right 12 bit frequency 0 - 4096Hz   */
    uint16_t mark[2];  /* left/right 16 bit mark time 0 - 65535ms  */
    uint16_t space[2]; /* left/right 16 bit space time 0 - 65535ms */
} TChirpConfig;

typedef struct {
    uint8_t priL; /* Primary Left scale : 0 ~ 255     */
    uint8_t mixL; /* Mixing Left scale : 0 ~ 255      */
    uint8_t priR; /* Primary Right scale : 0 ~ 255    */
    uint8_t mixR; /* Mixing Right scale : 0 ~ 255     */
} TMixScaleConfig;

/* For custon equalizer gain configuration */
typedef struct {
    uint32_t audio_l_g0; /* IIR biquad filter left ch gain 0 */
    uint32_t audio_l_g1; /* IIR biquad filter left ch gain 1 */
    uint32_t audio_l_g2; /* IIR biquad filter left ch gain 2 */
    uint32_t audio_l_g3; /* IIR biquad filter left ch gain 3 */
    uint32_t audio_l_g4; /* IIR biquad filter left ch gain 4 */
    uint32_t audio_l_gl; /* IIR biquad filter left ch global gain  */
    uint32_t audio_r_g0; /* IIR biquad filter left ch gain 0 */
    uint32_t audio_r_g1; /* IIR biquad filter left ch gain 1 */
    uint32_t audio_r_g2; /* IIR biquad filter left ch gain 2 */
    uint32_t audio_r_g3; /* IIR biquad filter left ch gain 3 */
    uint32_t audio_r_g4; /* IIR biquad filter left ch gain 4 */
    uint32_t audio_r_gl; /* IIR biquad filter left ch global gain */
} TEqGainConfig;

typedef struct {
    uint8_t opcode; /* AUDIO_MIX_CONFIG_REQ */
    uint8_t mixSrc;
    uint8_t mixSrcSf;
    TMixScaleConfig mixScale;
    TChirpConfig chirpConfig;
} TAudioMixConfigReq;

typedef struct {
    uint8_t opcode; /* AUDIO_MIX_CONFIG_RESP */
    uint8_t status;
} TAudioMixConfigResp;

typedef struct {
    uint8_t opcode;      /* AUDIO_BURST_FRAMES_IND */
    uint32_t burstSize; /* in bytes */
} TAudioBurstFramesInd;

typedef struct {
    uint8_t opcode; /* AUDIO_BURST_END_IND */
} TAudioBurstEndInd;

typedef struct {
    uint8_t opcode; /* AUDIO_CODEC_FLUSH_REQ */
} TAudioCodecFlushReq;

typedef struct {
    uint8_t opcode; /* AUDIO_EQ_MODE_CONFIG_REQ */
    uint8_t eq_mode;
    TEqGainConfig filterGain; /* Valid only when eq_mode is 0xFF */
} TAudioEqModeConfigReq;

typedef struct {
    uint8_t opcode; /* AUDIO_SCALE_CONFIG_REQ */
    TMixScaleConfig mixScale;
} TAudioScaleConfigReq;

#pragma pack(pop) /* pop saved alignment to stack */

#endif /* BT_VENDOR_UIPC_MSG_H */
