// Copyright © 2024 Shiomachi Software. All rights reserved.
#include "Common.h"

// [define] / [マクロ定義]
#define CMD_WAIT_SEND_END 1000 // Wait time for sending response frame before resetting after writing config data to FLASH (ms) / FLASHに設定データを書き込んでリセットする前の応答フレームの送信完了待ち時間(ms)

// Function prototypes / [関数プロトタイプ宣言]
static void CMD_ExecReqCmd_GetFwError(ST_FRM_REQ_FRAME *pstReqFrm);
static void CMD_ExecReqCmd_ClearFwError(ST_FRM_REQ_FRAME *pstReqFrm);
static void CMD_ExecReqCmd_SetUartConfig(ST_FRM_REQ_FRAME *pstReqFrm);
static void CMD_ExecReqCmd_GetUartConfig(ST_FRM_REQ_FRAME *pstReqFrm);
static void CMD_ExecReqCmd_EraseFlash(ST_FRM_REQ_FRAME *pstReqFrm);
static void CMD_ExecReqCmd_GetFwInfo(ST_FRM_REQ_FRAME *pstReqFrm);
static void CMD_ExecReqCmd_SetNwConfig2(ST_FRM_REQ_FRAME *pstReqFrm);
static void CMD_ExecReqCmd_GetNwConfig2(ST_FRM_REQ_FRAME *pstReqFrm);

// Execute request command / 要求コマンドの実行
void CMD_ExecReqCmd(ST_FRM_REQ_FRAME *pstReqFrm)
{   
    switch (pstReqFrm->cmd) 
    {
        // Get FW info command / FW情報取得コマンド
        case CMD_GET_FW_INFO:
            CMD_ExecReqCmd_GetFwInfo(pstReqFrm);
            break;                           
        // Change UART config command / UART通信設定変更コマンド
        case CMD_SET_UART_CONFIG:
            CMD_ExecReqCmd_SetUartConfig(pstReqFrm);
            break;
        // Get UART config command / UART通信設定取得コマンド
        case CMD_GET_UART_CONFIG:
            CMD_ExecReqCmd_GetUartConfig(pstReqFrm);
            break;               
        // Get FW error command / FWエラー取得コマンド
        case CMD_GET_FW_ERR:
            CMD_ExecReqCmd_GetFwError(pstReqFrm);
            break;    
        // Clear FW error command / FWエラークリアコマンド
        case CMD_CLEAR_FW_ERR:
            CMD_ExecReqCmd_ClearFwError(pstReqFrm);
            break; 
        // Erase FLASH command / FLASH消去コマンド    
        case CMD_ERASE_FLASH: 
            CMD_ExecReqCmd_EraseFlash(pstReqFrm);
            break;             
        // Change network config 2 command / ネットワーク設定変更コマンド2
        case CMD_SET_NW_CONFIG2:
            CMD_ExecReqCmd_SetNwConfig2(pstReqFrm);
            break;    
        // Get network config 2 command / ネットワーク設定取得コマンド2
        case CMD_GET_NW_CONFIG2:
            CMD_ExecReqCmd_GetNwConfig2(pstReqFrm);
            break;         
        default:
            break;       
    }
}

// Execute get FW info command / FW情報取得コマンドの実行
static void CMD_ExecReqCmd_GetFwInfo(ST_FRM_REQ_FRAME *pstReqFrm)
{
    USHORT expectedSize = 0;            // Expected value of request frame data size / 要求フレームのデータサイズの期待値
    USHORT dataSize = 0;                // Response frame data size / 応答フレームのデータサイズ
    USHORT errCode = FRM_ERR_SUCCESS;   // Error code / エラーコード 
    ST_FW_INFO stFwInfo = {0};          // FW info / FW情報
    PVOID pBuf = NULL;                  // Response frame data / 応答フレームのデータ
    
    // Check data size / データサイズをチェック
    if (pstReqFrm->dataSize != expectedSize) {
        errCode = FRM_ERR_DATA_SIZE; // Invalid data size / データサイズが不正
    }
    else { // Normal case / 正常系
        memset(&stFwInfo, 0, sizeof(stFwInfo));
        strcpy(stFwInfo.szMakerName, MAKER_NAME);     // Maker name / メーカー名
        strcpy(stFwInfo.szFwName, FW_NAME);           // FW name / FW名 
        stFwInfo.fwVer = FW_VER;                      // FW version / FWバージョン
        pico_get_unique_board_id(&stFwInfo.board_id); // Unique board ID Size = PICO_UNIQUE_BOARD_ID_SIZE_BYTES / ユニークボードID サイズ = PICO_UNIQUE_BOARD_ID_SIZE_BYTES

        dataSize = sizeof(stFwInfo); // Response frame data size / 応答フレームのデータサイズ
        pBuf = (PVOID)&stFwInfo;     // Response frame data / 応答フレームのデータ   
    }

    // Send response frame / 応答フレームを送信        
    FRM_MakeAndSendResFrm(pstReqFrm->seqNo, pstReqFrm->cmd, errCode, dataSize, pBuf);
}

// Execute change UART config command / UART通信設定変更コマンドの実行
static void CMD_ExecReqCmd_SetUartConfig(ST_FRM_REQ_FRAME *pstReqFrm)
{
    USHORT expectedSize = 0;            // Expected value of request frame data size / 要求フレームのデータサイズの期待値
    USHORT errCode = FRM_ERR_SUCCESS;   // Error code / エラーコード 
    ST_UART_CONFIG stUartConfig = {0};  // UART config / UART通信設定
    ST_FLASH_DATA stFlashData = {0};    // FLASH data / FLASHデータ

    // Check data size / データサイズをチェック
    expectedSize = sizeof(stUartConfig);
    if (pstReqFrm->dataSize != expectedSize) {
        errCode = FRM_ERR_DATA_SIZE;    // Invalid data size / データサイズが不正
    }
    else { 
        // Get arguments / 引数を取得
        memcpy(&stUartConfig, &pstReqFrm->aData[0], sizeof(stUartConfig)); // UART通信設定
        // Check arguments / 引数をチェック
        if (stUartConfig.dataBits != 8) { // Data bit length is out of range / データビット長が範囲外
            errCode = FRM_ERR_PARAM;    // Invalid parameter / 引数が不正
        }
        else if ((stUartConfig.stopBits != 1) && (stUartConfig.stopBits != 2)) { // Stop bit length is out of range / ストップビット長が範囲外
            errCode = FRM_ERR_PARAM;    // Invalid parameter / 引数が不正
        }
        else if ((stUartConfig.parity != UART_PARITY_NONE) // Parity is out of range / パリティが範囲外
            && (stUartConfig.parity != UART_PARITY_EVEN) 
            && (stUartConfig.parity != UART_PARITY_ODD)) {
            errCode = FRM_ERR_PARAM;    // Invalid parameter / 引数が不正
        }        
    }

    // Send response frame regardless of success/failure / 成功・失敗に関わらず応答フレームを送信        
    FRM_MakeAndSendResFrm(pstReqFrm->seqNo, pstReqFrm->cmd, errCode, 0, NULL);

    if (errCode == FRM_ERR_SUCCESS) { // Normal case / 正常系     
        // Wait for USB response frame send completion (read completion from host side) / USBの応答フレーム送信完了(ホスト側からの読み取り完了)を待つ
        busy_wait_ms(CMD_WAIT_SEND_END);
        // [Write to FLASH] / [FLASHへ書き込み]
        // Read FLASH data / FLASHデータ読み込み
        FLASH_Read(&stFlashData);
        // Write to FLASH / FLASHへ書き込み
        memcpy(&stFlashData.stUartConfig, &stUartConfig, sizeof(stUartConfig));
        FLASH_Write(&stFlashData); // Microcontroller will be reset / マイコンはリセットされる       
    }
}

// Execute get UART config command / UART通信設定取得コマンドの実行
static void CMD_ExecReqCmd_GetUartConfig(ST_FRM_REQ_FRAME *pstReqFrm)
{
    USHORT expectedSize = 0;            // Expected value of request frame data size / 要求フレームのデータサイズの期待値
    USHORT dataSize = 0;                // Response frame data size / 応答フレームのデータサイズ
    USHORT errCode = FRM_ERR_SUCCESS;   // Error code / エラーコード 
    ST_FLASH_DATA *pstFlashData = NULL; // FLASH data at power on / 電源起動時のFLASHデータ
    PVOID pBuf = NULL;                  // Response frame data / 応答フレームのデータ
    
    // Check data size / データサイズをチェック
    if (pstReqFrm->dataSize != expectedSize) {
        errCode = FRM_ERR_DATA_SIZE; // Invalid data size / データサイズが不正
    }
    else { // Normal case / 正常系
        // Get FLASH data at power on / 電源起動時のFLASHデータを取得
        pstFlashData = FLASH_GetDataAtPowerOn();

        dataSize = sizeof(pstFlashData->stUartConfig);  // Response frame data size / 応答フレームのデータサイズ
        pBuf = (PVOID)&pstFlashData->stUartConfig;      // Response frame data / 応答フレームのデータ
    }

    // Send response frame / 応答フレームを送信        
    FRM_MakeAndSendResFrm(pstReqFrm->seqNo, pstReqFrm->cmd, errCode, dataSize, pBuf);
}

// Execute get FW error command / FWエラー取得コマンドの実行
static void CMD_ExecReqCmd_GetFwError(ST_FRM_REQ_FRAME *pstReqFrm)
{
    USHORT expectedSize = 0;            // Expected value of request frame data size / 要求フレームのデータサイズの期待値
    USHORT dataSize = 0;                // Response frame data size / 応答フレームのデータサイズ
    USHORT errCode = FRM_ERR_SUCCESS;   // Error code / エラーコード 
    ULONG errorBits = 0;                // FW error / FWエラー
    PVOID pBuf = NULL;                  // Data part of response frame / 応答フレームのデータ部

    // Check data size / データサイズをチェック
    if (pstReqFrm->dataSize != expectedSize) {
        errCode = FRM_ERR_DATA_SIZE; // Invalid data size / データサイズが不正
    }
    else { // Normal case / 正常系
        // Get FW error / FWエラーを取得
        errorBits = CMN_GetFwErrorBits();        

        dataSize = sizeof(errorBits); // Response frame data size / 応答フレームのデータサイズ
        pBuf = (PVOID)&errorBits;     // Response frame data / 応答フレームのデータ 
    }

    // Send response frame / 応答フレームを送信        
    FRM_MakeAndSendResFrm(pstReqFrm->seqNo, pstReqFrm->cmd, errCode, dataSize, pBuf); 
}

// Execute clear FW error command / FWエラークリアコマンドの実行
static void CMD_ExecReqCmd_ClearFwError(ST_FRM_REQ_FRAME *pstReqFrm)
{
    USHORT expectedSize = 0;            // Expected value of request frame data size / 要求フレームのデータサイズの期待値
    USHORT errCode = FRM_ERR_SUCCESS;   // Error code / エラーコード 

    // Check data size / データサイズをチェック
    if (pstReqFrm->dataSize != expectedSize) {
        errCode = FRM_ERR_DATA_SIZE; // Invalid data size / データサイズが不正
    }
    else { // Normal case / 正常系
        // Clear FW error / FWエラークリア
        CMN_ClearFwErrorBits(true);
    }

    // Send response frame / 応答フレームを送信        
    FRM_MakeAndSendResFrm(pstReqFrm->seqNo, pstReqFrm->cmd, errCode, 0, NULL); 
}

// Execute erase FLASH command / FLASH消去コマンドの実行
static void CMD_ExecReqCmd_EraseFlash(ST_FRM_REQ_FRAME *pstReqFrm)
{
    USHORT expectedSize = 0;            // Expected value of request frame data size / 要求フレームのデータサイズの期待値
    USHORT errCode = FRM_ERR_SUCCESS;   // Error code / エラーコード 

    // Check data size / データサイズをチェック
    if (pstReqFrm->dataSize != expectedSize) {
        errCode = FRM_ERR_DATA_SIZE;    // Invalid data size / データサイズが不正
    }

    // Send response frame regardless of success/failure / 成功・失敗に関わらず応答フレームを送信        
    FRM_MakeAndSendResFrm(pstReqFrm->seqNo, pstReqFrm->cmd, errCode, 0, NULL); 

    if (errCode == FRM_ERR_SUCCESS) { // Normal case / 正常系
        // Wait for USB response frame send completion (read completion from host side) / USBの応答フレーム送信完了(ホスト側からの読み取り完了)を待つ
        busy_wait_ms(CMD_WAIT_SEND_END);         
        // Erase FLASH / FLASH消去
        FLASH_Erase(); // Microcontroller will be reset / マイコンはリセットされる   
    }      
}

// Execute change network config 2 command / ネットワーク設定変更コマンド2の実行
static void CMD_ExecReqCmd_SetNwConfig2(ST_FRM_REQ_FRAME *pstReqFrm)
{
    USHORT expectedSize = 0;            // Expected value of request frame data size / 要求フレームのデータサイズの期待値
    USHORT errCode = FRM_ERR_SUCCESS;   // Error code / エラーコード 
    ST_NW_CONFIG2 stNwConfig = {0};     // Network config / ネットワーク設定
    ST_FLASH_DATA stFlashData = {0};    // FLASH data / FLASHデータ

    // Check data size / データサイズをチェック
    expectedSize = sizeof(stNwConfig);
    if (pstReqFrm->dataSize != expectedSize) {
        errCode = FRM_ERR_DATA_SIZE;    // Invalid data size / データサイズが不正
    }
    else { // Normal case / 正常系
        // Get arguments / 引数を取得
        memcpy(&stNwConfig, &pstReqFrm->aData[0], sizeof(stNwConfig)); // ネットワーク設定
    }

    // Send response frame regardless of success/failure / 成功・失敗に関わらず応答フレームを送信        
    FRM_MakeAndSendResFrm(pstReqFrm->seqNo, pstReqFrm->cmd, errCode, 0, NULL);

    if (errCode == FRM_ERR_SUCCESS) { // Normal case / 正常系
        // Wait for USB response frame send completion (read completion from host side) / USBの応答フレーム送信完了(ホスト側からの読み取り完了)を待つ
        busy_wait_ms(CMD_WAIT_SEND_END);
        // [Write to FLASH] / [FLASHへ書き込み]
        // Read FLASH data / FLASHデータ読み込み
        FLASH_Read(&stFlashData);
        // Write to FLASH / FLASHへ書き込み
        memcpy(&stFlashData.stNwConfig, &stNwConfig, sizeof(stNwConfig));
        FLASH_Write(&stFlashData); // Microcontroller will be reset / マイコンはリセットされる   
    }      
}

// Execute get network config 2 command / ネットワーク設定取得コマンド2の実行
static void CMD_ExecReqCmd_GetNwConfig2(ST_FRM_REQ_FRAME *pstReqFrm)
{
    USHORT expectedSize = 0;            // Expected value of request frame data size / 要求フレームのデータサイズの期待値
    USHORT dataSize = 0;                // Response frame data size / 応答フレームのデータサイズ
    USHORT errCode = FRM_ERR_SUCCESS;   // Error code / エラーコード 
    ST_FLASH_DATA *pstFlashData = NULL; // FLASH data at power on / 電源起動時のFLASHデータ
    PVOID pBuf = NULL;                  // Data part of response frame / 応答フレームのデータ部
    
    // Check data size / データサイズをチェック
    if (pstReqFrm->dataSize != expectedSize) {
        errCode = FRM_ERR_DATA_SIZE; // Invalid data size / データサイズが不正
    }
    else { // Normal case / 正常系
        // Get FLASH data at power on / 電源起動時のFLASHデータを取得
        pstFlashData = FLASH_GetDataAtPowerOn();

        dataSize = sizeof(pstFlashData->stNwConfig); // Response frame data size / 応答フレームのデータサイズ
        pBuf = (PVOID)&pstFlashData->stNwConfig;     // Response frame data / 応答フレームのデータ     
    }

    // Send response frame / 応答フレームを送信        
    FRM_MakeAndSendResFrm(pstReqFrm->seqNo, pstReqFrm->cmd, errCode, dataSize, pBuf);    
}