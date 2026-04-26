// Copyright © 2024 Shiomachi Software. All rights reserved.
#include "Common.h"

// [define] 
#define TMR_CALLBACK_PERIOD   1    // 定期タイマコールバックの周期(ms)
#define TMR_WDT_TIMEOUT       5000 // WDTタイマクリアのタイムアウト時間(ms) ※FLASHの消去・書き込みが間に合う時間にすること
#define TMR_RECV_TIMEOUT      500  // 要求フレームのヘッダを受信後、TMR_RECV_TIMEOUT[ms]経過しても要求フレームの末尾まで受信してない場合はタイムアウトとする
#define TMR_LED_PERIOD        500  // LED点滅の周期(ms)
#define TMR_STABILIZATION_WAIT_TIME 50 // 起動してからの安定待ち時間(ms) ※値は適当

// [ファイルスコープ変数の宣言]
static repeating_timer_t f_stTimer = {0};               // 定期タイマコールバック登録時に渡すパラメータ
static volatile ULONG f_timerCnt_wdt = 0;               // WDTタイマのタイマカウント
static volatile ULONG f_timerCnt_stabilizationWait = 0; // 起動してからの安定待ち時間のタイマカウント
static volatile ULONG f_timerCnt_recvTimeout = 0;       // 要求フレーム受信タイムアウトのタイマカウント
static volatile ULONG f_timerCnt_led = 0;               // LED点滅のタイマカウント

// [関数プロトタイプ宣言]
static bool TMR_PeriodicCallback(repeating_timer_t *pstTimer);

// 定期タイマコールバック
static bool TMR_PeriodicCallback(repeating_timer_t *pstTimer) 
{
    // [WDTタイマのタイマカウント]
    if (f_timerCnt_wdt <  TMR_WDT_TIMEOUT) { // タイムアウトしてない場合
        f_timerCnt_wdt++;
    }
    else { // タイムアウトした場合
        // watchdog_enable()を使用して即WDTタイムアウトで再起動する
        CMN_WdtEnableReboot();
    }

    // [起動してからの安定待ち時間のタイマカウント]
    if (f_timerCnt_stabilizationWait < TMR_STABILIZATION_WAIT_TIME) {
        f_timerCnt_stabilizationWait++;
    }

    // 要求フレーム受信タイムアウトのタイマカウント
    if (f_timerCnt_recvTimeout < TMR_RECV_TIMEOUT) {
        f_timerCnt_recvTimeout++;
    }

    // [LED点滅のタイマカウント]
    if (f_timerCnt_led < TMR_LED_PERIOD) {
        f_timerCnt_led++; 
    }
 
    return true; // keep repeating
}

// WDTタイマのタイマカウントをクリア
void TMR_WdtClear()
{
    f_timerCnt_wdt = 0;
}

// 起動してからの安定待ち時間が経過したかどうかを取得
bool TMR_IsStabilizationWaitTimePassed()
{
    return (f_timerCnt_stabilizationWait >= TMR_STABILIZATION_WAIT_TIME) ? true : false;
}

// 要求フレーム受信タイムアウトのタイマカウントをクリア
void TMR_ClearRecvTimeout()
{
    f_timerCnt_recvTimeout = 0;
}

// 要求フレーム受信タイムアウトが発生したか否かを取得
bool TMR_IsRecvTimeout()
{
    return (f_timerCnt_recvTimeout >= TMR_RECV_TIMEOUT) ? true : false;
}

// LEDのON/OFFを変更するタイミングか否かを取得
bool TMR_IsLedChangeTiming()
{
    return (f_timerCnt_led >= TMR_LED_PERIOD) ? true : false;
}

// LED点滅のタイマカウントをクリア
void TMR_ClearLedTimer()
{
    f_timerCnt_led = 0;
}

// タイマーを初期化
void TMR_Init()
{
    // 定期タイマコールバックの登録
    add_repeating_timer_ms(TMR_CALLBACK_PERIOD, TMR_PeriodicCallback, NULL, &f_stTimer);
}