// Copyright © 2024 Shiomachi Software. All rights reserved.
#ifndef TCP_CMN_H
#define TCP_CMN_H

// [define]
#define WIFI_HOSTNAME FW_NAME // hostname
#define TCP_PORT 7777 // ソケットポート番号

#define TCP_CMN_POLL_TIME_S 5
#define TCP_CMN_DRIVER_WORK_TIMEOUT 1000 //ms

// デフォルト値
#define TCP_CMN_DEFAULT_COUNTRY_CODE    "JP"        // カントリーコードのデフォルト値
#define TCP_CMN_DEFAULT_MY_IP_ADDR      0xC0A80A64  // 自分のIPアドレスのデフォルト値
#define TCP_CMN_DEFAULT_REMOTE_IP_ADDR  0xC0A80AC8  // 相手のIPアドレスのデフォルト値
#define TCP_CMN_DEFAULT_IS_CLIENT       0           // 自分がTCPクライアントか否かのデフォルト値

// [列挙体]
// クライアントのフェーズの種類
typedef enum _E_TCP_CMN_PHASE {
    E_TCP_CMN_PHASE_NOT_INIT,        // 未初期化
    E_TCP_CMN_PHASE_INITED,          // 初期化済み
    E_TCP_CMN_PHASE_CONNECTING_AP,   // APへの接続処理を実行中
    E_TCP_CMN_PHASE_CONNECTED_AP,    // APに接続済み
    E_TCP_CMN_PHASE_TCP_OPENED,      // TCPオープン済み
    E_TCP_CMN_PHASE_TCP_CONNECTED    // TCP接続済み
} E_TCP_CMN_PHASE;

#pragma pack(1)

// [構造体]
typedef struct TCP_CMN_T_ {
    struct tcp_pcb *pcb;
    struct tcp_pcb *server_pcb;
    ip_addr_t server_addr;
} TCP_CMN_T;

// ネットワーク設定
typedef struct _ST_NW_CONFIG2 {
    char  szCountryCode[3]; // カントリーコード
    UCHAR aMyIpAddr[4];     // 自分のIPアドレス
    char  szSsid[33];       // APのSSID
    char  szPassword[65];   // APのパスワード

    UCHAR aRemoteIpAddr[4]; // 相手のIPアドレス
    UCHAR isClient;         // 自分がTCPクライアントか否か
} ST_NW_CONFIG2;

#pragma pack()

// [グローバル変数]
extern TCP_CMN_T f_state; 
extern E_TCP_CMN_PHASE f_ePhase;

// [関数プロトタイプ宣言]
int tcp_cmn_init();
bool tcp_cmn_check_link_up();
void tcp_cmn_check_link_down();
bool tcp_cmn_connect_ap_async();
bool tcp_cmn_is_link_up();
err_t tcp_cmn_close(void *arg);
err_t tcp_cmn_result(void *arg, int status);
err_t tcp_cmn_sent(void *arg, struct tcp_pcb *tpcb, u16_t len);
err_t tcp_cmn_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err);
err_t tcp_cmn_poll(void *arg, struct tcp_pcb *tpcb);
void tcp_cmn_err(void *arg, err_t err);
err_t tcp_cmn_send_data(uint8_t* buffer_sent, uint32_t size);
void tcp_cmn_set_default(ST_NW_CONFIG2 *pstConfig);
bool tcp_cmn_is_connected();
void tcp_client_main();
void tcp_server_main();
void tcp_cmn_main();

#endif