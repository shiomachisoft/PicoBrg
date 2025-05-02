// Copyright © 2024 Shiomachi Software. All rights reserved.
#include "Common.h"

// [define]
#define TCP_CONNECT_AP_INTERVAL 100000ULL   // us 100ms APとの接続失敗が確定した場合、この時間を待ってからフェーズをE_TCP_PHASE_WIFI_INITEDに戻す
#define TCP_CONNECT_AP_TIMEOUT  10000000ULL // us 10秒 APとの接続試行中、この時間が経過しても接続できない場合、フェーズをE_TCP_PHASE_WIFI_INITEDに戻す
#define TCP_CONNECT_AP_KEEP     5000000ULL  // us 5秒 この時間の間、APとの接続が維持できていれば、接続成功とみなす ※なぜかWiFi.begin直後にノータイムでWiFi.status()が(実際にはまだ接続していないのに)WL_CONNECTEDを返す件の対策。
#define TCP_CLIENT_CONNECT_TIMEOUT 2 // 秒 PicoがTCPソケット通信のクライアント場合:コネクトのタイムアウト
#define TCP_CLIENT_CONNECT_INTERVAL 5000000ULL // us 5秒 PicoがTCPソケット通信のクライアント場合:コネクトのインターバル

// [ファイルスコープ変数]
static WiFiClient f_tcpClient;           // TCPクライアント
static WiFiServer f_tcpServer(TCP_PORT); // TCPサーバー
static E_TCP_PHASE f_ePhase = E_TCP_PHASE_WIFI_NOT_INIT; // フェーズ
static uint64_t f_startUs_tryConnectToAp = 0;
static uint64_t f_startUs_keepConnectWithAp = 0;
static uint64_t f_startUs_tryConnectToServer = 0;
static bool f_isTcpServerBegan = false; // TCPサーバーを開始済みか否か

// [関数プロトタイプ宣言]
static void tcp_wifi_init(ST_FLASH_DATA* pstFlashData);
static void tcp_wifi_try_connect_ap(ST_FLASH_DATA* pstFlashData);
static bool tcp_is_ap_connect_completed();
static void tcp_is_ap_changed_to_disconnect();
static void tcp_close(bool bInited);
static bool tcp_is_connected();
static bool tcp_get_char(uint8_t* pData);

// WiFiとTCPソケット通信のメイン処理
void tcp_main()
{
    UCHAR data;
    ULONG i;
    WiFiClient client;
    ST_FLASH_DATA* pstFlashData = FLASH_GetDataAtPowerOn(); // 電源起動時のFLASHデータを取得
    uint64_t currentUs = time_us_64();
    uint64_t diffUs;

    // APとの接続が切れていなかを確認する
    tcp_is_ap_changed_to_disconnect();  

    // フェーズ
    switch (f_ePhase)
    {
        case E_TCP_PHASE_WIFI_NOT_INIT: // WiFi未初期化 
            // Wifiを初期化 
            tcp_wifi_init(pstFlashData);
            f_ePhase = E_TCP_PHASE_WIFI_INITED; // WiFi初期化済み 
            break;
        case E_TCP_PHASE_WIFI_INITED: // WiFi初期化済み
            // APへの接続を実行
            tcp_wifi_try_connect_ap(pstFlashData);
            f_ePhase = E_TCP_PHASE_AP_CONNECTING; // APへの接続処理を実行中        
            break;
        case E_TCP_PHASE_AP_CONNECTING: // APへの接続処理を実行中 
            // APとの接続が成功したかを確認する
            if (tcp_is_ap_connect_completed()) {
                f_startUs_tryConnectToServer = 0;
                f_ePhase = E_TCP_PHASE_AP_CONNECTED; // APに接続済み
            }         
            break;
        case E_TCP_PHASE_AP_CONNECTED: // APに接続済み
            // [TCPソケット通信の接続処理]          
            if (pstFlashData->stNwConfig.isClient) { // PicoがTCPクライアントの場合             
                if (!f_tcpClient.connected()) {
                    diffUs = currentUs - f_startUs_tryConnectToServer;
                    if (diffUs >= TCP_CLIENT_CONNECT_INTERVAL) {
                        // TCPサーバーへの接続を試みる
                        IPAddress ip(pstFlashData->stNwConfig.aServerIpAddr[0],
                                    pstFlashData->stNwConfig.aServerIpAddr[1], 
                                    pstFlashData->stNwConfig.aServerIpAddr[2],
                                    pstFlashData->stNwConfig.aServerIpAddr[3]);           
                        f_tcpClient.setTimeout(TCP_CLIENT_CONNECT_TIMEOUT); // connect()のタイムアウトを設定
                        f_tcpClient.connect(ip, TCP_PORT); 
                        if (f_tcpClient.connected()) {
                            f_ePhase = E_TCP_PHASE_TCP_CONNECTED; // TCP接続済み
                        }
                        else {
                            f_startUs_tryConnectToServer = time_us_64();
                        }
                    }
                }         
            }
            else { // PicoがTCPサーバーの場合
                // TCPサーバーを開始
                if (!f_isTcpServerBegan) {
                    f_tcpServer.begin();   
                    f_isTcpServerBegan = true;
                }                    
                // アクセプト
                client = f_tcpServer.accept();
                if (client) {
                    f_tcpClient = client;  
                    f_ePhase = E_TCP_PHASE_TCP_CONNECTED; // TCP接続済み          
                } 
            }
            break;
        case E_TCP_PHASE_TCP_CONNECTED: // TCP接続済み
            if (!f_tcpClient.connected()) { // TCP接続が切れた場合
              tcp_close(false); // フェーズをE_TCP_PHASE_AP_CONNECTEDに戻す
            }
            else { 
                // TCP受信データを取り出し⇒UART送信データをエンキュー   
                for (i = 0; i < CMN_QUE_DATA_MAX_UART_SEND; i++) {
                     // TCP受信データ1byte取り出し
                    if (tcp_get_char(&data)) {
                        // UART送信データ1byteのエンキュー
                        if (!CMN_Enqueue(CMN_QUE_KIND_UART_SEND, &data, sizeof(UCHAR), true)) {
                            break; // キューが満杯
                        }
                    }
                    else {
                        break;
                    }
                }    
            }
            break;
        default:
            break;
    }   
}

// APへの接続を実行
static void tcp_wifi_try_connect_ap(ST_FLASH_DATA* pstFlashData)
{
    const char szBlank[33] = {0}; 

    if (memcmp(pstFlashData->stNwConfig.szSsid, szBlank, sizeof(szBlank))) {
        // SSIDが空白ではない場合

        // Connect to WPA/WPA2 network.
        WiFi.begin(pstFlashData->stNwConfig.szSsid, pstFlashData->stNwConfig.szPassword);
        f_startUs_tryConnectToAp = time_us_64();   
        f_startUs_keepConnectWithAp = 0; 
    }
}

// APとの接続が成功したかを確認する
static bool tcp_is_ap_connect_completed() 
{
    int status;
    bool bRet = false;
    uint64_t currentUs;
    uint64_t diffUs;
    uint64_t threshold;

    currentUs = time_us_64();

    status = WiFi.status();
    if (WL_CONNECTED == status) {
        // ※なぜかWiFi.begin直後にノータイムでWiFi.status()が(実際にはまだ接続していないのに)WL_CONNECTEDを返す件の対策。
        threshold = TCP_CONNECT_AP_KEEP; 
        if (0 == f_startUs_keepConnectWithAp) {
            f_startUs_keepConnectWithAp = currentUs;
        }
        diffUs = currentUs - f_startUs_keepConnectWithAp; 
        if (diffUs >= threshold) { 
            bRet = true; // APとの接続は成功している
        }    
    }  
    else { // APとまだ接続できていない

        f_startUs_keepConnectWithAp = 0;

        switch (status) {
          // 接続失敗が確定
          case WL_CONNECT_FAILED:
          case WL_NO_SSID_AVAIL:
          case WL_CONNECTION_LOST:
          case WL_DISCONNECTED:
          case WL_NO_SHIELD:
            threshold = TCP_CONNECT_AP_INTERVAL;
            break;
          // 接続を試み中  
          default:
            threshold = TCP_CONNECT_AP_TIMEOUT;
            break;
        }
        diffUs = currentUs - f_startUs_tryConnectToAp;    
        if (diffUs >= threshold) {
            tcp_close(true); // フェーズをE_TCP_PHASE_AP_INITEDに戻す
        }   
    }

    return bRet;
}

// APと接続済み否かを取得
bool tcp_is_ap_connected()
{
    return (f_ePhase >= E_TCP_PHASE_AP_CONNECTED) ? true : false;
}

// APとの接続が切れていなかを確認する
static void tcp_is_ap_changed_to_disconnect()
{
    int status;

    if (f_ePhase >=  E_TCP_PHASE_AP_CONNECTED) {
        status = WiFi.status();
        if (status != WL_CONNECTED) { // APとの接続が切れている場合
            tcp_close(true); // フェーズをE_TCP_PHASE_WIFI_INITEDに戻す
        }
    }
}

// TCP接続済みか否かを取得
static bool tcp_is_connected()
{
    return (E_TCP_PHASE_TCP_CONNECTED == f_ePhase) ? true : false;
}

// TCPソケットを切断
static void tcp_close(bool bInited) 
{
    f_tcpClient.stop();

    f_tcpServer.stop();
    f_isTcpServerBegan = false;

    if (bInited) {
        f_ePhase = E_TCP_PHASE_WIFI_INITED; // フェーズをE_TCP_PHASE_WIFI_INITEDに戻す
    }
    else {
        f_ePhase = E_TCP_PHASE_AP_CONNECTED; // フェーズをE_TCP_PHASE_AP_CONNECTEDに戻す
    }
}

// TCPソケット受信データ1byte取り出し
static bool tcp_get_char(uint8_t* pData)
{
    int ret = -1;
    bool bRet = false;
    
    if (f_tcpClient.available()) {
        ret = f_tcpClient.read();
    }

    if (ret > 0) {
        *pData = (uint8_t)ret;
        bRet = true;
    }

    return bRet;
}

// TCPソケット送信
bool tcp_send_data(uint8_t* buffer, uint32_t size)
{
    bool bRet = true;
    size_t ret = 0;

    if ((size > 0) && tcp_is_connected()) { // TCP接続済みの場合
        ret = f_tcpClient.write(buffer, size);
        if (ret <= 0) {            
            tcp_close(false);
            bRet = false; // 本関数は失敗を返す   
        }
        else {
            // 本関数は成功を返す           
        }
    }
    else {
        // 破棄(本関数は成功を返す)
    }
        
    return bRet;
}

// WiFiを初期化
static void tcp_wifi_init(ST_FLASH_DATA* pstFlashData)
{
    WiFi.mode(WIFI_STA);
    WiFi.setHostname(WIFI_HOSTNAME);
    WiFi.noLowPowerMode();

    // IPアドレスを設定
    IPAddress ip(pstFlashData->stNwConfig.aMyIpAddr[0],
                 pstFlashData->stNwConfig.aMyIpAddr[1], 
                 pstFlashData->stNwConfig.aMyIpAddr[2],
                 pstFlashData->stNwConfig.aMyIpAddr[3]); 
    WiFi.config(ip);                                                
}
