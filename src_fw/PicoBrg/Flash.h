// Copyright © 2024 Shiomachi Software. All rights reserved.
#ifndef FLASH_H
#define FLASH_H

#include "Common.h"

#pragma pack(1)

// [構造体]
// FLASHデータ
typedef struct _ST_FLASH_DATA {
    char           szFwName[FW_NAME_BUF_SIZE];  // FW名
    ULONG          fwVer;                       // FWバージョン
    ST_UART_CONFIG stUartConfig;                // UART通信設定
    ST_NW_CONFIG2  stNwConfig;                  // ネットワーク設定
    USHORT         checksum;                    // チェックサム
} ST_FLASH_DATA;

#pragma pack()

// [関数プロトタイプ宣言]
ST_FLASH_DATA* FLASH_GetDataAtPowerOn();
void FLASH_Read(ST_FLASH_DATA *pstFlashData);
void FLASH_Write(ST_FLASH_DATA *pstFlashData);
void FLASH_Erase();
void FLASH_Init();

#endif