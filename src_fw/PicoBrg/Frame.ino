// Copyright © 2024 Shiomachi Software. All rights reserved.
#include "Common.h"

// [define]
#define SERIAL_BAUDRATE 115200 // CDCのボーレート(bps)

// [ファイルスコープ変数]
static ST_FRM_RECV_DATA_INFO f_stRecvDataInf = {0}; // USB受信データ情報

// [関数プロトタイプ宣言]
static ST_FRM_REQ_FRAME* FRM_RecvReqFrame(void);

// USB受信データ取り出し⇒コマンド解析・実行
void FRM_Main()
{
    ST_FRM_REQ_FRAME *pstReqFrm = NULL; // 要求フレーム

	// USB受信データから要求フレームを作成する
	pstReqFrm = FRM_RecvReqFrame();
	if (pstReqFrm != NULL) { // 要求フレームの抽出が完了した場合
		// コマンドを解析・実行
		CMD_ExecReqCmd(pstReqFrm);
	}
}

// USB受信データから要求フレームを作成する
static ST_FRM_REQ_FRAME* FRM_RecvReqFrame(void)
{
	int32_t ret = -1;
	UCHAR data = 0; 					// 受信データ(1byte)
	ULONG reqFrmSize = 0; 			    // 要求フレームのサイズ(チェックサム除く)
	ST_FRM_REQ_FRAME *pstReqFrm = NULL; // 抽出が完了した要求フレーム(未完了の場合はNULL)
	ST_FRM_RECV_DATA_INFO *pstRecv = &f_stRecvDataInf;
	
	// [要求フレームの受信タイムアウト判定]
	if (pstRecv->reqFrmSize > 0) { // 要求フレームのヘッダは受信済みの場合
		if (TIMER_IsRecvTimeout() // 右記のタイムアウトが発生した場合:要求フレームのヘッダを受信後、TIMER_RECV_TIMEOUT[ms]経過しても要求フレームの末尾まで受信してない場合はタイムアウトとする
		|| (!tud_cdc_connected())) { // 未接続の場合 
			pstRecv->reqFrmSize = 0; // フレーム破棄
		}
	}	
	
	// [USB受信データ1byte取り出し]
	if (Serial.available() > 0) {
		ret = Serial.read();
	}
	if (ret < 0) { // USB受信データが無い場合
		return pstReqFrm; // NULLを返す
	}
	data = (UCHAR)ret;

	// [USB受信データから要求フレームを作成する]
	// ヘッダ
	if (pstRecv->reqFrmSize == offsetof(ST_FRM_REQ_FRAME, header)) {
		if (FRM_HEADER_REQ == data) { 
			// 要求ヘッダの場合

			pstRecv->recved_dataSize = 0;	 // データイサイズ部の受信済みサイズを初期化
			pstRecv->recved_checksum = 0; 	 // チェックサム部の受信済みサイズを初期化
			pstRecv->p = (UCHAR*)&pstRecv->stReqFrm; // 要求フレームデータ格納先ポインタを初期化	
			*pstRecv->p++ = data;			 // ヘッダを格納
			pstRecv->reqFrmSize++;			 // 要求フレームの受信済みサイズ+1

			// 右記のタイマカウントをクリア:要求フレームのヘッダを受信後、TIMER_RECV_TIMEOUT[ms]経過しても要求フレームの末尾まで受信してない場合はタイムアウトとする
			TIMER_ClearRecvTimeout();	
		}
		else {
			// 要求ヘッダではない場合	

			pstRecv->reqFrmSize = 0; // フレーム破棄
		}		
	}
	// シーケンス番号
	else if (pstRecv->reqFrmSize < offsetof(ST_FRM_REQ_FRAME, seqNo) + sizeof(pstRecv->stReqFrm.seqNo)) { 
		*pstRecv->p++ = data;  // シーケンス番号を格納
		pstRecv->reqFrmSize++; // 要求フレームの受信済みサイズ+1
	}
	// コマンド
	else if (pstRecv->reqFrmSize < offsetof(ST_FRM_REQ_FRAME, cmd) + sizeof(pstRecv->stReqFrm.cmd)) { 
		*pstRecv->p++ = data;  // コマンドを格納
		pstRecv->reqFrmSize++; // 要求フレームの受信済みサイズ+1					
	}
	// データサイズ
	else if (pstRecv->reqFrmSize < offsetof(ST_FRM_REQ_FRAME, dataSize) + sizeof(pstRecv->stReqFrm.dataSize)) { 	
		*pstRecv->p++ = data;  	 // データサイズを格納
		pstRecv->reqFrmSize++; 	 // 要求フレームの受信済みサイズ+1	
		pstRecv->recved_dataSize++; // データサイズ部の受信済みサイズ+1
		if (pstRecv->recved_dataSize == sizeof(pstRecv->stReqFrm.dataSize)) { // データサイズ部の受信が完了した場合
			if (pstRecv->stReqFrm.dataSize > FRM_DATA_MAX_SIZE) { // データサイズが最大値を超えている場合
				pstRecv->reqFrmSize = 0; // フレーム破棄
			}			
		}				
	}
	// データ部
	else if (pstRecv->reqFrmSize < offsetof(ST_FRM_REQ_FRAME, dataSize) + sizeof(pstRecv->stReqFrm.dataSize) + pstRecv->stReqFrm.dataSize) { 
		*pstRecv->p++ = data;  // データ部を格納
		pstRecv->reqFrmSize++;	// 要求フレームの受信済みサイズ+1	
	}
	// チェックサム
	else if (pstRecv->reqFrmSize < offsetof(ST_FRM_REQ_FRAME, dataSize) + sizeof(pstRecv->stReqFrm.dataSize) + pstRecv->stReqFrm.dataSize + sizeof(pstRecv->stReqFrm.checksum)) {
		// データ部:aData[]メンバのサイズがFRM_DATA_MAX_SIZE固定のため、pstRecv->recved_checksumのような変数や下記の処理が必要 				
		if (!pstRecv->recved_checksum) { 
			pstRecv->p = (UCHAR*)&pstRecv->stReqFrm.checksum; // 格納先ポインタはチェックサム部のアドレスを指す
		}
		*pstRecv->p++ = data;  	 // チェックサムを格納
		pstRecv->reqFrmSize++; 	 // 要求フレームの受信済みサイズ+1
		pstRecv->recved_checksum++; // チェックサム部の受信済みサイズ+1
	}		
	else {
		// 無処理
	}

	if (pstRecv->reqFrmSize >= offsetof(ST_FRM_REQ_FRAME, dataSize) 
		+ sizeof(pstRecv->stReqFrm.dataSize) 
		+ pstRecv->stReqFrm.dataSize + sizeof(pstRecv->stReqFrm.checksum)) {
		// 要求フレームの抽出が完成した場合	

		pstRecv->reqFrmSize = 0; // 要求フレームの受信済みサイズを初期化

		// [チェックサム検査]
		// 要求フレームのサイズ(チェックサム除く)を計算
		reqFrmSize = offsetof(ST_FRM_REQ_FRAME, dataSize) + sizeof(pstRecv->stReqFrm.dataSize) + pstRecv->stReqFrm.dataSize; 
		// チェックサム検査を実行
		if (CMN_Checksum(&pstRecv->stReqFrm, pstRecv->stReqFrm.checksum, reqFrmSize)) {
			// チェックサム検査に合格した場合
			pstReqFrm = &pstRecv->stReqFrm; // 戻り値に要求フレームのポインタを設定
		}
	}

	return pstReqFrm;
}

// 応答フレームのUSB送信
void FRM_MakeAndSendResFrm(USHORT seqNo, USHORT cmd, USHORT errCode, USHORT dataSize, PVOID pBuf)
{
	ULONG frmSize;        			// 応答フレームのサイズ(チェックサム除く)
	UCHAR* pDataAry = (UCHAR*)pBuf;	// 応答フレームのデータ部
	ST_FRM_RES_FRAME stResFrm; 		// 応答フレーム

	// 応答フレームを作成
	stResFrm.header   = FRM_HEADER_RES;	// ヘッダ
	stResFrm.seqNo    = seqNo; 			// シーケンス番号
	stResFrm.cmd      = cmd;   			// コマンド
	stResFrm.errCode  = errCode;       	// エラーコード
	stResFrm.dataSize = dataSize;      	// データサイズ	
	// データ
	if ((FRM_ERR_SUCCESS == errCode) && (pDataAry != NULL) && (dataSize > 0)) { 
		memcpy(stResFrm.aData, pDataAry, dataSize);
	}
	// 応答フレームのサイズ(チェックサム除く)を計算
	frmSize = offsetof(ST_FRM_RES_FRAME, dataSize) + sizeof(stResFrm.dataSize) + stResFrm.dataSize;  
	// チェックサムを計算 
	stResFrm.checksum = CMN_CalcChecksum(&stResFrm, frmSize); 
	
	// USB送信
	Serial.write((UCHAR*)&stResFrm, frmSize); // ヘッダ部～データ部 ※データ部:aData[]メンバについてはfrmSize分だけが送信対象
	Serial.write((UCHAR*)&stResFrm.checksum, sizeof(stResFrm.checksum)); // チェックサム部
}

// USB通信を初期化
void FRM_Init()
{
	// 変数を初期化
	f_stRecvDataInf.p = (UCHAR*)&(f_stRecvDataInf.stReqFrm);

	// CDCを初期化
	Serial.begin(SERIAL_BAUDRATE);	
}




