// Copyright © 2024 Shiomachi Software. All rights reserved.
#ifndef UART_H
#define UART_H

#include "Common.h"

#pragma pack(1)

// [構造体]
// UART通信設定
typedef struct _ST_UART_CONFIG {
    ULONG baudrate; // ボーレート
    UCHAR dataBits; // データビット長
    UCHAR stopBits; // ストップビット長
    UCHAR parity;   // UART_PARITY_NONE, UART_PARITY_EVEN, UART_PARITY_ODD
} ST_UART_CONFIG;

#pragma pack()

// [関数プロトタイプ宣言]
void UART_Main();
void UART_SetDefault(ST_UART_CONFIG *pstConfig);
void UART_Init(ST_UART_CONFIG *pstConfig);

#endif