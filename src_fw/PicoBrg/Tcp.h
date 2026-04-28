// Copyright © 2024 Shiomachi Software. All rights reserved.
#ifndef TCP_H
#define TCP_H

#include "Common.h"

// [define] / [マクロ定義]
#define WIFI_HOSTNAME FW_NAME // hostname / ホスト名

// Default values / デフォルト値
#define WIFI_DEFAULT_COUNTRY_CODE   "XX"        // Default country code *Cannot be set in Arduino WiFi library / カントリーコードのデフォルト値 ※ArduinoのWiFiライブラリではカントリーコードは設定できない
#define WIFI_DEFAULT_MY_IP_ADDR     0xC0A80A64  // Default Pico IP address / PicoのIPアドレスのデフォルト値

#define TCP_DEFAULT_IS_CLIENT   0               // TCP client or server. Default is server. / TCPソケット通信のクライアントかサーバーか。デフォルトはサーバー。
#define TCP_DEFAULT_REMOTE_IP_ADDR  0xC0A80AC8  // Default remote IP address for TCP socket / TCPソケット通信のサーバーのIPアドレスのデフォルト値
#define TCP_PORT 7777                           // Socket port number / ソケットポート番号

// Enumerations / [列挙体]
// Phase / フェーズ
typedef enum _E_TCP_PHASE {
    E_TCP_PHASE_WIFI_NOT_INIT,   // WiFi not initialized / WiFi未初期化
    E_TCP_PHASE_WIFI_INITED,     // WiFi initialized / WiFi初期化済み
    E_TCP_PHASE_AP_CONNECTING,   // Connecting to AP / APへの接続処理を実行中
    E_TCP_PHASE_AP_CONNECTED,    // Connected to AP / APに接続済み
    E_TCP_PHASE_TCP_CONNECTED    // TCP connected / TCP接続済み
} E_TCP_PHASE;

// Function prototypes / [関数プロトタイプ宣言]
bool TCP_IsApConnected();
bool TCP_IsConnected();
int32_t TCP_SendData(uint8_t* buffer, uint32_t size);
void TCP_Main();
void TCP_Init(ST_FLASH_DATA* pstFlashData);

#endif