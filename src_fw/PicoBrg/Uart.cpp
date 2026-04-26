// Copyright © 2024 Shiomachi Software. All rights reserved.
#include "Common.h"

// [define] / [マクロ定義]
#define UART_ID  uart0      // UART ID / UARTのID
#define UART_IRQ UART0_IRQ  // UART IRQ / UARTのIRQ

// Default values / デフォルト値
#define UART_DEFAULT_BAUD_RATE 9600             // Baud rate / ボーレート
#define UART_DEFAULT_DATA_BITS 8                // Data bit length / データビット長
#define UART_DEFAULT_STOP_BITS 1                // Stop bit length / ストップビット長
#define UART_DEFAULT_PARITY    UART_PARITY_NONE // Parity / パリティ

// File scope variables / [ファイルスコープ変数]
static volatile bool f_isSentFirstByte= false; // Whether 1st byte has been sent / 1byte目を送信済みか否か
static volatile bool f_hasPendingData = false; // Whether there is pending wireless send data / 無線送信保留中のデータがあるか否か
static volatile UCHAR f_pendingData = 0;       // Pending wireless send data / 無線送信保留中のデータ

// Function prototypes / [関数プロトタイプ宣言]
static void UART_Interrupt();
static void UART_Recv();
static void UART_SendFirstByte();
static inline bool UART_Send();

// UART interrupt / UART割り込み
static void UART_Interrupt() 
{
    io_rw_32 dr;  // Data Register:UARTDR
    io_rw_32 mis = uart_get_hw(UART_ID)->mis; // Masked interrupt status register / マスク済み割り込みステータスレジスタ
    UCHAR errFlags; // UART error flag / UARTエラーフラグ
	ULONG errorBits = 0;   

    if (mis & (UART_UARTMIS_RXMIS_BITS | UART_UARTMIS_RTMIS_BITS)) {
        // If UART receive interrupt or receive timeout interrupt / UART受信割り込み、または受信タイムアウト割り込みの場合

        while (uart_is_readable(UART_ID)) { // If UART receive data still exists / UARTの受信データがまだ存在する場合
            // Extract UART receive data (1byte) and error flag simultaneously / UARTの受信データ(1byte)とエラーフラグを同時に取り出し
            dr = uart_get_hw(UART_ID)->dr;
            errFlags = (dr >> 8) & 0x0F; // Extract error flag directly from bits 8-11 of dr / drの8～11ビット目からエラーフラグを直接抽出

            if (errFlags) {
                // If an error has occurred, clear it (writing any value clears all errors by spec) / エラーが発生している場合はクリア(任意の値を書き込むことで全エラーがクリアされる仕様)
                uart_get_hw(UART_ID)->rsr = 0;
            }
  
            if (!errFlags) { // If no UART error occurred / UARTエラーが発生していない場合
                // Extract lower 8 bits of data / 下位8ビットのデータを抽出
                UCHAR rxData = (UCHAR)(dr & 0xFF);
                // Enqueue 1 byte of UART receive data / UART受信データ1byteのエンキュー
                (void)CMN_Enqueue(CMN_QUE_KIND_UART_RECV, (PVOID)&rxData, true);
            }

            if (TMR_IsStabilizationWaitTimePassed()) { 
                // If stabilization wait time after startup has passed / 起動してからの安定待ち時間が経過していた場合

                // Set FW error / FWエラーを設定
                if (errFlags & (1 << 0)) {
                    errorBits |= CMN_ERR_BIT_UART_FRAMING_ERR;
                }        
                if (errFlags & (1 << 1)) {
                    errorBits |= CMN_ERR_BIT_UART_PARITY_ERR;
                } 
                if (errFlags & (1 << 2)) {
                    errorBits |= CMN_ERR_BIT_UART_BREAK_ERR;
                } 
                if (errFlags & (1 << 3)) {
                    errorBits |= CMN_ERR_BIT_UART_OVERRUN_ERR;
                }                         
            }
        }
        // Set FW error / FWエラーを設定
        CMN_SetErrorBits(errorBits, true);
    }

    if (mis & UART_UARTMIS_TXMIS_BITS) {
        // If UART send interrupt / UART送信割り込みの場合
   
        // UART send next 1 byte / 次の1byteのUART送信
        if (!UART_Send()) { 
            // If UART send data queue is empty (no more UART send data) / UART送信データのキューが空の場合(UART送信データがもう無い場合) 
            
            f_isSentFirstByte = false; // 1st byte is unsent / 1byte目は未送信   

            // Clear UART send interrupt / UART送信割り込みをクリア
            uart_get_hw(UART_ID)->icr = UART_UARTICR_TXIC_BITS ;

            // -------------------------------------------------------------
            // [Note] *Specification when FIFO is disabled (depth is 1) / [備考] ※FIFOが無効(深さが1)の場合の仕様
            //
            // - UART send interrupt (UARTTXINTR) / ・UART送信割り込み (UARTTXINTR)
            //   If there is no send data in the transmit FIFO, the send interrupt is asserted HIGH. /   送信FIFOに送信データが存在しない場合は、送信割り込みが HIGH にアサートされる。
            //   Writing to the transmit FIFO once or clearing the interrupt clears the send interrupt. /   送信FIFOに1回の書き込みを実行するか、割り込みをクリアすると、送信割り込みがクリアされる。
            //
            // - UART receive interrupt (UARTRXINTR) / ・UART受信割り込み (UARTRXINTR)
            //   Cleared just by reading the receive FIFO. /   受信FIFOの読み取りだけでクリアされる。
            // -------------------------------------------------------------
        } 
    }
}

// UART main process / UARTメイン処理
void UART_Main()
{
    // Extract UART receive data ⇒ Wireless send / UART受信データ取り出し⇒無線送信
    UART_Recv();
    // UART send 1st byte / 1byte目のUART送信
    UART_SendFirstByte(); 
}

// Extract UART receive data ⇒ Wireless send / UART受信データ取り出し⇒無線送信
static void UART_Recv()
{
    while (true) {
        CMN_EntrySpinLock(); // Acquire spin lock / スピンロックを獲得

        // If there is no pending data, try to dequeue new data / 未送信のデータがない場合、新しくデキューを試みる
        if (!f_hasPendingData) {
            UCHAR temp;
            if (CMN_Dequeue(CMN_QUE_KIND_UART_RECV, (PVOID)&temp, false)) {
                f_pendingData = temp;
                f_hasPendingData = true;
            }
        }

        // If there is data to send / 送信すべきデータがある場合
        if (f_hasPendingData) {
            UCHAR temp = f_pendingData;
            // Enqueue to wireless send queue / 無線送信キューにエンキュー
            if (CMN_Enqueue(CMN_QUE_KIND_WL_SEND, (PVOID)&temp, false)) {
                f_hasPendingData = false; // Enqueue success / エンキュー成功
            } else {
                CMN_ExitSpinLock(); // Release spin lock / スピンロックを解放
                break; // Wireless send queue is full (unsent data will be resent in next loop) / 無線送信キューが満杯(未送信データは次回ループで再送)
            }
        } else {
            CMN_ExitSpinLock(); // Release spin lock / スピンロックを解放
            break; // No data to process / 処理するデータなし
        }
        
        CMN_ExitSpinLock(); // Release spin lock / スピンロックを解放
    }
}

// Clear pending UART receive data / 保留中のUART受信データをクリアする
void UART_ClearPendingData()
{
    CMN_EntrySpinLock(); // Acquire spin lock / スピンロックを獲得
    f_hasPendingData = false;
    f_pendingData = 0;
    CMN_ExitSpinLock(); // Release spin lock / スピンロックを解放
}

// UART send 1st byte / 1byte目のUART送信
static void UART_SendFirstByte()
{
    if (!f_isSentFirstByte) {  // If 1st byte is unsent / 1byte目が未送信の場合
        if (uart_is_writable(UART_ID)) { // If UART send is possible / UART送信可能な場合
            // Extract UART send data ⇒ UART send / UART送信データ取り出し⇒UART送信
            (void)UART_Send(); 
        }
    }
}

// Extract UART send data ⇒ UART send / UART送信データ取り出し⇒UART送信
static inline bool UART_Send()
{
    UCHAR data;
    bool isSent = false; // Whether sent / 送信したか否か

     // Dequeue 1 byte of UART send data / UART送信データ1byteのデキュー
    if (CMN_Dequeue(CMN_QUE_KIND_UART_SEND, &data, true)) {   
        f_isSentFirstByte = true; // 1 byte sent / 1byteを送信済み
        // UART send (1byte) / UART送信(1byte)
        uart_get_hw(UART_ID)->dr = (io_rw_32)data;
        isSent = true;
    }

    return isSent;
}

// Store default values in ST_UART_CONFIG structure / ST_UART_CONFIG構造体にデフォルト値を格納
void UART_SetDefault(ST_UART_CONFIG *pstConfig)
{
    pstConfig->baudrate = UART_DEFAULT_BAUD_RATE; // Baud rate / ボーレート
    pstConfig->dataBits = UART_DEFAULT_DATA_BITS; // Data bit length / データビット長
    pstConfig->stopBits = UART_DEFAULT_STOP_BITS; // Stop bit length / ストップビット長
    pstConfig->parity   = (uart_parity_t)UART_DEFAULT_PARITY;    // Parity / パリティ
}

// Initialize UART / UARTを初期化
void UART_Init(ST_UART_CONFIG *pstConfig)
{
    // Initialize UART / UARTを初期化
    uart_init(UART_ID, pstConfig->baudrate);  // Baud rate / ボーレート
    // Pin function setting / ピンの機能設定
    gpio_set_function(UART_TX, GPIO_FUNC_UART);
    gpio_set_function(UART_RX, GPIO_FUNC_UART);
    // Disable CTS/RTS / CTS/RTSを無効に設定
    uart_set_hw_flow(UART_ID, false, false);
    // Communication settings / 通信設定
    uart_set_format(UART_ID, pstConfig->dataBits, pstConfig->stopBits, (uart_parity_t)pstConfig->parity);
    // Disable UART FIFO to match sample program / サンプルプログラムに合わせてUARTのFIFOを無効に設定
    uart_set_fifo_enabled(UART_ID, false);
    // Interrupt setting / 割り込み設定
    irq_set_exclusive_handler(UART_IRQ, UART_Interrupt);
    irq_set_priority(UART_IRQ, CMN_IRQ_PRIORITY_UART); // Interrupt priority / 割り込みの優先度
    irq_set_enabled(UART_IRQ, true); // Enable specified interrupt on executing CPU core / 実行中のCPUコア上の指定の割り込みを有効   
    uart_set_irq_enables(UART_ID, true, true); // Enable UART RX/TX interrupts / UARTのRX・TX割り込みを有効
}
