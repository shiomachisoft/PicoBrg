// Copyright © 2024 Shiomachi Software. All rights reserved.
#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include <time.h>
#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "hardware/gpio.h"
#include "hardware/adc.h"
#include "hardware/uart.h"
#include "hardware/spi.h"
#include "hardware/dma.h"
#include "hardware/i2c.h"
#include "hardware/pwm.h"
#include "pico/multicore.h"
#include "hardware/flash.h"
#include "class/cdc/cdc_device.h"
#include "pico/unique_id.h"
#include "hardware/pll.h"
#include "hardware/clocks.h"
#include "hardware/structs/pll.h"
#include "hardware/structs/clocks.h"
#include "hardware/watchdog.h"
#include "hardware/resets.h"
#include "pico/bootrom.h"
#include "hardware/exception.h"

#include "pico/cyw43_arch.h"

#include "Arduino.h"
#include "WiFi.h"
#include "BTstackLib.h"
#include "ble/att_server.h"

#include "Type.h"
#include "Ver.h"
#include "Frame.h"
#include "Gpio.h"
#include "Uart.h"
#include "Wireless.h"
#include "Flash.h"
#include "Ble.h"
#include "Tcp.h"
#include "Timer.h"
#include "Cmd.h"

// [define] / [マクロ定義]
// Interrupt priority / 割り込みの優先度
#define CMN_IRQ_PRIORITY_UART 0 // UART (highest priority) / UART(最優先)

// Queue size / キューサイズ
#define CMN_QUE_DATA_MAX_WL_SEND        1024
#define CMN_QUE_DATA_MAX_UART_SEND      1024
#define CMN_QUE_DATA_MAX_UART_RECV      1024

// FW error bits / FWエラービット
#define CMN_ERR_BIT_WDT_RESET                       (1 << 0)  // Microcontroller was reset by WDT timeout / WDTタイムアウトでマイコンがリセットした
#define CMN_ERR_BIT_UART_FRAMING_ERR                (1 << 1)  // UART: Framing error / UART:Framing error     
#define CMN_ERR_BIT_UART_PARITY_ERR                 (1 << 2)  // UART: Parity error / UART:Parity error
#define CMN_ERR_BIT_UART_BREAK_ERR                  (1 << 3)  // UART: Break error / UART:Break error
#define CMN_ERR_BIT_UART_OVERRUN_ERR                (1 << 4)  // UART: Overrun error / UART:Overrun error
#define CMN_ERR_BIT_BUF_SIZE_NOT_ENOUGH_USB_WL_SEND (1 << 7)  // Request data discarded due to insufficient buffer space (USB/wireless send) / バッファに空きがないので要求データを破棄した(USB/無線送信)
#define CMN_ERR_BIT_BUF_SIZE_NOT_ENOUGH_UART_SEND   (1 << 8)  // Request data discarded due to insufficient buffer space (UART send) / バッファに空きがないので要求データを破棄した(UART送信)
#define CMN_ERR_BIT_BUF_SIZE_NOT_ENOUGH_UART_RECV   (1 << 9)  // Request data discarded due to insufficient buffer space (UART receive) / バッファに空きがないので要求データを破棄した(UART受信)
#define CMN_ERR_BIT_BUF_SIZE_NOT_ENOUGH_WL_RECV     (1 << 11) // Request data discarded due to insufficient buffer space (wireless receive) *Unused / バッファに空きがないので要求データを破棄した(無線受信) ※未使用
#define CMN_ERR_BIT_WL_SEND_ERR                     (1 << 12) // Wireless send failed / 無線送信が失敗した

// Enumerations / [列挙体]
// Queue kinds / キューの種類
typedef enum _E_CMN_QUE_KIND { 
    CMN_QUE_KIND_WL_SEND = 0,   // Wireless send / 無線送信   
    CMN_QUE_KIND_UART_SEND,     // UART send / UART送信
    CMN_QUE_KIND_UART_RECV,     // UART receive / UART受信 
    CMN_QUE_KIND_NUM            // Number of queue kinds / キューの種類の数
} E_CMN_QUE_KIND;

// Structures / [構造体]
// Queue / キュー
typedef struct _ST_QUE {
    ULONG head; // Head / 先頭
    ULONG tail; // Tail / 末尾
    ULONG max;  // Number of elements in queue data array / キューのデータ配列の要素数
    PVOID pBuf; // Pointer to queue data array / キューのデータ配列へのポインタ
} ST_QUE;

// Function prototypes / [関数プロトタイプ宣言]
bool CMN_Enqueue(ULONG iQue, PVOID pData, bool bSpinLock);
bool CMN_Dequeue(ULONG iQue, PVOID pData, bool bSpinLock);
void CMN_ClearQue(ULONG iQue, bool bSpinLock);
void CMN_EntrySpinLock();
void CMN_ExitSpinLock();
void CMN_SetErrorBits(ULONG errorBits, bool bSpinLock);
ULONG CMN_GetFwErrorBits();
void CMN_ClearFwErrorBits(bool bSpinLock);
bool CMN_Checksum(PVOID pBuf, USHORT expect, ULONG size);
USHORT CMN_CalcChecksum(PVOID pBuf, ULONG size);
void CMN_WdtEnableReboot();
void CMN_WdtRebootWithoutEnable();
void CMN_Init();

#endif
