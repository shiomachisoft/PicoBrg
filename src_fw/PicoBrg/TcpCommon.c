// Copyright © 2024 Shiomachi Software. All rights reserved.
#include "Common.h"

// [define]
#define TCP_CMN_CONNECT_AP_INTERVAL 100000ULL // us 100ms APとの接続に失敗した場合、この時間を待ってからフェーズをE_TCP_CMN_PHASE_INITEDに戻す
#define TCP_CMN_CONNECT_AP_TIMEOUT  10000000ULL // us 10秒 この時間が経過してもAPと接続できない場合、フェーズをE_TCP_CMN_PHASE_INITEDに戻す

// [グローバル変数変数]
TCP_CMN_T f_state = {0}; 
E_TCP_CMN_PHASE f_ePhase = E_TCP_CMN_PHASE_NOT_INIT; // フェーズ

// [ファイルスコープ変数]
static uint64_t f_startUs = 0;
static UCHAR f_aRecvData[CMN_QUE_DATA_MAX_WL_RECV] = {0}; // 受信バッファ

// TCPのメイン処理
void tcp_cmn_main()
{
    ST_FLASH_DATA* pstFlashData = FLASH_GetDataAtPowerOn(); // 電源起動時のFLASHデータを取得

    if (pstFlashData->stNwConfig.isClient) {
        tcp_client_main();
    }
    else {
        tcp_server_main();
    }
}

// 初期化
int tcp_cmn_init()
{
    int err = -1;
    ST_FLASH_DATA* pstFlashData;

    do {
        // 電源起動時のFLASHをデータを取得
        pstFlashData = FLASH_GetDataAtPowerOn(); 
        // CYW43を初期化
        err = cyw43_arch_init_with_country(CYW43_COUNTRY(pstFlashData->stNwConfig.szCountryCode[0], 
                                                         pstFlashData->stNwConfig.szCountryCode[1], 
                                                         pstFlashData->stNwConfig.szCountryCode[2]
                                                         ));
        if (err != 0) break;

        cyw43_arch_enable_sta_mode();
        netif_set_hostname(netif_default, WIFI_HOSTNAME);

        //err = cyw43_wifi_pm(&cyw43_state, CYW43_PERFORMANCE_PM);
        err = cyw43_wifi_pm(&cyw43_state, CYW43_NONE_PM/*cyw43_pm_value(CYW43_NO_POWERSAVE_MODE, 10, 1, 1, 1)*/);
        if (err != 0) break;
    } while(0);    
    
    return err;
}

bool tcp_cmn_connect_ap_async()
{
    int err;
    bool bRet = false;
    ST_FLASH_DATA* pstFlashData;

    pstFlashData = FLASH_GetDataAtPowerOn(); // 電源起動時のFLASHデータを取得
    err = cyw43_arch_wifi_connect_bssid_async(pstFlashData->stNwConfig.szSsid, 
                                              NULL, 
                                              pstFlashData->stNwConfig.szPassword, 
                                              CYW43_AUTH_WPA2_MIXED_PSK); 
    if (0 == err) {
        f_startUs = time_us_64();
        bRet = true; 
    }

    return bRet;
}

// APとの接続を確認
bool tcp_cmn_check_link_up() // cyw43_arch_wifi_connect_bssid_until()を参考にした
{
    int status;
    uint64_t endUs, diffUs, threshold;
    ST_FLASH_DATA* pstFlashData;
    bool bRet = false;

    status = cyw43_tcpip_link_status(&cyw43_state, CYW43_ITF_STA);
    if (status >= CYW43_LINK_NOIP) { // Connected to wifi, but no IP address        
        pstFlashData = FLASH_GetDataAtPowerOn(); // 電源起動時のFLASHデータを取得
        ip_addr_t ip = IPADDR4_INIT_BYTES(pstFlashData->stNwConfig.aMyIpAddr[0], 
                                          pstFlashData->stNwConfig.aMyIpAddr[1], 
                                          pstFlashData->stNwConfig.aMyIpAddr[2],
                                          pstFlashData->stNwConfig.aMyIpAddr[3]
                                          );
        netif_set_ipaddr(netif_default, &ip);
        //ip_addr_t mask = IPADDR4_INIT_BYTES(255,255,255,0);
        //netif_set_netmask(netif_default, &mask);
        //ip_addr_t gw = IPADDR4_INIT_BYTES(192,168,10,1); 
        //netif_set_gw(netif_default, &gw); 
        bRet = true;    
    }  
    else { // CYW43_LINK_UP以上
        endUs = time_us_64();
        diffUs = endUs - f_startUs;
        if (status < CYW43_LINK_DOWN) { // 接続失敗
            threshold = TCP_CMN_CONNECT_AP_INTERVAL;
        }
        else { // 接続を試み中
            threshold = TCP_CMN_CONNECT_AP_TIMEOUT;
        }

        if (diffUs >= threshold) {
            f_ePhase = E_TCP_CMN_PHASE_INITED;
        }   
    }

    return bRet;
}

void tcp_cmn_check_link_down()
{
    int status;

    if (f_ePhase >= E_TCP_CMN_PHASE_CONNECTED_AP) {
        status = cyw43_tcpip_link_status(&cyw43_state, CYW43_ITF_STA); 
        if (status < CYW43_LINK_NOIP) {
            f_ePhase = E_TCP_CMN_PHASE_INITED;
        }
    }

    if (f_ePhase < E_TCP_CMN_PHASE_TCP_OPENED) {
        tcp_cmn_close(&f_state); 
    }    
}

// APと接続済み否かを取得
bool tcp_cmn_is_link_up()
{
    return (f_ePhase >= E_TCP_CMN_PHASE_TCP_OPENED) ? true : false;
}

err_t tcp_cmn_close(void *arg) 
{
    TCP_CMN_T *state = (TCP_CMN_T*)arg;
    err_t err = ERR_OK;

    if (state->pcb != NULL) {
        tcp_arg(state->pcb, NULL);
        tcp_poll(state->pcb, NULL, 0);
        tcp_sent(state->pcb, NULL);
        tcp_recv(state->pcb, NULL);
        tcp_err(state->pcb, NULL);
        err = tcp_close(state->pcb);
        if (err != ERR_OK) {
            tcp_abort(state->pcb);
            err = ERR_ABRT;
        }
        state->pcb = NULL;
    }
    if (state->server_pcb) {
        tcp_arg(state->server_pcb, NULL);
        tcp_close(state->server_pcb);
        state->server_pcb = NULL;
    }

    return err;
}

err_t tcp_cmn_sent(void *arg, struct tcp_pcb *tpcb, u16_t len) 
{
    return ERR_OK;
}

err_t tcp_cmn_send_data(uint8_t* buffer_sent, uint32_t size)
{
    // this method is callback from lwIP, so cyw43_arch_lwip_begin is not required, however you
    // can use this method to cause an assertion in debug mode, if this method is called when
    // cyw43_arch_lwip_begin IS needed
    cyw43_arch_lwip_check();
    err_t err = tcp_write(f_state.pcb, buffer_sent, size, TCP_WRITE_FLAG_COPY);
    if (err != ERR_OK) {
        (void)tcp_cmn_result(&f_state, -1);
    }
    return err;
}

err_t tcp_cmn_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err) 
{
    ULONG i;
    ULONG copySize;
 
    if (!p) {
        return tcp_cmn_result(arg, -1);
    }
    // this method is callback from lwIP, so cyw43_arch_lwip_begin is not required, however you
    // can use this method to cause an assertion in debug mode, if this method is called when
    // cyw43_arch_lwip_begin IS needed
    cyw43_arch_lwip_check();
    if (p->tot_len > 0) {
        // Receive the buffer
        copySize = p->tot_len;
        if (sizeof(f_aRecvData) < copySize) {
            copySize = sizeof(f_aRecvData);
        }
        pbuf_copy_partial(p, f_aRecvData, copySize, 0);
        for (i = 0; i < copySize; i++) {
            // 無線受信データ1byteのエンキュー
            if (!CMN_Enqueue(CMN_QUE_KIND_WL_RECV, &f_aRecvData[i], sizeof(UCHAR), true)) { 
                break; // キューが満杯
            }
        }
        tcp_recved(tpcb, p->tot_len);
    }
    pbuf_free(p);

    return ERR_OK;
}

// 一定時間、データの送信も受信もない時に呼ばれる
err_t tcp_cmn_poll(void *arg, struct tcp_pcb *tpcb) 
{
    return tcp_cmn_result(arg, ERR_OK);
}

void tcp_cmn_err(void *arg, err_t err) {
    if (err != ERR_ABRT) {
        tcp_cmn_result(arg, err);
    }
}

err_t tcp_cmn_result(void *arg, int status) 
{
    TCP_CMN_T *state = (TCP_CMN_T*)arg;

    //return tcp_server_close(arg);
    // ↓status = ERR_ABRTの場合、tcp_server_close()を実行しない理由:
    // tcp_server_close()の中でtcp_close()がERR_OK以外を返した時にERR_ABRTがセットされる。
    // つまりERR_ABRTということは既にtcp_server_close()を実行済み。
    if ((status != ERR_OK) && (status != ERR_ABRT)) {
        tcp_cmn_close(state); 
        f_ePhase = E_TCP_CMN_PHASE_INITED;  
    }
    return ERR_OK;
}

// TCP接続済みか否かを取得
bool tcp_cmn_is_connected()
{
    return (E_TCP_CMN_PHASE_TCP_CONNECTED == f_ePhase) ? true : false;
}

// ST_NW_CONFIG2構造体にデフォルト値を格納
void tcp_cmn_set_default(ST_NW_CONFIG2 *pstConfig)
{
    strcpy(pstConfig->szCountryCode, TCP_CMN_DEFAULT_COUNTRY_CODE);
    pstConfig->aMyIpAddr[0] = (TCP_CMN_DEFAULT_MY_IP_ADDR >> 24) & 0xFF;
    pstConfig->aMyIpAddr[1] = (TCP_CMN_DEFAULT_MY_IP_ADDR >> 16) & 0xFF;
    pstConfig->aMyIpAddr[2] = (TCP_CMN_DEFAULT_MY_IP_ADDR >> 8) & 0xFF;
    pstConfig->aMyIpAddr[3] = (TCP_CMN_DEFAULT_MY_IP_ADDR) & 0xFF;
    memset(pstConfig->szSsid, 0, sizeof(pstConfig->szSsid));
    memset(pstConfig->szPassword, 0, sizeof(pstConfig->szPassword));

    pstConfig->aRemoteIpAddr[0] = (TCP_CMN_DEFAULT_REMOTE_IP_ADDR >> 24) & 0xFF;
    pstConfig->aRemoteIpAddr[1] = (TCP_CMN_DEFAULT_REMOTE_IP_ADDR >> 16) & 0xFF;
    pstConfig->aRemoteIpAddr[2] = (TCP_CMN_DEFAULT_REMOTE_IP_ADDR >> 8) & 0xFF;
    pstConfig->aRemoteIpAddr[3] = (TCP_CMN_DEFAULT_REMOTE_IP_ADDR) & 0xFF;
    pstConfig->isClient = TCP_CMN_DEFAULT_IS_CLIENT;    
}

