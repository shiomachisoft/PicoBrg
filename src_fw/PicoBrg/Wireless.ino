// Copyright © 2025 Shiomachi Software. All rights reserved.
#include "Common.h"

// [ファイルスコープ変数]
static UCHAR f_aSendData[CMN_QUE_DATA_MAX_WL_SEND] = {0}; // 無線送信データのバッファ

// 無線のメイン処理
void WL_Main()
{
	ST_FLASH_DATA* pstFlashData = FLASH_GetDataAtPowerOn(); // 電源起動時のFLASHデータを取得

	if (pstFlashData->stNwConfig.isWifi) {
		// TCPのメイン処理
		tcp_main();
	}
	else {
		// BLEのメイン処理
		BLE_Main();
	}
}

// 無線送信データをデキュー⇒無線送信
void WL_SendMain()
{
	UCHAR data;
	ULONG i;
	ULONG size; // 送信サイズ
	bool  isSendErr = false;
	ST_FLASH_DATA* pstFlashData = FLASH_GetDataAtPowerOn(); // 電源起動時のFLASHデータを取得

	if (!pstFlashData->stNwConfig.isWifi) { // BLEの場合
		if (BLE_IsNotifying()) { // 通知処理中
			return;
		}
	}

	for (i = 0; i < sizeof(f_aSendData); i++) {
		// 無線送信データ1byteのデキュー
		if (CMN_Dequeue(CMN_QUE_KIND_WL_SEND, &data, sizeof(UCHAR), true)) {
			// 無線送信データのバッファに無線送信要求データを格納
			f_aSendData[i] = data; 
		}
		else {
			break; // キューが空
		}
	}
	size = i; // 送信サイズ

	if (size > 0) {
		if (pstFlashData->stNwConfig.isWifi) { // TCPの場合
			// TCP送信
			if (!tcp_send_data(f_aSendData, size)) {
				isSendErr = true;
			}
		}
		else { // BLEの場合
			// BLE送信(Notify)
			if (!BLE_ReqToNotify(f_aSendData, size)) {
				isSendErr = true;
			}
		}			

		if (isSendErr) {
			// FWエラーを設定
			CMN_SetErrorBits(CMN_ERR_BIT_WL_SEND_ERR, true);
		}
	}
}

// 無線送信要求の発行
void WL_ReqToSend(PVOID pBuf, ULONG size)
{
	UCHAR* pDataAry = (UCHAR*)pBuf;
	ULONG i;

	for (i = 0; i < size; i++) {
		// 無線送信データをエンキュー
		if (!CMN_Enqueue(CMN_QUE_KIND_WL_SEND, &pDataAry[i], sizeof(UCHAR), true)) { 
			break; // キューが満杯
		}
	} 		
}

// ST_NW_CONFIG2構造体にデフォルト値を格納
void WL_SetDefault(ST_NW_CONFIG2 *pstConfig)
{
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