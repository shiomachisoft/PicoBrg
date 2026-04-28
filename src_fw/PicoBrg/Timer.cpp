// Copyright © 2024 Shiomachi Software. All rights reserved.
#include "Common.h"

// [define] / [マクロ定義]
#define TMR_CALLBACK_PERIOD   1    // Periodic timer callback period (ms) / 定期タイマコールバックの周期(ms)
#define TMR_WDT_TIMEOUT       5000 // WDT timer clear timeout (ms) *Set to a time that allows FLASH erase/write to complete / WDTタイマクリアのタイムアウト時間(ms) ※FLASHの消去・書き込みが間に合う時間にすること
#define TMR_RECV_TIMEOUT      500  // Timeout if the end of the request frame is not received even after TMR_RECV_TIMEOUT[ms] has elapsed since receiving the request frame header / 要求フレームのヘッダを受信後、TMR_RECV_TIMEOUT[ms]経過しても要求フレームの末尾まで受信していない場合はタイムアウトとする
#define TMR_LED_PERIOD        500  // LED blink period (ms) / LED点滅の周期(ms)
#define TMR_STABILIZATION_WAIT_TIME 50 // Stabilization wait time after startup (ms) *Appropriate value / 起動してからの安定待ち時間(ms) ※値は適当

// File scope variables / [ファイルスコープ変数]
static repeating_timer_t f_stTimer = {0};               // Parameter passed when registering periodic timer callback / 定期タイマコールバック登録時に渡すパラメータ
static volatile ULONG f_timerCnt_wdt = 0;               // Timer count for WDT timer / WDTタイマのタイマカウント
static volatile ULONG f_timerCnt_stabilizationWait = 0; // Timer count for stabilization wait time after startup / 起動してからの安定待ち時間のタイマカウント
static volatile ULONG f_timerCnt_recvTimeout = 0;       // Timer count for request frame receive timeout / 要求フレーム受信タイムアウトのタイマカウント
static volatile ULONG f_timerCnt_led = 0;               // Timer count for LED blink / LED点滅のタイマカウント

// Function prototypes / [関数プロトタイプ宣言]
static bool TMR_PeriodicCallback(repeating_timer_t *pstTimer);

// Periodic timer callback / 定期タイマコールバック
static bool TMR_PeriodicCallback(repeating_timer_t *pstTimer) 
{
    // [Timer count for WDT timer] / [WDTタイマのタイマカウント]
    if (f_timerCnt_wdt < TMR_WDT_TIMEOUT) { // If not timed out / タイムアウトしてない場合
        f_timerCnt_wdt++;
    }
    else { // If timed out / タイムアウトした場合
        // Reboot immediately on WDT timeout using watchdog_enable() / watchdog_enable()を使用して即WDTタイムアウトで再起動する
        CMN_WdtEnableReboot();
    }

    // [Timer count for stabilization wait time after startup] / [起動してからの安定待ち時間のタイマカウント]
    if (f_timerCnt_stabilizationWait < TMR_STABILIZATION_WAIT_TIME) {
        f_timerCnt_stabilizationWait++;
    }

    // Timer count for request frame receive timeout / 要求フレーム受信タイムアウトのタイマカウント
    if (f_timerCnt_recvTimeout < TMR_RECV_TIMEOUT) {
        f_timerCnt_recvTimeout++;
    }

    // [Timer count for LED blink] / [LED点滅のタイマカウント]
    if (f_timerCnt_led < TMR_LED_PERIOD) {
        f_timerCnt_led++; 
    }
 
    return true; // keep repeating
}

// Clear timer count for WDT timer / WDTタイマのタイマカウントをクリア
void TMR_WdtClear()
{
    f_timerCnt_wdt = 0;
}

// Get whether stabilization wait time after startup has passed / 起動してからの安定待ち時間が経過したかどうかを取得
bool TMR_IsStabilizationWaitTimePassed()
{
    return (f_timerCnt_stabilizationWait >= TMR_STABILIZATION_WAIT_TIME) ? true : false;
}

// Clear timer count for request frame receive timeout / 要求フレーム受信タイムアウトのタイマカウントをクリア
void TMR_ClearRecvTimeout()
{
    f_timerCnt_recvTimeout = 0;
}

// Get whether request frame receive timeout occurred / 要求フレーム受信タイムアウトが発生したか否かを取得
bool TMR_IsRecvTimeout()
{
    return (f_timerCnt_recvTimeout >= TMR_RECV_TIMEOUT) ? true : false;
}

// Get whether it is time to change LED ON/OFF / LEDのON/OFFを変更するタイミングか否かを取得
bool TMR_IsLedChangeTiming()
{
    return (f_timerCnt_led >= TMR_LED_PERIOD) ? true : false;
}

// Clear timer count for LED blink / LED点滅のタイマカウントをクリア
void TMR_ClearLedTimer()
{
    f_timerCnt_led = 0;
}

// Initialize timer / タイマーを初期化
void TMR_Init()
{
    // Register periodic timer callback / 定期タイマコールバックの登録
    add_repeating_timer_ms(TMR_CALLBACK_PERIOD, TMR_PeriodicCallback, NULL, &f_stTimer);
}