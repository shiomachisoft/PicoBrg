// Copyright © 2025 Shiomachi Software. All rights reserved.
#ifndef WIRELESS_H
#define WIRELESS_H

#include "Common.h"

// [define] / [マクロ定義]
#define WL_DEFAULT_IS_WIFI 0 // Default is to use BLE for wireless / デフォルトでは無線はBLEを使用

#pragma pack(1)

// Structures / [構造体]
// Network config / ネットワーク設定
typedef struct _ST_NW_CONFIG2 {
    UCHAR isWifi;                   // Whether to use WiFi (TCP) or BLE for wireless / 無線はWiFi(TCP)とBLEのどちらを使用するか
    char  szCountryCode[3];         // Country code *Unused / カントリーコード ※未使用
    UCHAR aMyIpAddr[4];             // Pico IP address / PicoのIPアドレス
    char  szSsid[33];               // AP SSID / APのSSID
    char  szPassword[65];           // AP password / APのパスワード
    UCHAR aServerIpAddr[4];         // TCP socket communication server IP address / TCPソケット通信のサーバーのIPアドレス
    UCHAR isClient;                 // Whether TCP socket communication client or server / TCPソケット通信のクライアントかサーバーか     
} ST_NW_CONFIG2;

#pragma pack()

// Function prototypes / [関数プロトタイプ宣言]
void WL_Init();
void WL_Main();
void WL_SendMain();
void WL_SetDefault(ST_NW_CONFIG2 *pstConfig);
bool WL_IsConnected();

#endif
