/******************************************************************************
 *
 *  Copyright 2015 Broadcom Corporation
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
#ifndef BT_OPCODE_H
#define BT_OPCODE_H

/* opCode */
#define UIPC_OPEN_REQ 0x00
#define UIPC_OPEN_RSP 0x01
#define UIPC_CLOSE_REQ 0x02
#define UIPC_CLOSE_RSP 0x03

/* opCode */
#define SLIP_SYNC_TO_LITE_REQ 0
#define SLIP_SYNC_TO_LITE_RESP 1
#define SLIP_SYNC_TO_FULL_REQ 2
#define SLIP_SYNC_TO_FULL_RESP 3
#define SLIP_SYNC_NOTIFY 4

/* opCode */
#define L2C_SYNC_TO_LITE_REQ 0
#define L2C_SYNC_TO_LITE_RESP 1
#define L2C_REMOVE_TO_LITE_REQ 2
#define L2C_REMOVE_TO_LITE_RESP 3
#define L2C_FLUSH_TO_FULL_IND 4

/* opCode */
#define AVDT_SYNC_TO_LITE_REQ 0
#define AVDT_SYNC_TO_LITE_RESP 1
#define AVDT_RESYNC_TO_LITE_REQ 2
#define AVDT_RESYNC_TO_LITE_RESP 3
#define AVDT_SYNC_TO_FULL_REQ 4
#define AVDT_SYNC_TO_FULL_RESP 5
#define AVDT_REMOVE_TO_LITE_REQ 6
#define AVDT_REMOVE_TO_LITE_RESP 7
#define AVDT_SYNC_TO_BTC_LITE_REQ 8
#define AVDT_SYNC_TO_BTC_LITE_RESP 9

/* opCode for MM light stack */
#define BTA_AV_SYNC_TO_LITE_REQ 0
#define BTA_AV_SYNC_TO_LITE_RESP 1
#define BTA_AV_STR_START_TO_LITE_REQ 2
#define BTA_AV_STR_START_TO_LITE_RESP 3
#define BTA_AV_STR_STOP_TO_LITE_REQ 4
#define BTA_AV_STR_STOP_TO_LITE_RESP 5
#define BTA_AV_STR_CLEANUP_TO_LITE_REQ 6
#define BTA_AV_STR_CLEANUP_TO_LITE_RESP 7
#define BTA_AV_STR_SUSPEND_TO_LITE_REQ 8
#define BTA_AV_STR_SUSPEND_TO_LITE_RESP 9
#define BTA_AV_SYNC_ERROR_RESP 10

/* opCode for BTC light stack */
#define A2DP_START_REQ 11
#define A2DP_START_RESP 12
#define A2DP_STOP_REQ 13
#define A2DP_STOP_RESP 14
#define A2DP_CLEANUP_REQ 15
#define A2DP_CLEANUP_RESP 16
#define A2DP_SUSPEND_REQ 17
#define A2DP_SUSPEND_RESP 18

#define A2DP_JITTER_DONE_IND 41 /* For BTSNK */

#define AUDIO_CODEC_CONFIG_REQ 19
#define AUDIO_CODEC_CONFIG_RESP 20
#define AUDIO_CODEC_SET_BITRATE_REQ 21
#define AUDIO_CODEC_FLUSH_REQ 22
#define AUDIO_ROUTE_CONFIG_REQ 23
#define AUDIO_ROUTE_CONFIG_RESP 24
#define AUDIO_MIX_CONFIG_REQ 25
#define AUDIO_MIX_CONFIG_RESP 26
#define AUDIO_BURST_FRAMES_IND 27
#define AUDIO_BURST_END_IND 28
#define AUDIO_EQ_MODE_CONFIG_REQ 29
#define AUDIO_SCALE_CONFIG_REQ 30


#define AUDIO_ROUTE_SRC_FMRX 0x00
#define AUDIO_ROUTE_SRC_I2S 0x01
#define AUDIO_ROUTE_SRC_ADC 0x02
#define AUDIO_ROUTE_SRC_HOST 0x03
#define AUDIO_ROUTE_SRC_PTU 0x04
#define AUDIO_ROUTE_SRC_BTSNK 0x05
#define AUDIO_ROUTE_SRC_NONE 0x80
#define MAX_AUDIO_ROUTE_SRC 6

#define AUDIO_ROUTE_MIX_NONE 0x00
#define AUDIO_ROUTE_MIX_HOST 0x01
#define AUDIO_ROUTE_MIX_PCM 0x02
#define AUDIO_ROUTE_MIX_CHIRP 0x03
#define AUDIO_ROUTE_MIX_I2S 0x04
#define AUDIO_ROUTE_MIX_ADC 0x05
#define AUDIO_ROUTE_MIX_RESERVED 0x06
#define MAX_AUDIO_ROUTE_MIX 7

#define AUDIO_ROUTE_OUT_NONE 0x0000
#define AUDIO_ROUTE_OUT_BTA2DP 0x0001
#define AUDIO_ROUTE_OUT_FMTX 0x0002
#define AUDIO_ROUTE_OUT_BTSCO 0x0004
#define AUDIO_ROUTE_OUT_HOST 0x0008
#define AUDIO_ROUTE_OUT_DAC 0x0010
#define AUDIO_ROUTE_OUT_I2S 0x0020
#define AUDIO_ROUTE_OUT_BTA2DP_DAC 0x0040
#define AUDIO_ROUTE_OUT_BTA2DP_I2S 0x0080
#define AUDIO_ROUTE_OUT_BTSCO_DAC 0x0100
#define AUDIO_ROUTE_OUT_BTSCO_I2S 0x0200
#define AUDIO_ROUTE_OUT_HOST_BTA2DP 0x0400
#define AUDIO_ROUTE_OUT_HOST_BTSCO 0x0800
#define AUDIO_ROUTE_OUT_HOST_DAC 0x1000
#define AUDIO_ROUTE_OUT_HOST_I2S 0x2000
#define AUDIO_ROUTE_OUT_DAC_I2S 0x4000
#define AUDIO_ROUTE_OUT_RESERVED_2 0x8000

#define MAX_AUDIO_SINGLE_ROUTE_OUT 6
#define MAX_AUDIO_MULTI_ROUTE_OUT 16

#define AUDIO_ROUTE_SF_8K 0x00
#define AUDIO_ROUTE_SF_16K 0x01
#define AUDIO_ROUTE_SF_32K 0x02
#define AUDIO_ROUTE_SF_44_1K 0x03
#define AUDIO_ROUTE_SF_48K 0x04
#define AUDIO_ROUTE_SF_11K 0x05
#define AUDIO_ROUTE_SF_12K 0x06
#define AUDIO_ROUTE_SF_22K 0x07
#define AUDIO_ROUTE_SF_24K 0x08
#define AUDIO_ROUTE_SF_NA 0xFF

#define AUDIO_ROUTE_EQ_BASS_BOOST 0x00
#define AUDIO_ROUTE_EQ_CLASSIC 0x01
#define AUDIO_ROUTE_EQ_JAZZ 0x02
#define AUDIO_ROUTE_EQ_LIVE 0x03
#define AUDIO_ROUTE_EQ_NORMAL 0x04
#define AUDIO_ROUTE_EQ_ROCK 0x05
#define AUDIO_ROUTE_EQ_BYPASS 0x06

#define AUDIO_ROUTE_DIGITAL_VOLUME_CONTROL 0x07

#define AUDIO_ROUTE_EQ_CONFIG_GAIN 0xFF /* Custion Gain Config */

#endif