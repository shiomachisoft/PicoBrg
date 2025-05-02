// Copyright © 2025 Shiomachi Software. All rights reserved.
#ifndef WIRELESS_H
#define WIRELESS_H

// [define]
#define WL_DEFAULT_IS_WIFI 0 // デフォルトでは無線はBLEを使用

#pragma pack(1)

// [構造体]
// ネットワーク設定
typedef struct _ST_NW_CONFIG2 {
    UCHAR isWifi;                   // 無線はWiFi(TCP)とBLEのどちらを使用するか
    char  szCountryCode[3];         // カントリーコード ※未使用
    UCHAR aMyIpAddr[4];             // PicoのIPアドレス
    char  szSsid[33];               // APのSSID
    char  szPassword[65];           // APのパスワード
    UCHAR aServerIpAddr[4];         // TCPソケット通信のサーバーのIPアドレス
    UCHAR isClient;                 // TCPソケット通信のクライアントかサーバーか     
} ST_NW_CONFIG2;

#pragma pack()

// [関数プロトタイプ宣言]
void WL_Main();
void WL_SendMain();
void WL_ReqToSend(PVOID pBuf, ULONG size);
void WL_SetDefault(ST_NW_CONFIG2 *pstConfig);

#endif
