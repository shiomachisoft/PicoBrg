// Copyright © 2024 Shiomachi Software. All rights reserved.
#ifndef FRAME_H
#define FRAME_H

#include "Common.h"

// [define] / [マクロ定義]
#define FRM_DATA_MAX_SIZE 1024 // Size of data part buffer in frame. Data part of any command must be within this size. / フレーム中のデータ部のバッファのサイズ。どのコマンドのデータ部もこのサイズ以下であること。

// Enumerations / [列挙体]
// Frame header / フレームのヘッダ
typedef enum _E_FRM_HEADER {
    FRM_HEADER_REQ = 0xA0, // Request / 要求
    FRM_HEADER_RES         // Response / 応答
} E_FRM_HEADER;

// Command / コマンド
typedef enum _E_FRM_CMD {
    CMD_GET_FW_INFO = 0x0001,       // Get FW info / FW情報取得 
    CMD_SET_GPIO_CONFIG,            // Change GPIO config / GPIO設定変更  
    CMD_GET_GPIO_CONFIG,            // Get GPIO config / GPIO設定取得 
    CMD_GET_GPIO,                   // GPIO input / GPIO入力                      
    CMD_OUT_GPIO,                   // GPIO output / GPIO出力
    CMD_GET_ADC,                    // ADC/Temperature input / ADC・温度入力
    CMD_SET_UART_CONFIG,            // Change UART config / UART設定変更
    CMD_GET_UART_CONFIG,            // Get UART config / UART設定取得 
    CMD_SEND_UART,                  // UART send / UART送信
    CMD_SET_SPI_CONFIG,             // Change SPI config / SPI設定変更
    CMD_GET_SPI_CONFIG,             // Get SPI config / SPI設定取得
    CMD_SENDRECV_SPI,               // SPI master send/receive / SPIマスタ送受信 
    CMD_SET_I2C_CONFIG,             // Change I2C config / I2C設定変更
    CMD_GET_I2C_CONFIG,             // Get I2C config / I2C設定取得 
    CMD_SEND_I2C,                   // I2C master send / I2Cマスタ送信
    CMD_RECV_I2C,                   // I2C master receive / I2Cマスタ受信
    CMD_START_PWM,                  // Start PWM / PWM開始  
    CMD_STOP_PWM,                   // Stop PWM / PWM停止
    CMD_GET_FW_ERR,                 // Get FW error / FWエラー取得  
    CMD_CLEAR_FW_ERR,               // Clear FW error / FWエラークリア 
    CMD_ERASE_FLASH,                // Erase FLASH / FLASH消去
    CMD_SET_NW_CONFIG,              // Change network config / ネットワーク設定変更
    CMD_GET_NW_CONFIG,              // Get network config / ネットワーク設定取得
    CMD_SET_NW_CONFIG2,             // Change network config 2 / ネットワーク設定変更2
    CMD_GET_NW_CONFIG2              // Get network config 2 / ネットワーク設定取得2   
} E_FRM_CMD;

// Error code in frame / フレーム中のエラーコード
typedef enum _E_FRM_ERR {
    FRM_ERR_SUCCESS = 0x0000,    // Success / 成功
    FRM_ERR_DATA_SIZE,           // Invalid data size / データサイズが不正  
    FRM_ERR_PARAM,               // Invalid parameter / 引数が不正
    FRM_ERR_BUF_NOT_ENOUGH,      // Not enough buffer space / バッファに空きがない
    FRM_ERR_I2C_NO_DEVICE        // I2C:address not acknowledged, or, no device present. / I2C:アドレスが応答しない、またはデバイスが存在しない
} E_FRM_ERR;

#pragma pack(1)

// Structures / [構造体]

// Request frame / 要求フレーム
typedef struct _ST_FRM_REQ_FRAME {
    UCHAR   header;                     // Header (1byte) / ヘッダ(1byte)
    USHORT  seqNo;                      // Sequence number (2byte) / シーケンス番号(2byte)
    USHORT  cmd;                        // Command (2byte) / コマンド(2byte)
    USHORT  dataSize;                   // Data size (2byte) / データサイズ(2byte)
    UCHAR   aData[FRM_DATA_MAX_SIZE];   // Data / データ
    USHORT  checksum;                   // Checksum (2byte) / チェックサム(2byte)
} ST_FRM_REQ_FRAME;

// Response frame / 応答フレーム
typedef struct _ST_FRM_RES_FRAME {
    UCHAR   header;                     // Header (1byte) / ヘッダ(1byte)
    USHORT  seqNo;                      // Sequence number (2byte) / シーケンス番号(2byte)
    USHORT  cmd;                        // Command (2byte) / コマンド(2byte)
    USHORT  errCode;                    // Error code (2byte) / エラーコード(2byte) 
    USHORT  dataSize;                   // Data size (2byte) / データサイズ(2byte)
    UCHAR   aData[FRM_DATA_MAX_SIZE];   // Data / データ
    USHORT  checksum;                   // Checksum (2byte) / チェックサム(2byte)
} ST_FRM_RES_FRAME;

// Notification frame / 通知フレーム
typedef struct _ST_FRM_NOTIFY_FRAME {
    UCHAR  header;                      // Header (1byte) / ヘッダ(1byte)
    USHORT dataSize;                    // Data size (2byte) / データサイズ(2byte)
    UCHAR  aData[FRM_DATA_MAX_SIZE];    // Data / データ
    USHORT checksum;                    // Checksum (2byte) / チェックサム(2byte)    
} ST_FRM_NOTIFY_FRAME;

#pragma pack()

// USB receive data info / USB受信データ情報
typedef struct _ST_FRM_RECV_DATA_INFO {
    ULONG reqFrmSize; 		    // Received size of request frame / 要求フレームの受信済みサイズ
    ULONG recvDataSize;	        // Received size of data size part / データサイズ部の受信済みサイズ
    ULONG recvChecksum; 	    // Received size of checksum part / チェックサム部の受信済みサイズ
    ST_FRM_REQ_FRAME stReqFrm; 	// Request frame / 要求フレーム 
    UCHAR *p;		            // Request frame data storage pointer / 要求フレームデータ格納先ポインタ
} ST_FRM_RECV_DATA_INFO;

// Function prototypes / [関数プロトタイプ宣言]
void FRM_Init();
void FRM_Main();
void FRM_SendResFrm(USHORT seqNo, USHORT cmd, USHORT errCode, USHORT dataSize, PVOID pBuf);

#endif