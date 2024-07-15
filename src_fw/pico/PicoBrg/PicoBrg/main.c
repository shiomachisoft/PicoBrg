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
		FRM_RecvMain();

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

#ifdef MY_BOARD_PICO_W
		// TCPのメイン処理
		tcp_cmn_main();
#endif

		// USB/無線の送信メイン処理
		FRM_SendMain();
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
#ifdef MY_BOARD_PICO_W		
		cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, bLedOn);	
#else		
		gpio_put(ONBOARD_LED, bLedOn);
#endif		
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
	//ST_GPIO_CONFIG stGpioConfig; // GPIO設定

	// 例外ハンドラを登録
	MAIN_RegisterExceptionHandler();

	// 標準入出力を初期化
    stdio_init_all();

	// GPIOを初期化
	//GPIO_SetDefault(&stGpioConfig);
	//GPIO_Init(&stGpioConfig);

	// 共通ライブラリを初期化
	CMN_Init();	

	// FLASHライブラリを初期化
	FLASH_Init();
	pstFlashData = FLASH_GetDataAtPowerOn();

	// UARTを初期化
	UART_Init(&pstFlashData->stUartConfig);

	// USB/無線共通処理を初期化
	FRM_Init();

	// タイマーを初期化
	TIMER_Init();

	// 起動してからの安定待ち時間を待つ
	busy_wait_ms(TIMER_STABILIZATION_WAIT_TIME);

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