// Copyright © 2024 Shiomachi Software. All rights reserved.
#include "Common.h"

// [ファイルスコープ変数]
static UCHAR f_aSendData[CMN_QUE_DATA_MAX_WL_SEND] = {0}; // 無線送信データのバッファ

// 無線受信データをデキュー⇒UART送信データをエンキュー
void WL_RecvMain()
{
    UCHAR data;
    ULONG i;

	for (i = 0; i < CMN_QUE_DATA_MAX_UART_SEND ; i++) { // データサイズ分繰り返す
	    // 無線受信データ1byteのデキュー
		if (!CMN_Dequeue(CMN_QUE_KIND_WL_RECV, &data, sizeof(UCHAR), true)) { 
			break; // キューが空のの場合
		}
		
		// UART送信データ1byteのエンキュー
		if (!CMN_Enqueue(CMN_QUE_KIND_UART_SEND, &data, sizeof(UCHAR), true)) {
			break; // キューが満杯
		}
	}    
}

// 無線送信データをデキュー⇒無線送信
void WL_SendMain()
{
	UCHAR data;
	ULONG iData;
	ULONG size; // 送信サイズ

	for (iData = 0; iData < CMN_QUE_DATA_MAX_WL_SEND; iData++) { // 無線送信バッファのサイズ分繰り返す
		// 無線送信データ1byteのデキュー
		if (CMN_Dequeue(CMN_QUE_KIND_WL_SEND, &data, sizeof(UCHAR), true)) {
			// 無線送信データのバッファに無線送信要求データを格納
			f_aSendData[iData] = data; 
		}
		else {
			break; // キューが空
		}
	}
	size = iData; // 送信サイズ

	if (size > 0) {
		if (tcp_cmn_is_connected()) { // TCP接続済み
			// TCP送信
			if (ERR_OK != tcp_cmn_send_data(f_aSendData, size)) {
				CMN_SetErrorBits(CMN_ERR_BIT_BUF_tcp_write_err, true);
			}
		}			
	}
}

// 無線送信要求の発行
void WL_ReqToSend(PVOID pBuf, ULONG size)
{
	UCHAR* pDataAry = (UCHAR*)pBuf;
	ULONG iData;

	for (iData = 0; iData < size; iData++) {
		// 無線送信データをエンキュー
		if (!CMN_Enqueue(CMN_QUE_KIND_WL_SEND, &pDataAry[iData], sizeof(UCHAR), true)) { 
			break; // キューが満杯
		}
	} 		
}