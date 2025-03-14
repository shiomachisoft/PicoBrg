// Copyright © 2024 Shiomachi Software. All rights reserved.
#include "Common.h"

// [ファイルスコープ変数]
static bool f_isWillClearWdtByCore1 = false; // CPUコア1によってWDTタイマをクリアする番か否か

// [関数プロトタイプ宣言]
static void MAIN_Init();
static void MAIN_MainLoop_Core0();
static void MAIN_MainLoop_Core1();
static void MAIN_ControlLed();
static void MAIN_ExceptionHandler();
static void MAIN_RegisterExceptionHandler();

// メイン関数
int main() 
{
	// 電源起動時の初期化
	MAIN_Init();
	
	// CPUコア1のメインループを開始
	multicore_launch_core1(MAIN_MainLoop_Core1); 

	// CPUコア0のメインループを開始
	MAIN_MainLoop_Core0();

	return 0;
}

// CPUコア0のメインループ
static void MAIN_MainLoop_Core0()
{
    while (1) 
	{
		if (!f_isWillClearWdtByCore1) { // CPUコア0によってWDTタイマをクリアする番の場合
			// WDTタイマをクリア
			TIMER_WdtClear();
			f_isWillClearWdtByCore1 = true;
		}	

		// USB受信データ取り出し⇒コマンド解析・実行
		FRM_Main();

		// 無線受信データをデキュー⇒UART送信データをエンキュー
		WL_RecvMain();

 		// UARTメイン処理
		UART_Main();
    }
}

// CPUコア1のメインループ
static void MAIN_MainLoop_Core1() 
{
	while (1) 
	{
		if (f_isWillClearWdtByCore1) { // CPUコア1によってWDTタイマをクリアする番の場合	
			// WDTタイマをクリア
			TIMER_WdtClear();
			f_isWillClearWdtByCore1 = false;
		}	

		// LEDを制御する
		MAIN_ControlLed();

		// TCPのメイン処理
		tcp_cmn_main();

		// 無線送信データをデキュー⇒無線送信
		WL_SendMain();
	}
}

// LEDを制御する
static void MAIN_ControlLed()
{
	static bool bLedOn = false;

	// LEDのON/OFFを変更するタイミングか否かを取得
	if (true == TIMER_IsLedChangeTiming()) {
		// LEDのON/OFFを決定
		if (true == tcp_cmn_is_link_up()) {
			bLedOn = true;
		} 
		else {
			bLedOn = !bLedOn;	
		}
		
		// LEDにON/OFFを出力	
		cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, bLedOn);		
		// LED点滅のタイマカウントをクリア
		TIMER_ClearLedTimer();
	}
}

// 例外ハンドラ
static void MAIN_ExceptionHandler()
{
	// watchdog_enable()を使用して即WDTタイムアウトで再起動する
	CMN_WdtEnableReboot();
}

// 電源起動時の初期化
static void MAIN_Init()
{
	ST_FLASH_DATA *pstFlashData; // 電源起動時のFLASHデータ

	// 例外ハンドラを登録
	MAIN_RegisterExceptionHandler();

	// CDCを初期化
    stdio_init_all();

	// 共通ライブラリを初期化
	CMN_Init();	

	// FLASHライブラリを初期化
	FLASH_Init();
	pstFlashData = FLASH_GetDataAtPowerOn();

	// UARTを初期化
	UART_Init(&pstFlashData->stUartConfig);

	// USB通信を初期化
	FRM_Init();

	// タイマーを初期化
	TIMER_Init();

	// 起動してからの安定待ち時間を待つ
	while (!TIMER_IsStabilizationWaitTimePassed()) {}

	if (watchdog_enable_caused_reboot()) { // watchdog_reboot()ではなくwatchdog_enable()のWDTタイムアウトで再起動していた場合
		// FWエラーを設定
		CMN_SetErrorBits(CMN_ERR_BIT_WDT_RESET, true);
	}	
}

// 例外ハンドラを登録
static void MAIN_RegisterExceptionHandler()
{
	exception_set_exclusive_handler(NMI_EXCEPTION, MAIN_ExceptionHandler);
	exception_set_exclusive_handler(HARDFAULT_EXCEPTION, MAIN_ExceptionHandler);
	exception_set_exclusive_handler(SVCALL_EXCEPTION, MAIN_ExceptionHandler);
	exception_set_exclusive_handler(PENDSV_EXCEPTION, MAIN_ExceptionHandler);
	exception_set_exclusive_handler(SYSTICK_EXCEPTION, MAIN_ExceptionHandler);	
}