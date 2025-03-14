// Copyright © 2024 Shiomachi Software. All rights reserved.
#include "Common.h"

// [define]
#define TCP_CLIENT_CONNECT_TIMEOUT 3000000ULL // us 3秒 この時間が経過してもTCP接続できない場合、フェーズをE_TCP_CMN_PHASE_INITEDに戻す

// [ファイルスコープ変数]
static uint64_t f_startUs = 0;

// [関数プロトタイプ宣言]
static bool tcp_client_open(void *arg);
static err_t tcp_client_connected(void *arg, struct tcp_pcb *tpcb, err_t err);
static void tcp_client_check_connect_timeout();

// TCPクライアントのメイン処理
void tcp_client_main()
{   
    // TCP接続タイムアウトの場合、TCPクローズを行う
    tcp_client_check_connect_timeout();

    // APとの接続が切れていなかを確認する
    tcp_cmn_check_link_down();  

    // フェーズ
    switch (f_ePhase)
    {
        case E_TCP_CMN_PHASE_NOT_INIT: // 未初期化  
            if (0 == tcp_cmn_init()) {
                f_ePhase = E_TCP_CMN_PHASE_INITED; // 初期化済み
            }        
            break;
        case E_TCP_CMN_PHASE_INITED: // 初期化済み
            if (true == tcp_cmn_connect_ap_async()) {
                f_ePhase = E_TCP_CMN_PHASE_CONNECTING_AP; // APへの接続処理を実行中
            }
            break;
        case E_TCP_CMN_PHASE_CONNECTING_AP: // APへの接続処理を実行中
            if (true == tcp_cmn_check_link_up()) {
                f_ePhase = E_TCP_CMN_PHASE_CONNECTED_AP; // APに接続済み
            }         
            break;
        case E_TCP_CMN_PHASE_CONNECTED_AP: // APに接続済み 
            if (true == tcp_client_open(&f_state)) {
                f_ePhase = E_TCP_CMN_PHASE_TCP_OPENED; // TCPオープン済み
            }        
            break;
        case E_TCP_CMN_PHASE_TCP_OPENED: // TCPオープン済み
        case E_TCP_CMN_PHASE_TCP_CONNECTED: // TCP接続済み
        default:
            // 何もしない
            break;
    }   

    // ポーリング
    if (f_ePhase >= E_TCP_CMN_PHASE_INITED) { // 初期化済みの場合
        cyw43_arch_poll();
        //cyw43_arch_wait_for_work_until(make_timeout_time_ms(TCP_CMN_DRIVER_WORK_TIMEOUT)); 
    } 
}

static bool tcp_client_open(void *arg) 
{
    TCP_CMN_T *state = (TCP_CMN_T*)arg;
    ST_FLASH_DATA* pstFlashData = FLASH_GetDataAtPowerOn(); // 電源起動時のFLASHデータを取得
    // サーバーのIPアドレス
    ip_addr_t ip = IPADDR4_INIT_BYTES(pstFlashData->stNwConfig.aRemoteIpAddr[0], 
                                      pstFlashData->stNwConfig.aRemoteIpAddr[1], 
                                      pstFlashData->stNwConfig.aRemoteIpAddr[2],
                                      pstFlashData->stNwConfig.aRemoteIpAddr[3]
                                      );

    // サーへの接続を開始する
    memcpy(&state->server_addr, &ip, sizeof(ip_addr_t));
    state->pcb = tcp_new_ip_type(IP_GET_TYPE(&state->server_addr));
    if (!state->pcb) {
        return false;
    }

    tcp_arg(state->pcb, state);
    // ↓これを有効にすると、なぜか、tcp_cmn_send_data()を実行してからtcp_cmn_sent()が呼ばれるまでに何秒もかかる
    //tcp_poll(state->pcb, tcp_cmn_poll, TCP_CMN_POLL_TIME_S * 2);
    tcp_sent(state->pcb, tcp_cmn_sent);
    tcp_recv(state->pcb, tcp_cmn_recv);
    tcp_err(state->pcb, tcp_cmn_err);

    // cyw43_arch_lwip_begin/end should be used around calls into lwIP to ensure correct locking.
    // You can omit them if you are in a callback from lwIP. Note that when using pico_cyw_arch_poll
    // these calls are a no-op and can be omitted, but it is a good practice to use them in
    // case you switch the cyw43_arch type later.
    cyw43_arch_lwip_begin();
    err_t err = tcp_connect(state->pcb, &state->server_addr, TCP_PORT, tcp_client_connected);
    cyw43_arch_lwip_end();

    f_startUs = time_us_64();

    return err == ERR_OK;
}

static err_t tcp_client_connected(void *arg, struct tcp_pcb *tpcb, err_t err) 
{
    if (err != ERR_OK) {
        return tcp_cmn_result(arg, err);
    }
    
    f_ePhase = E_TCP_CMN_PHASE_TCP_CONNECTED; // TCP接続済み

    return ERR_OK;
}

static void tcp_client_check_connect_timeout()
{
    uint64_t endUs, diffUs;

    if (E_TCP_CMN_PHASE_TCP_OPENED == f_ePhase) {
        endUs = time_us_64();
        diffUs = endUs - f_startUs;
        if (diffUs >= TCP_CLIENT_CONNECT_TIMEOUT) {
            f_ePhase = E_TCP_CMN_PHASE_CONNECTED_AP;
            tcp_cmn_close(&f_state);
        }
    }    
}