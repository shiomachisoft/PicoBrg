// Copyright © 2024 Shiomachi Software. All rights reserved.
#include "Common.h"

// [define]
#define UART_ID  uart0      // UARTのID
#define UART_IRQ UART0_IRQ  // UARTのIRQ

// デフォルト値
#define UART_DEFAULT_BAUD_RATE 9600             // ボーレート
#define UART_DEFAULT_DATA_BITS 8                // データビット長
#define UART_DEFAULT_STOP_BITS 1                // ストップビット長
#define UART_DEFAULT_PARITY    UART_PARITY_NONE // パリティ

// [ファイルスコープ変数]
static bool f_isSentFirstByte= false; // 1byte目を送信済みか否か
static UCHAR f_aUartRecvData[CMN_QUE_DATA_MAX_UART_RECV] = {0}; // UART受信データ

// [関数プロトタイプ宣言]
static void UART_Interrupt();
static void UART_Recv();
static void UART_SendFirstbyte();
static inline bool UART_Send();

// UART割り込み
static void UART_Interrupt() 
{
    ULONG errorBits = 0;    
    io_rw_32 dr;  // Data Register:UARTDR
    io_rw_32 rsr; // Receive Status Register/Error Clear Register:UARTRSR/UARTECR
    io_rw_32 ris = uart_get_hw(UART_ID)->ris; // 割り込みステータスレジスタ

    if (ris & UART_UARTRIS_RXRIS_BITS) {
        // UART受信割り込みの場合

        while  (uart_is_readable(UART_ID)) { // UARTの受信データがまだ存在する場合
            
            // UARTの受信データ(1byte)を取り出し
            dr = uart_get_hw(UART_ID)->dr;

            // UARTエラーを取得      
            rsr = uart_get_hw(UART_ID)->rsr;
            // rsrのビット
            // 0x00000008 Overrun error
            // 0x00000004 Break error
            // 0x00000002 Parity error
            // 0x00000001 Framing error            
            rsr &= 0x0000000F;
            // UARTエラーをクリア
            uart_get_hw(UART_ID)->rsr = ~rsr;
  
            if (!rsr) { // UARTエラーが発生していない場合
                // UART受信データ1byteのエンキュー
                (void)CMN_Enqueue(CMN_QUE_KIND_UART_RECV, (PVOID)&dr, sizeof(UCHAR), true);
            }

            if (TIMER_IsStabilizationWaitTimePassed()) { 
                // 起動してからの安定待ち時間が経過していた場合

                // FWエラーを設定
                if (rsr & (1 << 0)) {
                    errorBits |= CMN_ERR_BIT_UART_FRAMING_ERR;
                }        
                if (rsr & (1 << 1)) {
                    errorBits |= CMN_ERR_BIT_UART_PARITY_ERR;
                } 
                if (rsr & (1 << 2)) {
                    errorBits |= CMN_ERR_BIT_UART_BREAK_ERR;
                } 
                if (rsr & (1 << 3)) {
                    errorBits |= CMN_ERR_BIT_UART_OVERRUN_ERR;
                }                         
                CMN_SetErrorBits(errorBits, true);
            }
        }
    }

    if (ris & UART_UARTRIS_TXRIS_BITS) {
        // UART送信割り込みの場合
   
        // 次の1byteのUART送信
        if (!UART_Send()) { 
            // UART送信データのキューが空の場合(UART送信データがもう無い場合) 
            
            f_isSentFirstByte = false; // 1byte目は未送信   

            // UART送信割り込みをクリア
            uart_get_hw(UART_ID)->icr = UART_UARTICR_TXIC_BITS ;
            // 備考:
            // ・UART送信割り込み(UARTTXINTR)について※
            //   送信FIFOに送信データが存在しない場合は、送信割り込みが HIGH にアサートされる。
            //   送信FIFOに1回の書き込みを実行するか、割り込みをクリアすると、送信割り込みがクリアされる。
            //
            // ・UART受信割り込み(UARTRXINTR)について※
            //   受信FIFOの読み取りだけでクリアされる。
            //
            // ※FIFOが無効(深さが1)の場合
        } 
    }
}

// UARTメイン処理
void UART_Main()
{
    // UART受信データ取り出し⇒無線送信
    UART_Recv();
    // 1byte目のUART送信
    UART_SendFirstbyte(); 
}

// UART受信データ取り出し⇒無線送信
static void UART_Recv()
{
    UCHAR data;
    ULONG i;
    ULONG size;

    for (i = 0; i < sizeof(f_aUartRecvData); i++) { 
        // UART受信データ1byteのデキュー
        if (CMN_Dequeue(CMN_QUE_KIND_UART_RECV, &data, sizeof(UCHAR), true)) { 
            f_aUartRecvData[i] = data;  
        }
        else {
            break;
        }
    }
    size = i;
    if (size > 0) {
        // 無線送信要求の発行
        WL_ReqToSend(f_aUartRecvData, size);
    }
}

// 1byte目のUART送信
static void UART_SendFirstbyte()
{
    if (!f_isSentFirstByte) {  // 1byte目をまだ送信済みではない場合
        if (uart_is_writable(UART_ID)) { // UART送信可能な場合
            // UART送信データ取り出し⇒UART送信
            (void)UART_Send(); 
        }
    }
}

// UART送信データ取り出し⇒UART送信
static inline bool UART_Send()
{
    UCHAR data;
    bool isSent = false; // 送信したか否か

     // UART送信データ1byteのデキュー
    if (CMN_Dequeue(CMN_QUE_KIND_UART_SEND, &data, sizeof(UCHAR), true)) {   
        f_isSentFirstByte = true; // 1byteを送信済み
        // UART送信(1byte)
        uart_get_hw(UART_ID)->dr = (io_rw_32)data;
        isSent = true;
    }

    return isSent;
}

// ST_UART_CONFIG構造体にデフォルト値を格納
void UART_SetDefault(ST_UART_CONFIG *pstConfig)
{
    pstConfig->baudrate = UART_DEFAULT_BAUD_RATE; // ボーレート
    pstConfig->dataBits = UART_DEFAULT_DATA_BITS; // データビット長
    pstConfig->stopBits = UART_DEFAULT_STOP_BITS; // ストップビット長
    pstConfig->parity   = (uart_parity_t)UART_DEFAULT_PARITY;    // パリティ
}

// UARTを初期化
void UART_Init(ST_UART_CONFIG *pstConfig)
{
    // UARTを初期化
    uart_init(UART_ID, pstConfig->baudrate);  // ボーレート
    // ピンの機能設定
    gpio_set_function(UART_TX, GPIO_FUNC_UART);
    gpio_set_function(UART_RX, GPIO_FUNC_UART);
    // CTS/RTSを無効に設定
    uart_set_hw_flow(UART_ID, false, false);
    // 通信設定
    uart_set_format(UART_ID, pstConfig->dataBits, pstConfig->stopBits, (uart_parity_t)pstConfig->parity);
    // サンプルプログラムに合わせてUARTのFIFOを無効に設定
    uart_set_fifo_enabled(UART_ID, false);
    // 割り込み設定
    irq_set_exclusive_handler(UART_IRQ, UART_Interrupt);
    irq_set_priority(UART_IRQ, CMN_IRQ_PRIORITY_UART); // 割り込みの優先度
    irq_set_enabled(UART_IRQ, true); // 実行中のCPUコア上の指定の割り込みを有効   
    uart_set_irq_enables(UART_ID, true, true); // UARTのRX・TX割り込みを有効
}

