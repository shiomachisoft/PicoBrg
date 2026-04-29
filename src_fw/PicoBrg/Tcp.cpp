// Copyright © 2024 Shiomachi Software. All rights reserved.
#include "Common.h"

// [define] / [マクロ定義]
#define TCP_CONNECT_AP_INTERVAL 100000ULL   // us 100ms When AP connection failure is confirmed, wait this time before returning phase to E_TCP_PHASE_WIFI_INITED / us 100ms APとの接続失敗が確定した場合、この時間を待ってからフェーズをE_TCP_PHASE_WIFI_INITEDに戻す
#define TCP_CONNECT_AP_TIMEOUT  10000000ULL // us 10s If connection fails after this time during AP connection attempt, return phase to E_TCP_PHASE_WIFI_INITED / us 10秒 APとの接続試行中、この時間が経過しても接続できない場合、フェーズをE_TCP_PHASE_WIFI_INITEDに戻す
#define TCP_CONNECT_AP_KEEP     5000000ULL  // us 5s Consider successful if AP connection is maintained during this time *Workaround for WiFi.status() returning WL_CONNECTED immediately after WiFi.begin / us 5秒 この時間の間、APとの接続が維持できていれば、接続成功とみなす ※WiFi.begin直後に、実際にはまだ接続していないにも関わらずWiFi.status()がWL_CONNECTEDを返す問題への対策。
#define TCP_CLIENT_CONNECT_TIMEOUT 2 // seconds Connection timeout when Pico is TCP client / 秒 PicoがTCPソケット通信のクライアントの場合:コネクトのタイムアウト
#define TCP_CLIENT_CONNECT_INTERVAL 5000000ULL // us 5s Connection interval when Pico is TCP client / us 5秒 PicoがTCPソケット通信のクライアントの場合:コネクトのインターバル

// File scope variables / [ファイルスコープ変数]
static WiFiClient f_tcpClient;           // TCP client / TCPクライアント
static WiFiServer f_tcpServer(TCP_PORT); // TCP server / TCPサーバー
static volatile E_TCP_PHASE f_ePhase = E_TCP_PHASE_WIFI_NOT_INIT; // Phase / フェーズ
static volatile uint64_t f_startUsTryConnectToAp = 0;
static volatile uint64_t f_startUsKeepConnectWithAp = 0;
static volatile uint64_t f_startUsTryConnectToServer = 0;
static volatile bool f_isTcpServerBegun = false; // Whether TCP server has started / TCPサーバーを開始済みか否か

// Function prototypes / [関数プロトタイプ宣言]
static void TCP_ConnectToAp(ST_FLASH_DATA* pstFlashData);
static bool TCP_IsApConnectCompleted();
static void TCP_CheckApDisconnect();
static void TCP_Close(bool bResetWifi);
static bool TCP_PeekChar(uint8_t* pData);
static void TCP_OnConnected();

// Main process for WiFi and TCP socket communication / WiFiとTCPソケット通信のメイン処理
void TCP_Main()
{
    UCHAR data;
    ULONG i;
    WiFiClient client;
    ST_FLASH_DATA* pstFlashData = FLASH_GetDataAtPowerOn(); // Get FLASH data at power on / 電源起動時のFLASHデータを取得
    volatile uint64_t currentUs = time_us_64();
    volatile uint64_t diffUs;

    // Check if the connection with AP is lost / APとの接続が切れていないかを確認する
    TCP_CheckApDisconnect();  

    // Phase / フェーズ
    switch (f_ePhase)
    {
        case E_TCP_PHASE_WIFI_INITED: // WiFi initialized / WiFi初期化済み
            // Execute connection to AP / APへの接続を実行
            TCP_ConnectToAp(pstFlashData);
            f_ePhase = E_TCP_PHASE_AP_CONNECTING; // Executing connection process to AP / APへの接続処理を実行中        
            break;
        case E_TCP_PHASE_AP_CONNECTING: // Executing connection process to AP / APへの接続処理を実行中 
            // Check if the connection with AP is successful / APとの接続が成功したかを確認する
            if (TCP_IsApConnectCompleted()) {
                f_startUsTryConnectToServer = 0;
                f_ePhase = E_TCP_PHASE_AP_CONNECTED; // Connected to AP / APに接続済み
            }         
            break;
        case E_TCP_PHASE_AP_CONNECTED: // Connected to AP / APに接続済み
            // Connection processing of TCP socket communication / [TCPソケット通信の接続処理]          
            if (pstFlashData->stNwConfig.isClient) { // When Pico is a TCP client / PicoがTCPクライアントの場合             
                if (!f_tcpClient.connected()) {
                    diffUs = currentUs - f_startUsTryConnectToServer;
                    if (diffUs >= TCP_CLIENT_CONNECT_INTERVAL) {
                        // Try connecting to TCP server / TCPサーバーへの接続を試みる
                        IPAddress ip(pstFlashData->stNwConfig.aServerIpAddr[0],
                                    pstFlashData->stNwConfig.aServerIpAddr[1], 
                                    pstFlashData->stNwConfig.aServerIpAddr[2],
                                    pstFlashData->stNwConfig.aServerIpAddr[3]);           
                        f_tcpClient.setTimeout(TCP_CLIENT_CONNECT_TIMEOUT); // Set timeout for connect() / connect()のタイムアウトを設定
                        f_tcpClient.connect(ip, TCP_PORT); 
                        if (f_tcpClient.connected()) {
                            TCP_OnConnected(); // Common processing upon TCP connection completion / TCP接続完了時の共通処理
                        }
                        else {
                            f_startUsTryConnectToServer = time_us_64();
                        }
                    }
                }         
            }
            else { // When Pico is a TCP server / PicoがTCPサーバーの場合
                // Start TCP server / TCPサーバーを開始
                if (!f_isTcpServerBegun) {
                    f_tcpServer.begin();   
                    f_isTcpServerBegun = true;
                }                    
                // Accept / アクセプト
                client = f_tcpServer.accept();
                if (client) {
                    f_tcpClient = client;  
                    TCP_OnConnected(); // Common processing upon TCP connection completion / TCP接続完了時の共通処理
                } 
            }
            break;
        case E_TCP_PHASE_TCP_CONNECTED: // TCP connected / TCP接続済み
            if (!f_tcpClient.connected()) { // When TCP connection is lost / TCP接続が切れた場合
              TCP_Close(false); // Return the phase to E_TCP_PHASE_AP_CONNECTED / フェーズをE_TCP_PHASE_AP_CONNECTEDに戻す
            }
            else { 
                // Processing when Pico is a TCP server and a connection request comes from a new client / PicoがTCPサーバーの場合で、新しいクライアントから接続要求が来た場合の処理
                // (Recovery when an existing client forcefully disconnects or remains in a half-open state and reconnects) / (既存のクライアントが強制切断・ハーフオープン状態になったまま再接続してきた場合の救済)
                if (!pstFlashData->stNwConfig.isClient) {
                    WiFiClient newClient = f_tcpServer.accept();
                    if (newClient) {
                        f_tcpClient.stop(); // Forcefully discard old connection / 古い接続を強制破棄
                        f_tcpClient = newClient; // Switch to new connection / 新しい接続に乗り換え
                        TCP_OnConnected(); // Common processing upon TCP connection completion / TCP接続完了時の共通処理
                    }
                }

                // Extract TCP received data ⇒ Enqueue UART send data / TCP受信データを取り出し⇒UART送信データをエンキュー   
                for (i = 0; i < CMN_QUE_DATA_MAX_UART_SEND; i++) {
                     // Extract 1 byte of TCP received data / TCP受信データ1byte取り出し
                    if (TCP_PeekChar(&data)) {
                        // Enqueue 1 byte of UART send data / UART送信データ1byteのエンキュー
                        if (!CMN_Enqueue(CMN_QUE_KIND_UART_SEND, &data, true)) {
                            break; // Queue is full (leave data in TCP buffer without extracting it to enable flow control) / キューが満杯(データは抽出せずTCPバッファに残しフロー制御を効かせる)
                        }
                        f_tcpClient.read(); // Enqueue successful, so actually extract from buffer / エンキュー成功したので実際にバッファから抽出
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

// Execute connection to AP / APへの接続を実行
static void TCP_ConnectToAp(ST_FLASH_DATA* pstFlashData)
{
    if (pstFlashData->stNwConfig.szSsid[0] != '\0') {
        // When SSID is not blank / SSIDが空白ではない場合

        // Securely reset the previous connection state (internal state) before trying to connect / 前回の接続状態(内部ステート)を確実にリセットしてから接続を試みる
        WiFi.disconnect();
        // Connect to WPA/WPA2 network / WPA/WPA2ネットワークに接続
        WiFi.begin(pstFlashData->stNwConfig.szSsid, pstFlashData->stNwConfig.szPassword);
    }

    f_startUsTryConnectToAp = time_us_64();   
    f_startUsKeepConnectWithAp = 0; 
}

// Check if the connection with AP is successful / APとの接続が成功したかを確認する
static bool TCP_IsApConnectCompleted() 
{
    int status;
    bool bRet = false;
    volatile uint64_t currentUs;
    volatile uint64_t diffUs;
    volatile uint64_t threshold;

    currentUs = time_us_64();

    status = WiFi.status();
    if (WL_CONNECTED == status) {
        // * Workaround for the issue where WiFi.status() immediately returns WL_CONNECTED right after WiFi.begin (even though it is not actually connected yet). / ※WiFi.begin直後に即座にWiFi.status()が(実際にはまだ接続していないのに)WL_CONNECTEDを返す件の対策。
        threshold = TCP_CONNECT_AP_KEEP; 
        if (0 == f_startUsKeepConnectWithAp) {
            f_startUsKeepConnectWithAp = currentUs;
        }
        diffUs = currentUs - f_startUsKeepConnectWithAp; 
        if (diffUs >= threshold) { 
            bRet = true; // Connection with AP is successful / APとの接続は成功している
        }    
    }  
    else { // Not connected to AP yet / APとまだ接続できていない

        f_startUsKeepConnectWithAp = 0;

        switch (status) {
          // Connection failure confirmed / 接続失敗が確定
          case WL_CONNECT_FAILED:
          case WL_NO_SSID_AVAIL:
          case WL_CONNECTION_LOST:
          //case WL_DISCONNECTED:
          case WL_NO_SHIELD:
            threshold = TCP_CONNECT_AP_INTERVAL;
            break;
          // Trying to connect / 接続を試み中  
          default:
            threshold = TCP_CONNECT_AP_TIMEOUT;
            break;
        }
        diffUs = currentUs - f_startUsTryConnectToAp;    
        if (diffUs >= threshold) {
            TCP_Close(true); // Return the phase to E_TCP_PHASE_WIFI_INITED / フェーズをE_TCP_PHASE_WIFI_INITEDに戻す
        }   
    }

    return bRet;
}

// Check if the connection with AP is lost / APとの接続が切れていないかを確認する
static void TCP_CheckApDisconnect()
{
    int status;

    if (f_ePhase >=  E_TCP_PHASE_AP_CONNECTED) {
        status = WiFi.status();
        if (status != WL_CONNECTED) { // When the connection with AP is lost / APとの接続が切れている場合
            TCP_Close(true); // Return the phase to E_TCP_PHASE_WIFI_INITED / フェーズをE_TCP_PHASE_WIFI_INITEDに戻す
        }
    }
}

// Get whether AP is connected / APと接続済みか否かを取得
bool TCP_IsApConnected()
{
    return (f_ePhase >= E_TCP_PHASE_AP_CONNECTED) ? true : false;
}

// Get whether TCP is connected / TCP接続済みか否かを取得
bool TCP_IsConnected()
{
    return (E_TCP_PHASE_TCP_CONNECTED == f_ePhase) ? true : false;
}

// Common processing upon TCP connection completion / TCP接続完了時の共通処理
static void TCP_OnConnected()
{
    f_tcpClient.setNoDelay(true); // Disable Nagle algorithm / Nagleアルゴリズムを無効化
    f_ePhase = E_TCP_PHASE_TCP_CONNECTED; // TCP connected / TCP接続済み
}

// Disconnect TCP socket / TCPソケットを切断
static void TCP_Close(bool bResetWifi) 
{
    f_tcpClient.stop();

    if (bResetWifi) {
        f_tcpServer.stop();
        f_isTcpServerBegun = false;
        f_ePhase = E_TCP_PHASE_WIFI_INITED; // Return the phase to E_TCP_PHASE_WIFI_INITED / フェーズをE_TCP_PHASE_WIFI_INITEDに戻す
    }
    else {
        f_ePhase = E_TCP_PHASE_AP_CONNECTED; // Return the phase to E_TCP_PHASE_AP_CONNECTED / フェーズをE_TCP_PHASE_AP_CONNECTEDに戻す
        f_startUsTryConnectToServer = time_us_64(); // Update timer to correctly function the interval upon client reconnection / クライアント再接続時のインターバルを正しく機能させるためタイマーを更新
    }
}

// Peek at 1 byte of TCP socket received data (get without extracting) / TCPソケット受信データ1byteを覗き見(抽出せずに取得)
static bool TCP_PeekChar(uint8_t* pData)
{
    int ret = -1;
    bool bRet = false;
    
    if (f_tcpClient.available()) {
        ret = f_tcpClient.peek();
    }

    if (ret >= 0) {
        *pData = (uint8_t)ret;
        bRet = true;
    }

    return bRet;
}

// TCP socket send / TCPソケット送信
int32_t TCP_SendData(uint8_t* buffer, uint32_t size)
{
    size_t ret = 0;

    if ((size > 0) && TCP_IsConnected()) { // When TCP is connected / TCP接続済みの場合
        ret = f_tcpClient.write(buffer, size);
        if (ret == 0 && !f_tcpClient.connected()) {
            TCP_Close(false);
            return -1; // Disconnected, so return failure / 切断されたので失敗を返す
        }
        return (int32_t)ret; // Number of bytes sent (0 indicates temporary buffer full) / 送信できたバイト数(0の場合は一時的なバッファフル)
    }
    else {
        return size; // Discard (treat as success and return full size) / 破棄(成功扱いで全サイズ分を返す)
    }
}

// Initialize WiFi / WiFiを初期化
void TCP_Init(ST_FLASH_DATA* pstFlashData)
{
    WiFi.mode(WIFI_STA);
    WiFi.setHostname(WIFI_HOSTNAME);
    WiFi.noLowPowerMode();

    // Set IP address / IPアドレスを設定
    IPAddress ip(pstFlashData->stNwConfig.aMyIpAddr[0],
                 pstFlashData->stNwConfig.aMyIpAddr[1], 
                 pstFlashData->stNwConfig.aMyIpAddr[2],
                 pstFlashData->stNwConfig.aMyIpAddr[3]); 

    // Default gateway is unnecessary as it's dedicated to local network (specify 0.0.0.0) / ローカルネットワーク専用のため、デフォルトゲートウェイは不要(0.0.0.0を指定)
    IPAddress gateway(0, 0, 0, 0); 
    // Specify typical /24 for subnet mask to communicate only within the same network / 同一ネットワーク内でのみ通信するためサブネットマスクは一般的な /24 を指定
    IPAddress subnet(255, 255, 255, 0); // Typical /24 / 一般的な /24
    
    WiFi.config(ip, gateway, subnet);    
    
    f_ePhase = E_TCP_PHASE_WIFI_INITED; // WiFi initialized / WiFi初期化済み 
}
