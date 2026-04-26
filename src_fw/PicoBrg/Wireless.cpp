// Copyright © 2025 Shiomachi Software. All rights reserved.
#include "Common.h"

// [define] / [マクロ定義]
#define WL_BLE_NOTIFY_MAX_SIZE 20 // Max send size per BLE transmission (safe size considering MTU) / BLEの1回あたりの最大送信サイズ(MTUを考慮した安全なサイズ)

// File scope variables / [ファイルスコープ変数]
static UCHAR f_aSendData[CMN_QUE_DATA_MAX_WL_SEND] = {0}; // Wireless send data buffer / 無線送信データのバッファ
static ULONG f_sendDataSize = 0; // Size of pending send data / 送信待ちデータのサイズ

// Initialize wireless / 無線を初期化
void WL_Init()
{
	ST_FLASH_DATA* pstFlashData = FLASH_GetDataAtPowerOn(); // Get FLASH data at power on / 電源起動時のFLASHデータを取得

	if (pstFlashData->stNwConfig.isWifi) { // In case of TCP / TCPの場合
		TCP_Init(pstFlashData);
	}
	else { // In case of BLE / BLEの場合
		BLE_Init();
	}
}

// Wireless main process / 無線のメイン処理
void WL_Main()
{
	ST_FLASH_DATA* pstFlashData = FLASH_GetDataAtPowerOn(); // Get FLASH data at power on / 電源起動時のFLASHデータを取得

	if (pstFlashData->stNwConfig.isWifi) {
		// TCP main process / TCPのメイン処理
		TCP_Main();
	}
	else {
		// BLE main process / BLEのメイン処理
		BLE_Main();
	}
}

// Get whether connected to communication partner / 通信相手と接続済みか否かを取得
bool WL_IsConnected()
{
	ST_FLASH_DATA* pstFlashData = FLASH_GetDataAtPowerOn(); // Get FLASH data at power on / 電源起動時のFLASHデータを取得

	if (pstFlashData->stNwConfig.isWifi) { // In case of TCP / TCPの場合
		return TCP_IsApConnected();
	} else { // In case of BLE / BLEの場合
		return BLE_IsConnected();
	}
}

// Dequeue wireless send data ⇒ Wireless send / 無線送信データをデキュー⇒無線送信
void WL_SendMain()
{
	UCHAR data;
	ULONG i;
	ULONG maxSize;
	bool  isSendErr = false;
	ST_FLASH_DATA* pstFlashData = FLASH_GetDataAtPowerOn(); // Get FLASH data at power on / 電源起動時のFLASHデータを取得

	// If wireless is not connected, clear all queues and related flags / 無線が未接続の場合、全てのキューと関連フラグをクリアする
	if (!WL_IsConnected()) {
		CMN_ClearQue(CMN_QUE_KIND_WL_SEND, true);
		CMN_ClearQue(CMN_QUE_KIND_UART_SEND, true);
		CMN_ClearQue(CMN_QUE_KIND_UART_RECV, true);
		UART_ClearPendingData(); // Clear pending UART receive data / 保留中のUART受信データをクリア

		f_sendDataSize = 0; // No pending send data / 送信待ちデータは無し
		if (!pstFlashData->stNwConfig.isWifi) {
			BLE_ClearNotifyResult(); // Clear BLE send result / BLE送信結果をクリア
		}
		
		return; // Return to skip subsequent send/dequeue processing when disconnected / 未接続時は以降の送信・デキュー処理を行う必要がないため抜ける
	}

	// 1. Dequeue only when there is no pending send data / 1. 送信待ちデータがない場合のみキューからデキューする
	if (f_sendDataSize == 0) {
		maxSize = sizeof(f_aSendData);
		if (!pstFlashData->stNwConfig.isWifi) { // In case of BLE / BLEの場合
			// Limit max send size per transmission considering BLE MTU limit / BLEのMTU制限を考慮して、一度に送信する最大サイズを制限する
			maxSize = WL_BLE_NOTIFY_MAX_SIZE; 
		}

		for (i = 0; i < maxSize; i++) {
			// Dequeue 1 byte of wireless send data / 無線送信データ1byteのデキュー
			if (CMN_Dequeue(CMN_QUE_KIND_WL_SEND, &data, true)) {
				// Store wireless send request data in wireless send data buffer / 無線送信データのバッファに無線送信要求データを格納
				f_aSendData[i] = data; 
			}
			else {
				break; // Queue is empty / キューが空
			}
		}
		f_sendDataSize = i; // Hold dequeued size / デキューしたサイズを保持
	}

	// 2. If there is pending send data, perform send process (or check result) / 2. 送信待ちデータがある場合は送信処理(または結果確認)を行う
	if (f_sendDataSize > 0) {
		if (pstFlashData->stNwConfig.isWifi) { // In case of TCP / TCPの場合
			// TCP send / TCP送信
			int32_t sentSize = TCP_SendData(f_aSendData, f_sendDataSize);
			if (sentSize < 0) {
				isSendErr = true;
			}
			else if (sentSize > 0) {
				if ((ULONG)sentSize < f_sendDataSize) {
					f_sendDataSize -= sentSize;
					memmove(f_aSendData, &f_aSendData[sentSize], f_sendDataSize); // Shift unsent data to the front / 未送信分を前に詰める
				} else {
					f_sendDataSize = 0; // All sent successfully (or discarded due to disconnection) / 全て送信成功(または未接続による破棄)
				}
			}
            else {
                // TCP send buffer is full / TCPの送信バッファが満杯
            }
		}
		else { // In case of BLE / BLEの場合
			if (!BLE_IsNotifying()) {
				if (BLE_GetNotifyResultAndClear()) {
					// Previous send completed and succeeded / 前回の送信が完了し、成功した
					f_sendDataSize = 0;
				}
				else {
					// Unsent or previous send failed, so request (re)send / 未送信、または前回の送信が失敗したため、(再)送信要求
					if (!BLE_RequestNotify(f_aSendData, f_sendDataSize)) {
						isSendErr = true;
						// f_sendDataSize = 0; // Discard here to prevent clogging from infinite resend and prioritize latest data / 無限再送による詰まりを防ぎ最新データを優先する場合はここで破棄する
					}
				}
			}
			else {
				// No processing during Notify transmission / Notify送信中は無処理
			}
		}			

		if (isSendErr) {
			// Set FW error / FWエラーを設定
			CMN_SetErrorBits(CMN_ERR_BIT_WL_SEND_ERR, true);
		}
	}
}

// Store default values in ST_NW_CONFIG2 structure / ST_NW_CONFIG2構造体にデフォルト値を格納
void WL_SetDefault(ST_NW_CONFIG2 *pstConfig)
{
    memset(pstConfig, 0, sizeof(ST_NW_CONFIG2)); // Zero clear entire structure / 構造体全体をゼロクリア

    pstConfig->isWifi = WL_DEFAULT_IS_WIFI;
    strcpy(pstConfig->szCountryCode, WIFI_DEFAULT_COUNTRY_CODE);
    pstConfig->aMyIpAddr[0] = (WIFI_DEFAULT_MY_IP_ADDR >> 24) & 0xFF;
    pstConfig->aMyIpAddr[1] = (WIFI_DEFAULT_MY_IP_ADDR >> 16) & 0xFF;
    pstConfig->aMyIpAddr[2] = (WIFI_DEFAULT_MY_IP_ADDR >> 8) & 0xFF;
    pstConfig->aMyIpAddr[3] = (WIFI_DEFAULT_MY_IP_ADDR) & 0xFF;
    memset(pstConfig->szSsid, 0, sizeof(pstConfig->szSsid));
    memset(pstConfig->szPassword, 0, sizeof(pstConfig->szPassword));
    pstConfig->aServerIpAddr[0] = (TCP_DEFAULT_REMOTE_IP_ADDR >> 24) & 0xFF;
    pstConfig->aServerIpAddr[1] = (TCP_DEFAULT_REMOTE_IP_ADDR >> 16) & 0xFF;
    pstConfig->aServerIpAddr[2] = (TCP_DEFAULT_REMOTE_IP_ADDR >> 8) & 0xFF;
    pstConfig->aServerIpAddr[3] = (TCP_DEFAULT_REMOTE_IP_ADDR) & 0xFF;
    pstConfig->isClient = TCP_DEFAULT_IS_CLIENT;
} 