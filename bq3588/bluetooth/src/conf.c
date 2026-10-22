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
 *  Filename:      conf.c
 *
 *  Description:   Contains functions to conduct run-time module configuration
 *                 based on entries present in the .conf file
 *
 ******************************************************************************/

#define LOG_TAG "bt_vnd_conf"

#include <string.h>
#include <stdio.h>
#include <utils/Log.h>
#include "bt_vendor_brcm.h"

/******************************************************************************
**  Externs
******************************************************************************/
int UserialSetPort(char *pConfName, char *pConfValue, int param);
int HwSetPatchFilePath(char *pConfName, char *pConfValue, int param);
int HwSetPatchFileName(char *pConfName, char *pConfValue, int param);
#if (VENDOR_LIB_RUNTIME_TUNING_ENABLED == TRUE)
int HwSetPatchSettlementDelay(char *pConfName, char *pConfValue, int param);
#endif

/******************************************************************************
**  Local type definitions
******************************************************************************/

#define CONF_COMMENT '#'
#define CONF_DELIMITERS " =\n\r\t"
#define CONF_VALUES_DELIMITERS "=\n\r\t"
#define CONF_MAX_LINE_LEN 255

typedef int(ConfActionT)(char *pConfName, char *pConfValue, int param);

typedef struct {
    const char *confEntry;
    ConfActionT *pAction;
    int param;
} ConfEntryT;

/******************************************************************************
**  Static variables
******************************************************************************/

/*
 * Current supported entries and corresponding action functions
 */
static const ConfEntryT CONF_TABLE[] = {
    {"UartPort", UserialSetPort, 0},
    {"FwPatchFilePath", HwSetPatchFilePath, 0},
    {"FwPatchFileName", HwSetPatchFileName, 0},
#if (VENDOR_LIB_RUNTIME_TUNING_ENABLED == TRUE)
    {"FwPatchSettlementDelay", HwSetPatchSettlementDelay, 0},
#endif
    {(const char *)NULL, NULL, 0}
};

/*****************************************************************************
**   CONF INTERFACE FUNCTIONS
*****************************************************************************/

/*******************************************************************************
**
** Function        VndLoadConf
**
** Description     Read conf entry from pPath file one by one and call
**                 the corresponding config function
**
** Returns         None
**
*******************************************************************************/
void VndLoadConf(const char *pPath)
{
    FILE *pFile;
    char *pName;
    char *pValue;
    ConfEntryT *pEntry;
    char line[CONF_MAX_LINE_LEN + 1]; /* add 1 for \0 char */

    HILOGI("Attempt to load conf from %s", pPath);
    if ((pFile = fopen(pPath, "r")) == NULL) {
        HILOGI("VndLoadConf file >%s< not found", pPath);
        return;
    }

    /* read line by line */
    while (fgets(line, CONF_MAX_LINE_LEN + 1, pFile) != NULL) {
        if (line[0] == CONF_COMMENT) {
            continue;
        }

        pName = strtok(line, CONF_DELIMITERS);
        if (pName == NULL) {
            continue;
        }

        pValue = strtok(NULL, CONF_DELIMITERS);
        if (pValue == NULL) {
            HILOGW("VndLoadConf: missing value for name: %s", pName);
            continue;
        }

        pEntry = (ConfEntryT *)CONF_TABLE;
        while (pEntry->confEntry != NULL) {
            if (strcmp(pEntry->confEntry, (const char *)pName) == 0) {
                pEntry->pAction(pName, pValue, pEntry->param);
                break;
            }

            pEntry++;
        }
    }

    (void)fclose(pFile);
}
