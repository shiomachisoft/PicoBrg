// Copyright © 2025 Shiomachi Software. All rights reserved.
#ifndef BLE_H
#define BLE_H

#pragma pack(1)

// [構造体]
// 通知データ
typedef struct _ST_NOTIFY_DATA {
    uint8_t *buffer;
    uint16_t size;
} ST_NOTIFY_DATA;

#pragma pack()

// [関数プロトタイプ宣言]
void BLE_Init();
void BLE_Main();
bool BLE_ReqToNotify(uint8_t* pBuf, uint16_t size);
bool BLE_IsConnected();
bool BLE_IsNotifying();

#endif