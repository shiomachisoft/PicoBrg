// Copyright © 2025 Shiomachi Software. All rights reserved.
#include "Common.h"

// [define] / [マクロ定義]
// Nordic UART Service (NUS)
#define NUS_SERVICE_UUID    "6E400001-B5A3-F393-E0A9-E50E24DCCA9E" // Service UUID / サービスUUID
#define NUS_RX_CHAR_UUID    "6E400002-B5A3-F393-E0A9-E50E24DCCA9E" // Central -> Peripheral / セントラル -> ペリフェラル
#define NUS_TX_CHAR_UUID    "6E400003-B5A3-F393-E0A9-E50E24DCCA9E" // Peripheral -> Central / ペリフェラル -> セントラル
#define BLE_GATT_HEADER_SIZE 3 // GATT header size for Notify / NotifyのGATTヘッダサイズ
#define BLE_DEFAULT_NOTIFY_PAYLOAD_SIZE 20 // Default maximum payload size (Default MTU 23 - GATT header 3) / デフォルトの最大ペイロードサイズ(デフォルトMTU23 - GATTヘッダ3)

// File scope variables / [ファイルスコープ変数]
static BLEDevice* f_pDevice = NULL; // Connection device / 接続デバイス
static hci_con_handle_t f_conHandle;  // Connection handle / 接続ハンドル
static uint16_t f_characteristicWriteHandle;   // Characteristic write handle / Characteristic書き込みのハンドル
static uint16_t f_characteristicNotifyHandle;  // Characteristic notify handle / Characteristic通知のハンドル
static btstack_context_callback_registration_t f_stNotifiableCallbackContext; // Context for notifiable callback / 通知可能コールバックのコンテキスト 
static ST_NOTIFY_DATA f_stNotifyData; // Notification data / 通知データ
static volatile bool f_isNotifying = false; // Whether notification is in progress / 通知処理中か否か
static volatile bool f_isNotifyEnabled = false; // Whether central enabled Notify (CCCD) / セントラルがNotify(CCCD)を有効にしたか
static volatile bool f_lastNotifyResult = false; // Whether last Notify send succeeded / 前回のNotify送信が成功したか

// Advertising data / アドバタイズデータ
static const uint8_t f_advertisingData[] = {
    0x02, 0x01, 0x06, // Flags: LE General Discoverable Mode, BR/EDR Not Supported
    0x11, 0x07,       // Complete List of 128-bit Service Class UUIDs
    // 6E400001-B5A3-F393-E0A9-E50E24DCCA9E (Little Endian / リトルエンディアン)
    0x9E, 0xCA, 0xDC, 0x24, 0x0E, 0xE5, 0xA9, 0xE0,
    0x93, 0xF3, 0xA3, 0xB5, 0x01, 0x00, 0x40, 0x6E, // NUS_SERVICE_UUID
    8,                                  // Length: Length of "Type + Value" / Length:「Type + Value」の長さ
    0x09,                               // Type: Complete Local Name / Type:完全なローカル名
    'P', 'i', 'c', 'o', 'B', 'r', 'g'   // Value: local_name / Value:ローカル名 
};

// Function prototypes / [関数プロトタイプ宣言]
static void BLE_DeviceConnectedCallback(BLEStatus status, BLEDevice* pDevice);
static void BLE_DeviceDisconnectedCallback(BLEDevice* pDevice);
static int BLE_CharacteristicWriteCallback(uint16_t value_handle, uint8_t* pBuf, uint16_t size);
static void BLE_CharacteristicNotifiableCallback(void *context);

// Initialize BLE / BLEを初期化
void BLE_Init()
{
    // Set callbacks / コールバックを設定
    BTstack.setBLEDeviceConnectedCallback(BLE_DeviceConnectedCallback);
    BTstack.setBLEDeviceDisconnectedCallback(BLE_DeviceDisconnectedCallback);
    BTstack.setGATTCharacteristicWrite(BLE_CharacteristicWriteCallback);

    // Setup GATT Database / GATTデータベースをセットアップ
    // Nordic UART Service (NUS)
    BTstack.addGATTService(new UUID(NUS_SERVICE_UUID));
    f_characteristicWriteHandle = BTstack.addGATTCharacteristicDynamic(new UUID(NUS_RX_CHAR_UUID), ATT_PROPERTY_WRITE | ATT_PROPERTY_WRITE_WITHOUT_RESPONSE, 0);
    f_characteristicNotifyHandle = BTstack.addGATTCharacteristicDynamic(new UUID(NUS_TX_CHAR_UUID), ATT_PROPERTY_NOTIFY, 0);

    // Startup Bluetooth and activate advertisements / Bluetoothを起動し、アドバタイズを有効化
    BTstack.setup();
    BTstack.setAdvData(sizeof(f_advertisingData), f_advertisingData);
    BTstack.startAdvertising();
}

// BLE main process / BLEのメイン処理
void BLE_Main()
{
    // BTstack main process / BTstackのメイン処理
    BTstack.loop();
}

// Get whether connected to central / セントラルと接続済みか否かを取得
bool BLE_IsConnected()
{
    return (f_pDevice != NULL && f_isNotifyEnabled) ? true : false;
}

// Request notification / 通知の要求
bool BLE_RequestNotify(uint8_t *buffer, uint16_t size) 
{
    bool bRet = true;

    if ((size > 0) && BLE_IsConnected()) { // Connected to central and Notify enabled / セントラルと接続済みかつNotify有効
        f_isNotifying = true; // Notification in progress / 通知処理中
        f_lastNotifyResult = false; // Clear result / 結果をクリア
        memset(&f_stNotifiableCallbackContext, 0, sizeof(f_stNotifiableCallbackContext));
        // Set notification data to context / コンテキストに通知データを指定
        f_stNotifyData.buffer = buffer;
        f_stNotifyData.size = size;    
        f_stNotifiableCallbackContext.context = (void*)&f_stNotifyData;  
        // Set callback for when notification is possible / 通知可能になると呼ばれるコールバックを指定
        f_stNotifiableCallbackContext.callback = BLE_CharacteristicNotifiableCallback;
        // Request notification / 通知の要求
        if (0 == att_server_request_to_send_notification(&f_stNotifiableCallbackContext, f_conHandle)) {
            // This function returns success / 本関数は成功を返す 
        }
        else {
            // This function returns failure / 本関数は失敗を返す
            f_isNotifying = false;
            bRet = false;
        }
    }
    else {
        // Discard (return success to let Wireless.cpp discard data) / 破棄(Wireless.cpp側でデータを破棄させるために成功を返す) 
        f_lastNotifyResult = true; 
    }

    return bRet;
} 

// Get whether notification is in progress / 通知処理中か否かを取得
bool BLE_IsNotifying()
{
    return f_isNotifying;    
}

// Get last Notify send result and clear flag / 前回のNotify送信結果を取得し、フラグをクリアする
bool BLE_GetNotifyResultAndClear()
{
    bool bRet = f_lastNotifyResult;
    f_lastNotifyResult = false;
    return bRet;
}

// Clear Notify send result / Notify送信結果をクリアする
void BLE_ClearNotifyResult()
{
    f_lastNotifyResult = false;
}

// Get maximum Notify size considering MTU / MTUを考慮したNotify送信可能な最大サイズを取得
uint16_t BLE_GetMaxNotifySize()
{
    if (BLE_IsConnected()) {
        uint16_t mtu = att_server_get_mtu(f_conHandle);
        if (mtu > BLE_GATT_HEADER_SIZE) {
            return mtu - BLE_GATT_HEADER_SIZE; // Exclude GATT header / GATTヘッダを除外
        }
    }
    return BLE_DEFAULT_NOTIFY_PAYLOAD_SIZE; // Default maximum payload size / デフォルトの最大ペイロードサイズ
}

// Callback for device connection notification / デバイス接続通知のコールバック
static void BLE_DeviceConnectedCallback(BLEStatus status, BLEDevice* pDevice) 
{
    switch (status) {
        case BLE_STATUS_OK:
            if (pDevice != NULL) {
                f_conHandle = pDevice->getHandle();
            }
            f_pDevice = pDevice;
            break;
        default:
            break;
    }
}
  
// Callback for device disconnection notification / デバイス切断通知のコールバック
static void BLE_DeviceDisconnectedCallback(BLEDevice* pDevice) 
{
    f_pDevice = NULL;
    f_isNotifying = false;
    f_isNotifyEnabled = false;
    f_lastNotifyResult = false;
}

// Characteristic write callback / Characteristic書き込みコールバック
static int BLE_CharacteristicWriteCallback(uint16_t value_handle, uint8_t* pBuf, uint16_t size) 
{
    uint16_t i;
    UCHAR* pDataAry = (UCHAR*)pBuf;

    // When data destination is RX Characteristic / データの書き込み先がRX Characteristicの場合
    if (value_handle == f_characteristicWriteHandle) {
        // Enqueue UART send data / UART送信データをエンキュー   
        // *If queue is full, data that could not be enqueued is discarded / ※キューが満杯の場合、エンキューできなかったデータは破棄される
        for (i = 0; i < size ; i++) { // Repeat for data size / データサイズ分繰り返す
            // Enqueue 1 byte of UART send data / UART送信データ1byteのエンキュー
            if (!CMN_Enqueue(CMN_QUE_KIND_UART_SEND, &pDataAry[i], true)) {
                break; // Queue is full / キューが満杯
            }
        }
    }
    // When data destination is CCCD of TX Characteristic / データの書き込み先がTX CharacteristicのCCCD(Client Characteristic Configuration Descriptor)の場合
    else if (value_handle == f_characteristicNotifyHandle + 1) {
        if (size >= 2) {
            uint16_t cccd = pBuf[0] | (pBuf[1] << 8);
            f_isNotifyEnabled = (cccd & 0x0001) != 0;
        }
    }

    return 0;
}

// Callback called when notification is possible / 通知可能になると呼ばれるコールバック
static void BLE_CharacteristicNotifiableCallback(void *context)
{
    ST_NOTIFY_DATA* pstNotifyData = (ST_NOTIFY_DATA*)context;

    if (BLE_IsConnected()) { // Connected to central and Notify enabled / セントラルと接続済みかつNotify有効
        // Request Characteristic notification / Characteristic通知の要求
        if (0 != att_server_notify(f_conHandle, f_characteristicNotifyHandle, pstNotifyData->buffer, pstNotifyData->size)) {
            // Set FW error / FWエラーを設定
            CMN_SetErrorBits(CMN_ERR_BIT_WL_SEND_ERR, true);
            f_lastNotifyResult = false;
        } else {
            f_lastNotifyResult = true; // Send success / 送信成功
        }  
    } else {
        f_lastNotifyResult = false;
    }

    f_isNotifying = false;
}