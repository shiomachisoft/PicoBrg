#include "Common.h"

// [define]
#define CMD_WAIT_SEND_END 1000 // FLASHに設定データを書き込んでリセットする前の応答フレームの送信完了待ち時間(ms)

// [関数プロトタイプ宣言]
static void CMD_ExecReqCmd_SetUartConfig(ST_FRM_REQ_FRAME *pstReqFrm);
static void CMD_ExecReqCmd_GetUartConfig(ST_FRM_REQ_FRAME *pstReqFrm);
static void CMD_ExecReqCmd_GetFwError(ST_FRM_REQ_FRAME *pstReqFrm);
static void CMD_ExecReqCmd_ClearFwError(ST_FRM_REQ_FRAME *pstReqFrm);
static void CMD_ExecReqCmd_EraseFlash(ST_FRM_REQ_FRAME *pstReqFrm);
static void CMD_ExecReqCmd_GetFwInfo(ST_FRM_REQ_FRAME *pstReqFrm);
static void CMD_ExecReqCmd_SetNwConfig2(ST_FRM_REQ_FRAME *pstReqFrm);
static void CMD_ExecReqCmd_GetNwConfig2(ST_FRM_REQ_FRAME *pstReqFrm);

// 要求コマンド実行
void CMD_ExecReqCmd(ST_FRM_REQ_FRAME *pstReqFrm)
{   
    // 要求コマンドの実行
    switch (pstReqFrm->cmd) 
    {
    // FW情報取得コマンド
    case CMD_GET_FW_INFO:
        CMD_ExecReqCmd_GetFwInfo(pstReqFrm);
        break;                           
    // UART通信設定変更コマンド
    case CMD_SET_UART_CONFIG:
        CMD_ExecReqCmd_SetUartConfig(pstReqFrm);
        break;
    // UART通信設定取得コマンド
    case CMD_GET_UART_CONFIG:
        CMD_ExecReqCmd_GetUartConfig(pstReqFrm);
        break;               
    // FWエラー取得コマンド
    case CMD_GET_FW_ERR:
        CMD_ExecReqCmd_GetFwError(pstReqFrm);
        break;    
    // FWエラークリアコマンド
    case CMD_CLEAR_FW_ERR:
        CMD_ExecReqCmd_ClearFwError(pstReqFrm);
        break; 
    // FLASH消去コマンド    
    case CMD_ERASE_FLASH: 
        CMD_ExecReqCmd_EraseFlash(pstReqFrm);
        break;             
    // ネットワーク設定変更コマンド2
    case CMD_SET_NW_CONFIG2:
        CMD_ExecReqCmd_SetNwConfig2(pstReqFrm);
        break;    
    // ネットワーク設定取得コマンド2
    case CMD_GET_NW_CONFIG2:
        CMD_ExecReqCmd_GetNwConfig2(pstReqFrm);
        break;         
    default:
        break;       
    }
}

// FW情報取得コマンドの実行
static void CMD_ExecReqCmd_GetFwInfo(ST_FRM_REQ_FRAME *pstReqFrm)
{
    USHORT dataSize = 0;                // データサイズの期待値
    USHORT errCode = FRM_ERR_SUCCESS;   // エラーコード 
    ST_FW_INFO stFwInfo;                // FW情報
    
    // データサイズをチェック
    if (pstReqFrm->dataSize != dataSize) {
        errCode = FRM_ERR_DATA_SIZE; // データサイズが不正
    }
    else { // 正常系
        memset(&stFwInfo, 0, sizeof(stFwInfo));
        strcpy(stFwInfo.szMakerName, MAKER_NAME);     // メーカー名
        strcpy(stFwInfo.szFwName, FW_NAME);           // FW名 
        stFwInfo.fwVer = FW_VER;                      // FWバージョン
        pico_get_unique_board_id(&stFwInfo.board_id); // ユニークボードID サイズ = PICO_UNIQUE_BOARD_ID_SIZE_BYTES       
    }

    // 応答フレームを送信        
    FRM_MakeAndSendResFrm(pstReqFrm->seqNo, pstReqFrm->cmd, errCode, sizeof(stFwInfo), &stFwInfo);
}

// UART通信設定変更コマンドの実行
static void CMD_ExecReqCmd_SetUartConfig(ST_FRM_REQ_FRAME *pstReqFrm)
{
    USHORT dataSize = 0;                // データサイズの期待値
    USHORT errCode = FRM_ERR_SUCCESS;   // エラーコード 
    ST_UART_CONFIG stUartConfig;        // UART通信設定
    ST_FLASH_DATA stFlashData;          // FLASHデータ

    // データサイズをチェック
    dataSize = sizeof(stUartConfig);
    if (pstReqFrm->dataSize != dataSize) {
        errCode = FRM_ERR_DATA_SIZE; // データサイズが不正
    }
    else { 
        // 引数を取得
        memcpy(&stUartConfig, &pstReqFrm->aData[0], sizeof(stUartConfig)); // UART通信設定
        // 引数をチェック
        if (stUartConfig.dataBits != 8) { // データビット長が範囲外
            errCode = FRM_ERR_PARAM; // 引数が不正
        }
        else if ((stUartConfig.stopBits != 1) && (stUartConfig.stopBits != 2)) { // ストップビット長が範囲外
            errCode = FRM_ERR_PARAM; // 引数が不正
        }
        else if ((stUartConfig.parity != UART_PARITY_NONE) // パリティが範囲外
            && (stUartConfig.parity != UART_PARITY_EVEN) 
            && (stUartConfig.parity != UART_PARITY_ODD)) {
            errCode = FRM_ERR_PARAM; // 引数が不正
        }        
        else { // 正常系     
            // 成功の応答フレームを送信 
            FRM_MakeAndSendResFrm(pstReqFrm->seqNo, pstReqFrm->cmd, errCode, 0, NULL);
            // CPUコア1が応答フレームを送信するのを待つ
            busy_wait_ms(CMD_WAIT_SEND_END);
            // [FLASHへ書き込み]
            // FLASHデータ読み込み
            FLASH_Read(&stFlashData);
            // 書き込みデータを用意
            memcpy(&stFlashData.stUartConfig, &stUartConfig, sizeof(stUartConfig));
            // FLASHへ書き込み  
            FLASH_Write(&stFlashData); // この関数の中でリセットされる       
        }
    }

    // 失敗の応答フレームを送信        
    FRM_MakeAndSendResFrm(pstReqFrm->seqNo, pstReqFrm->cmd, errCode, 0, NULL);
}

// UART通信設定取得コマンドの実行
static void CMD_ExecReqCmd_GetUartConfig(ST_FRM_REQ_FRAME *pstReqFrm)
{
    USHORT dataSize = 0;                // データサイズの期待値
    USHORT errCode = FRM_ERR_SUCCESS;   // エラーコード 
    ST_FLASH_DATA *pstFlashData = NULL; // 電源起動時のFLASHデータ
    
    // データサイズをチェック
    if (pstReqFrm->dataSize != dataSize) {
        errCode = FRM_ERR_DATA_SIZE; // データサイズが不正
    }
    else { // 正常系
        // 電源起動時のFLASHデータを取得
        pstFlashData = FLASH_GetDataAtPowerOn();
    }

    // 応答フレームを送信        
    FRM_MakeAndSendResFrm(pstReqFrm->seqNo, pstReqFrm->cmd, errCode, sizeof(pstFlashData->stUartConfig), &pstFlashData->stUartConfig);
}

// FWエラー取得コマンドの実行
static void CMD_ExecReqCmd_GetFwError(ST_FRM_REQ_FRAME *pstReqFrm)
{
    USHORT dataSize = 0;                // データサイズの期待値
    USHORT errCode = FRM_ERR_SUCCESS;   // エラーコード 
    ULONG errorBits = 0;                // FWエラー

    // データサイズをチェック
    if (pstReqFrm->dataSize != dataSize) {
        errCode = FRM_ERR_DATA_SIZE; // データサイズが不正
    }
    else { // 正常系
        // FWエラーを取得
        errorBits = CMN_GetFwErrorBits(true);        
    }

    // 応答フレームを送信        
    FRM_MakeAndSendResFrm(pstReqFrm->seqNo, pstReqFrm->cmd, errCode, sizeof(errorBits), &errorBits); 
}

// FWエラークリアコマンドの実行
static void CMD_ExecReqCmd_ClearFwError(ST_FRM_REQ_FRAME *pstReqFrm)
{
    USHORT dataSize = 0;                // データサイズの期待値
    USHORT errCode = FRM_ERR_SUCCESS;   // エラーコード 

    // データサイズをチェック
    if (pstReqFrm->dataSize != dataSize) {
        errCode = FRM_ERR_DATA_SIZE; // データサイズが不正
    }
    else { // 正常系
        // FWエラークリア
        CMN_ClearFwErrorBits(true);
    }

    // 応答フレームを送信        
    FRM_MakeAndSendResFrm(pstReqFrm->seqNo, pstReqFrm->cmd, errCode, 0, NULL); 
}

// FLASH消去コマンドの実行
static void CMD_ExecReqCmd_EraseFlash(ST_FRM_REQ_FRAME *pstReqFrm)
{
    USHORT dataSize = 0;                // データサイズの期待値
    USHORT errCode = FRM_ERR_SUCCESS;   // エラーコード 

    // データサイズをチェック
    if (pstReqFrm->dataSize != dataSize) {
        errCode = FRM_ERR_DATA_SIZE;    // データサイズが不正
    }
    else { // 正常系
        // 成功の応答フレームを送信 
        FRM_MakeAndSendResFrm(pstReqFrm->seqNo, pstReqFrm->cmd, errCode, 0, NULL);   
        // CPUコア1が応答フレームを送信するのを待つ
        busy_wait_ms(CMD_WAIT_SEND_END);         
        // FLASH消去
        FLASH_Erase();
    }

    // 失敗の応答フレームを送信        
    FRM_MakeAndSendResFrm(pstReqFrm->seqNo, pstReqFrm->cmd, errCode, 0, NULL);        
}

// ネットワーク設定変更コマンドの実行
static void CMD_ExecReqCmd_SetNwConfig2(ST_FRM_REQ_FRAME *pstReqFrm)
{
    USHORT dataSize = 0;                // データサイズの期待値
    USHORT errCode = FRM_ERR_SUCCESS;   // エラーコード 
    ST_NW_CONFIG2 stNwConfig;            // ネットワーク設定
    ST_FLASH_DATA stFlashData;          // FLASHデータ

    // データサイズをチェック
    dataSize = sizeof(stNwConfig);
    if (pstReqFrm->dataSize != dataSize) {
        errCode = FRM_ERR_DATA_SIZE; // データサイズが不正
    }
    else { // 正常系
        // 引数を取得
        memcpy(&stNwConfig, &pstReqFrm->aData[0], sizeof(stNwConfig)); // ネットワーク設定
        // 成功の応答フレームを送信 
        FRM_MakeAndSendResFrm(pstReqFrm->seqNo, pstReqFrm->cmd, errCode, 0, NULL);
        // CPUコア1が応答フレームを送信するのを待つ
        busy_wait_ms(CMD_WAIT_SEND_END);
        // [FLASHへ書き込み]
        // FLASHデータ読み込み
        FLASH_Read(&stFlashData);
        // 書き込みデータを用意
        memcpy(&stFlashData.stNwConfig, &stNwConfig, sizeof(stNwConfig));
        // FLASHへ書き込み  
        FLASH_Write(&stFlashData); // この関数の中でリセットされる 
    }

    // 失敗の応答フレームを送信        
    FRM_MakeAndSendResFrm(pstReqFrm->seqNo, pstReqFrm->cmd, errCode, 0, NULL);       
}

// ネットワーク設定取得コマンドの実行
static void CMD_ExecReqCmd_GetNwConfig2(ST_FRM_REQ_FRAME *pstReqFrm)
{
    USHORT dataSize = 0;                // データサイズの期待値
    USHORT errCode = FRM_ERR_SUCCESS;   // エラーコード 
    ST_FLASH_DATA *pstFlashData = NULL; // 電源起動時のFLASHデータ
    
    // データサイズをチェック
    if (pstReqFrm->dataSize != dataSize) {
        errCode = FRM_ERR_DATA_SIZE; // データサイズが不正
    }
    else { // 正常系
        // 電源起動時のFLASHデータを取得
        pstFlashData = FLASH_GetDataAtPowerOn();
    }

    // 応答フレームを送信        
    FRM_MakeAndSendResFrm(pstReqFrm->seqNo, pstReqFrm->cmd, errCode, sizeof(pstFlashData->stNwConfig), &pstFlashData->stNwConfig);    
}