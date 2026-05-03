// Copyright © 2025 Shiomachi Software. All rights reserved.
#ifndef BLE_H
#define BLE_H

#include "Common.h"

// Structures / [構造体]
// Notification data / 通知データ
typedef struct _ST_NOTIFY_DATA {
    uint8_t *buffer;
    uint16_t size;
} ST_NOTIFY_DATA;

// Function prototypes / [関数プロトタイプ宣言]
void BLE_Init();
void BLE_Main();
bool BLE_RequestNotify(uint8_t* pBuf, uint16_t size);
bool BLE_IsConnected();
bool BLE_IsNotifying();
bool BLE_IsNotifyEnabled();
bool BLE_GetNotifyResultAndClear();
void BLE_ClearNotifyResult();
uint16_t BLE_GetMaxNotifySize();

#endif