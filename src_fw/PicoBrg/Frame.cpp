// Copyright © 2024 Shiomachi Software. All rights reserved.
#include "Common.h"

// [define] / [マクロ定義]
#define SERIAL_BAUDRATE 115200 // CDC baud rate (bps) / CDCのボーレート(bps)

// File scope variables / [ファイルスコープ変数]
static ST_FRM_RECV_DATA_INFO f_stRecvDataInf = {0}; // USB receive data info / USB受信データ情報

// Function prototypes / [関数プロトタイプ宣言]
static ST_FRM_REQ_FRAME* FRM_RecvReqFrame(void);

// Extract USB received data ⇒ Parse and execute command / USB受信データ取り出し⇒コマンド解析・実行
void FRM_Main()
{
    ST_FRM_REQ_FRAME *pstReqFrm = NULL; // Request frame / 要求フレーム

	// Create request frame from USB received data / USB受信データから要求フレームを作成する
	pstReqFrm = FRM_RecvReqFrame();
	if (pstReqFrm != NULL) { // If request frame extraction is completed / 要求フレームの抽出が完了した場合
		// Parse and execute command / コマンドを解析・実行
		CMD_ExecReqCmd(pstReqFrm);
	}
}

// Create request frame from USB received data / USB受信データから要求フレームを作成する
static ST_FRM_REQ_FRAME* FRM_RecvReqFrame(void)
{
	int32_t ret = -1;
	UCHAR data = 0; 					// Receive data (1byte) / 受信データ(1byte)
	ULONG reqFrmSize = 0; 				// Size of request frame (excluding checksum) / 要求フレームのサイズ(チェックサム除く)
	ST_FRM_REQ_FRAME *pstReqFrm = NULL; // Extracted request frame (NULL if incomplete) / 抽出が完了した要求フレーム(未完了の場合はNULL)
	ST_FRM_RECV_DATA_INFO *pstRecv = &f_stRecvDataInf;
	
	// [Request frame receive timeout check] / [要求フレームの受信タイムアウト判定]
	if (pstRecv->reqFrmSize > 0) { // If request frame header is already received / 要求フレームのヘッダは受信済みの場合
		if (TMR_IsRecvTimeout() // If request frame receive timeout occurred / 要求フレーム受信タイムアウトが発生した場合
		|| (!tud_cdc_connected())) { // If disconnected / 未接続の場合 
			pstRecv->reqFrmSize = 0; // Discard frame / フレーム破棄
		}
	}	
	
	// [Extract 1 byte of USB received data] / [USB受信データ1byte取り出し]
	if (Serial.available() > 0) {
		ret = Serial.read();
	}
	if (ret < 0) { // If there is no USB receive data / USB受信データが無い場合
		return pstReqFrm; // Return NULL / NULLを返す
	}
	data = (UCHAR)ret;

	// [Create request frame from USB received data] / [USB受信データから要求フレームを作成する]
	// Header / ヘッダ
	if (pstRecv->reqFrmSize == offsetof(ST_FRM_REQ_FRAME, header)) {
		if (FRM_HEADER_REQ == data) { 
			// If request header / 要求ヘッダの場合

			pstRecv->recvDataSize = 0;       // Initialize received size of data size part / データサイズ部の受信済みサイズを初期化
			pstRecv->recvChecksum = 0;       // Initialize received size of checksum part / チェックサム部の受信済みサイズを初期化
			pstRecv->p = (UCHAR*)&pstRecv->stReqFrm; // Initialize request frame data storage pointer / 要求フレームデータ格納先ポインタを初期化	
			*pstRecv->p++ = data;			 // Store header / ヘッダを格納
			pstRecv->reqFrmSize++;			 // Received size of request frame + 1 / 要求フレームの受信済みサイズ+1

			// Clear request frame receive timeout count / 要求フレーム受信タイムアウトのタイマカウントをクリア
			TMR_ClearRecvTimeout();	
		}
		else {
			// If not request header / 要求ヘッダではない場合	

			pstRecv->reqFrmSize = 0; // Discard frame / フレーム破棄
		}		
	}
	// Sequence number / シーケンス番号
	else if (pstRecv->reqFrmSize < offsetof(ST_FRM_REQ_FRAME, seqNo) + sizeof(pstRecv->stReqFrm.seqNo)) { 
		*pstRecv->p++ = data;  // Store sequence number / シーケンス番号を格納
		pstRecv->reqFrmSize++; // Received size of request frame + 1 / 要求フレームの受信済みサイズ+1
	}
	// Command / コマンド
	else if (pstRecv->reqFrmSize < offsetof(ST_FRM_REQ_FRAME, cmd) + sizeof(pstRecv->stReqFrm.cmd)) { 
		*pstRecv->p++ = data;  // Store command / コマンドを格納
		pstRecv->reqFrmSize++; // Received size of request frame + 1 / 要求フレームの受信済みサイズ+1					
	}
	// Data size / データサイズ
	else if (pstRecv->reqFrmSize < offsetof(ST_FRM_REQ_FRAME, dataSize) + sizeof(pstRecv->stReqFrm.dataSize)) { 	
		*pstRecv->p++ = data;       // Store data size / データサイズを格納
		pstRecv->reqFrmSize++;      // Received size of request frame + 1 / 要求フレームの受信済みサイズ+1	
		pstRecv->recvDataSize++;    // Received size of data size part + 1 / データサイズ部の受信済みサイズ+1
		if (pstRecv->recvDataSize == sizeof(pstRecv->stReqFrm.dataSize)) { // If reception of data size part is completed / データサイズ部の受信が完了した場合
			if (pstRecv->stReqFrm.dataSize > FRM_DATA_MAX_SIZE) { // If data size exceeds maximum value / データサイズが最大値を超えている場合
				pstRecv->reqFrmSize = 0; // Discard frame / フレーム破棄
			}			
		}				
	}
	// Data part / データ部
	else if (pstRecv->reqFrmSize < offsetof(ST_FRM_REQ_FRAME, dataSize) + sizeof(pstRecv->stReqFrm.dataSize) + pstRecv->stReqFrm.dataSize) { 
		*pstRecv->p++ = data;  // Store data part / データ部を格納
		pstRecv->reqFrmSize++;	// Received size of request frame + 1 / 要求フレームの受信済みサイズ+1	
	}
	// Checksum / チェックサム
	else if (pstRecv->reqFrmSize < offsetof(ST_FRM_REQ_FRAME, dataSize) + sizeof(pstRecv->stReqFrm.dataSize) + pstRecv->stReqFrm.dataSize + sizeof(pstRecv->stReqFrm.checksum)) {
		// Data part: aData[] member size is fixed to FRM_DATA_MAX_SIZE, so variable like pstRecv->recvChecksum and processing below are needed / データ部:aData[]メンバのサイズがFRM_DATA_MAX_SIZE固定のため、pstRecv->recvChecksumのような変数や下記の処理が必要 				
		if (!pstRecv->recvChecksum) { 
			pstRecv->p = (UCHAR*)&pstRecv->stReqFrm.checksum; // Storage pointer points to address of checksum part / 格納先ポインタはチェックサム部のアドレスを指す
		}
		*pstRecv->p++ = data;       // Store checksum / チェックサムを格納
		pstRecv->reqFrmSize++;      // Received size of request frame + 1 / 要求フレームの受信済みサイズ+1
		pstRecv->recvChecksum++;    // Received size of checksum part + 1 / チェックサム部の受信済みサイズ+1
	}		
	else {
		// No processing / 無処理
	}

	if (pstRecv->reqFrmSize >= offsetof(ST_FRM_REQ_FRAME, dataSize) 
		+ sizeof(pstRecv->stReqFrm.dataSize) 
		+ pstRecv->stReqFrm.dataSize + sizeof(pstRecv->stReqFrm.checksum)) {
		// If request frame extraction is completed / 要求フレームの抽出が完了した場合	

		pstRecv->reqFrmSize = 0; // Initialize received size of request frame / 要求フレームの受信済みサイズを初期化

		// [Checksum verification] / [チェックサム検査]
		// Calculate size of request frame (excluding checksum) / 要求フレームのサイズ(チェックサム除く)を計算
		reqFrmSize = offsetof(ST_FRM_REQ_FRAME, dataSize) + sizeof(pstRecv->stReqFrm.dataSize) + pstRecv->stReqFrm.dataSize; 
		// Execute checksum verification / チェックサム検査を実行
		if (CMN_Checksum(&pstRecv->stReqFrm, pstRecv->stReqFrm.checksum, reqFrmSize)) {
			// If checksum verification passes / チェックサム検査に合格した場合
			pstReqFrm = &pstRecv->stReqFrm; // Set pointer of request frame to return value / 戻り値に要求フレームのポインタを設定
		}
	}

	return pstReqFrm;
}

// USB send of response frame / 応答フレームのUSB送信
void FRM_MakeAndSendResFrm(USHORT seqNo, USHORT cmd, USHORT errCode, USHORT dataSize, PVOID pBuf)
{
	ULONG frmSize;        			// Size of response frame (excluding checksum) / 応答フレームのサイズ(チェックサム除く)
	UCHAR* pDataAry = (UCHAR*)pBuf;	// Data part of response frame / 応答フレームのデータ部
	ST_FRM_RES_FRAME stResFrm; 		// Response frame / 応答フレーム

	// Create response frame / 応答フレームを作成
	stResFrm.header   = FRM_HEADER_RES;	// Header / ヘッダ
	stResFrm.seqNo    = seqNo; 			// Sequence number / シーケンス番号
	stResFrm.cmd      = cmd;   			// Command / コマンド
	stResFrm.errCode  = errCode;       	// Error code / エラーコード
	stResFrm.dataSize = dataSize;      	// Data size / データサイズ	
	// Data / データ
	if ((pDataAry != NULL) && (dataSize > 0)) { 
		memcpy(stResFrm.aData, pDataAry, dataSize);
	}
	// Calculate size of response frame (excluding checksum) / 応答フレームのサイズ(チェックサム除く)を計算
	frmSize = offsetof(ST_FRM_RES_FRAME, dataSize) + sizeof(stResFrm.dataSize) + stResFrm.dataSize;  
	// Calculate checksum / チェックサムを計算 
	stResFrm.checksum = CMN_CalcChecksum(&stResFrm, frmSize); 
	
	// USB send / USB送信
	Serial.write((UCHAR*)&stResFrm, frmSize); // Header part to data part *For data part: aData[] member, only frmSize is target for send / ヘッダ部～データ部 ※データ部:aData[]メンバについてはfrmSize分だけが送信対象
	Serial.write((UCHAR*)&stResFrm.checksum, sizeof(stResFrm.checksum)); // Checksum part / チェックサム部
}

// Initialize USB communication / USB通信を初期化
void FRM_Init()
{
	// Initialize variables / 変数を初期化
	f_stRecvDataInf.p = (UCHAR*)&(f_stRecvDataInf.stReqFrm);

	// Initialize CDC / CDCを初期化
	Serial.begin(SERIAL_BAUDRATE);	
}
