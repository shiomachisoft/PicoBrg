#ifndef GPIO_H
#define GPIO_H

#include "Common.h"

// [define]
// GP番号
#define GP_00 0     // UART0 TX
#define GP_01 1     // UART0 RX
#define GP_02 2     // GPIO
#define GP_03 3     // GPIO
#define GP_25 25    // ONBOARD LED

// ピン(GP番号)の機能選択
#define UART_TX     GP_00
#define UART_RX     GP_01
#define ONBOARD_LED GP_25

#pragma pack(1)

// [構造体]
// GPIOのGP番号と方向
typedef struct _ST_GPIO_PIN{
    ULONG gp;  // GP番号
    bool  dir; // true:出力 false:入力
} ST_GPIO_PIN;

// GPIO設定
typedef struct _ST_GPIO_CONFIG {
   ULONG pullDownBits;   // プルダウンかプルアップか 
   ULONG initialValBits; // 電源ON時出力値
} ST_GPIO_CONFIG;

#pragma pack()

// [関数プロトタイプ宣言]
ULONG GPIO_GetInDirBits();
ULONG GPIO_GetOutDirBits();
void GPIO_SetDefault(ST_GPIO_CONFIG *pstConfig);
void GPIO_Init(ST_GPIO_CONFIG *pstConfig);

#endif