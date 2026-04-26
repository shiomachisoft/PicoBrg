// Copyright © 2024 Shiomachi Software. All rights reserved.
#ifndef GPIO_H
#define GPIO_H

#include "Common.h"

// [define] / [マクロ定義]
// GP number / GP番号
#define GP_00 0     // UART0 TX
#define GP_01 1     // UART0 RX

// Pin (GP number) function selection / ピン(GP番号)の機能選択
#define UART_TX     GP_00
#define UART_RX     GP_01

#endif