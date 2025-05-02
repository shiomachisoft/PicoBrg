// Copyright © 2024 Shiomachi Software. All rights reserved.
#ifndef TCP_H
#define TCP_H

// [define]
#define WIFI_HOSTNAME FW_NAME // hostname

// デフォルト値
#define WIFI_DEFAULT_COUNTRY_CODE   "JP"        // カントリーコードのデフォルト値 ※ArduinoのWifiライブラリではカントリーコードは設定できない
#define WIFI_DEFAULT_MY_IP_ADDR     0xC0A80A64  // PicoのIPアドレスのデフォルト値

#define TCP_DEFAULT_IS_CLIENT   0               // TCPソケット通信のクライアントかサーバーか。デフォルトはサーバー。
#define TCP_DEFAULT_REMOTE_IP_ADDR  0xC0A80AC8  // TCPソケット通信のサーバーのIPアドレスのデフォルト値
#define TCP_PORT 7777                           // ソケットポート番号

// [列挙体]
// フェーズ
typedef enum _E_TCP_PHASE {
    E_TCP_PHASE_WIFI_NOT_INIT,   // WiFi未初期化
    E_TCP_PHASE_WIFI_INITED,     // 初期化済み
    E_TCP_PHASE_AP_CONNECTING,   // APへの接続処理を実行中
    E_TCP_PHASE_AP_CONNECTED,    // APに接続済み
    E_TCP_PHASE_TCP_CONNECTED    // TCP接続済み
} E_TCP_PHASE;

// [関数プロトタイプ宣言]
bool tcp_is_ap_connected();
bool tcp_send_data(uint8_t* buffer, uint32_t size);
void tcp_main();

#endif