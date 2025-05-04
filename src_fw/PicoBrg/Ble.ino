// Copyright © 2025 Shiomachi Software. All rights reserved.
#include "Common.h"

// [ファイルスコープ変数]
static BLEDevice* f_pDevice = NULL; // 接続デバイス
static hci_con_handle_t f_con_handle; // 接続ハンドル
static uint16_t f_characteristicNotifyHandle;  // Characteristic通知のハンドル
static btstack_context_callback_registration_t f_stNotifiableCallbackContext; // 通知可能コールバックのコンテキスト 
static ST_NOTIFY_DATA f_stNotifyData; // 通知データ
static bool f_isNotifying = false; // 通知処理中か否か

// アドバタイズデータ
static const uint8_t f_advertisingData[] = {
    0x02, 0x01, 0x06, // Flags: LE General Discoverable Mode, BR/EDR Not Supported
    0x11, 0x07, // Complete List of 128-bit Service Class UUIDs
    0xF0, 0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7,
    0xF8, 0xF9, 0xFA, 0xFB, 0xFC, 0xFD, 0xFE, 0xFF, // custom_uuid
    8,                                  // Length:「Type + Value」の長さ
    0x09,                               // Type:    Complete Local Name
    'P', 'i', 'c', 'o', 'B', 'r', 'g'   // Value:   local_name 
};

// [関数プロトタイプ宣言]
static void BLE_DeviceConnectedCallback(BLEStatus status, BLEDevice* pDevice);
static void BLE_DeviceDisconnectedCallback(BLEDevice* pDevice);
static int BLE_CharacteristicWriteCallback(uint16_t value_handle, uint8_t* pBuf, uint16_t size);
static void BLE_CharacteristicNotifiableCallback(void *context);
static void BLE_Init();  

// デバイス接続通知のコールバック
static void BLE_DeviceConnectedCallback(BLEStatus status, BLEDevice* pDevice) 
{
    switch (status) {
    case BLE_STATUS_OK:
        if (pDevice != NULL) {
            f_con_handle = pDevice->getHandle();
        }
        f_pDevice = pDevice;
        break;
    default:
        break;
    }
}
  
// デバイス切断通知のコールバック
static void BLE_DeviceDisconnectedCallback(BLEDevice* pDevice) 
{
    f_pDevice = NULL;
    f_isNotifying = false;
}

// Characteristic書き込みコールバック
static int BLE_CharacteristicWriteCallback(uint16_t value_handle, uint8_t* pBuf, uint16_t size) 
{
    uint16_t i;

    // UART送信データをエンキュー   
    for (i = 0; i < size ; i++) { // データサイズ分繰り返す
        // UART送信データ1byteのエンキュー
        if (!CMN_Enqueue(CMN_QUE_KIND_UART_SEND, &pBuf[i], sizeof(uint8_t), true)) {
            break; // キューが満杯
        }
    }   

    return 0;
}

// 通知可能になると呼ばれるコールバック
static void BLE_CharacteristicNotifiableCallback(void *context)
{
    ST_NOTIFY_DATA* pstNotifyData = (ST_NOTIFY_DATA*)context;

    if (BLE_IsConnected()) { // セントラルと接続済み
        // Characteristic通知の要求
        if (0 != att_server_notify(f_con_handle, f_characteristicNotifyHandle, pstNotifyData->buffer, pstNotifyData->size)) {
            // FWエラーを設定
            CMN_SetErrorBits(CMN_ERR_BIT_WL_SEND_ERR, true);
        }  
    }

    f_isNotifying = false;
}

// 通知の要求
bool BLE_ReqToNotify(uint8_t *buffer, uint16_t size) 
{
    bool bRet = true;

    if ((size > 0) && BLE_IsConnected()) { // セントラルと接続済み
        f_isNotifying = true; // 通知処理中
        memset(&f_stNotifiableCallbackContext, 0, sizeof(f_stNotifiableCallbackContext));
        // コンテキストに通知データを指定
        f_stNotifyData.buffer = buffer;
        f_stNotifyData.size = size;    
        f_stNotifiableCallbackContext.context = (void*)&f_stNotifyData;  
        // 通知可能になると呼ばれるコールバックを指定
        f_stNotifiableCallbackContext.callback = BLE_CharacteristicNotifiableCallback;
        // 通知の要求
        if (0 == att_server_request_to_send_notification(&f_stNotifiableCallbackContext, f_con_handle)) {
            // 本関数は成功を返す 
        }
        else {
            // 本関数は失敗を返す
            f_isNotifying = false;
            bRet = false;
        }
    }
    else {
        // 破棄(本関数は成功を返す) 
    }

    return bRet;
} 

// 通知処理中か否かを取得
bool BLE_IsNotifying()
{
    return f_isNotifying;    
}

// セントラルと接続済みか否か
bool BLE_IsConnected()
{
    return (f_pDevice != NULL)  ? true : false;
}

// BLEのメイン処理
void BLE_Main()
{
    static bool s_isInited = false; // BLEを初期化済みか否か

    if (!s_isInited) {
        // BLEを初期化
        BLE_Init();
        s_isInited = true;
    }
    
    // BTstackのメイン処理
    BTstack.loop();
}

// BLEを初期化
static void BLE_Init()
{
    // set callbacks
    BTstack.setBLEDeviceConnectedCallback(BLE_DeviceConnectedCallback);
    BTstack.setBLEDeviceDisconnectedCallback(BLE_DeviceDisconnectedCallback);
    BTstack.setGATTCharacteristicWrite(BLE_CharacteristicWriteCallback);

    // setup GATT Database
    // Nordic UART Service (NUS)
    BTstack.addGATTService(new UUID("6E400001-B5A3-F393-E0A9-E50E24DCCA9E"));
    BTstack.addGATTCharacteristicDynamic(new UUID("6E400002-B5A3-F393-E0A9-E50E24DCCA9E"), ATT_PROPERTY_WRITE, 0);
    f_characteristicNotifyHandle = BTstack.addGATTCharacteristicDynamic(new UUID("6E400003-B5A3-F393-E0A9-E50E24DCCA9E"), ATT_PROPERTY_NOTIFY, 0);

    // startup Bluetooth and activate advertisements
    BTstack.setup();
    BTstack.setAdvData(sizeof(f_advertisingData), f_advertisingData);
    BTstack.startAdvertising();
}