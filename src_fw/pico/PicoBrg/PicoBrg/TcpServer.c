#include "Common.h"

// [関数プロトタイプ宣言]
static err_t tcp_server_accept(void *arg, struct tcp_pcb *pcb, err_t err);
static bool tcp_server_open(void *arg);

// TCPサーバーのメイン処理
void tcp_server_main()
{   
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
            if (true == tcp_server_open(&f_state)) {
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

static bool tcp_server_open(void *arg) {
    bool bRet = false;
    TCP_CMN_T *state = (TCP_CMN_T*)arg;

    struct tcp_pcb *pcb = tcp_new_ip_type(IPADDR_TYPE_ANY);
    if (!pcb) {
        goto end;
    }

    err_t err = tcp_bind(pcb, NULL, TCP_PORT);
    if (err) {
        goto end;
    }

    state->server_pcb = tcp_listen_with_backlog(pcb, 1);
    if (!state->server_pcb) {
        if (pcb) {
            tcp_close(pcb);
        }
        goto end;
    }

    tcp_arg(state->server_pcb, state);
    tcp_accept(state->server_pcb, tcp_server_accept);
    
    bRet = true;

end:
    return bRet;    
}

static err_t tcp_server_accept(void *arg, struct tcp_pcb *pcb, err_t err) {
    TCP_CMN_T *state = (TCP_CMN_T*)arg;
    if (err != ERR_OK || pcb == NULL) {    
        tcp_cmn_result(arg, err);     
        return ERR_VAL;
    }

    state->pcb = pcb;
    tcp_arg(pcb, state);
    tcp_sent(pcb, tcp_cmn_sent);
    tcp_recv(pcb, tcp_cmn_recv);
    // ↓これを有効にすると、なぜか、tcp_cmn_send_data()を実行してからtcp_cmn_sent()が呼ばれるまでに何秒もかかる
    //tcp_poll(pcb, tcp_cmn_poll, TCP_CMN_POLL_TIME_S * 2);
    tcp_err(pcb, tcp_cmn_err);

    f_ePhase = E_TCP_CMN_PHASE_TCP_CONNECTED; // TCP接続済み
    return ERR_OK;
}

