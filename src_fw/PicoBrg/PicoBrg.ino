// Copyright © 2024 Shiomachi Software. All rights reserved.
#include "Common.h"

// File scope variables / [ファイルスコープ変数]
static volatile bool f_isCpu0SetupCompleted = false; // Whether CPU core 0 setup is completed / CPUコア0のセットアップが完了済みか否か
static volatile bool f_isCore1TurnToClearWdt = false; // Whether it is CPU core 1's turn to clear WDT timer / CPUコア1によってWDTタイマをクリアする番か否か

// Function prototypes / [関数プロトタイプ宣言]
static void MN_ControlLed();
static void MN_RegisterExceptionHandler();
static void MN_ExceptionHandler();

// CPU core 0 setup / CPUコア0のセットアップ
void setup() 
{
	ST_FLASH_DATA *pstFlashData; // FLASH data at power on / 電源起動時のFLASHデータ

	// Register exception handler / 例外ハンドラを登録
	MN_RegisterExceptionHandler();

	// Initialize common library / 共通ライブラリを初期化
	CMN_Init();	

	// Set LED pin output / LEDピンの出力設定
	pinMode(LED_BUILTIN, OUTPUT);

	// Initialize FLASH library / FLASHライブラリを初期化
	FLASH_Init();
	pstFlashData = FLASH_GetDataAtPowerOn();

	// Initialize UART / UARTを初期化
	UART_Init(&pstFlashData->stUartConfig);

	// Initialize USB communication / USB通信を初期化
	FRM_Init();

	// Initialize timer / タイマーを初期化
	TMR_Init();

	// Wait until stabilization time after startup has passed / 起動後の安定待ち時間が経過するのを待つ
	while (!TMR_IsStabilizationWaitTimePassed()) {
		tight_loop_contents();
	}

	if (watchdog_enable_caused_reboot()) { // If rebooted by WDT timeout of watchdog_enable() instead of watchdog_reboot() / watchdog_reboot()ではなくwatchdog_enable()のWDTタイムアウトで再起動していた場合
		// Set FW error / FWエラーを設定
		CMN_SetErrorBits(CMN_ERR_BIT_WDT_RESET, true);
	}

	f_isCpu0SetupCompleted = true;
}

// CPU core 0 main loop / CPUコア0のメインループ
void loop()
{
	if (!f_isCore1TurnToClearWdt) { // If it is CPU core 0's turn to clear WDT timer / CPUコア0によってWDTタイマをクリアする番の場合
		// Clear WDT timer / WDTタイマをクリア
		TMR_WdtClear();
		f_isCore1TurnToClearWdt = true;
	}	

	// Extract USB received data ⇒ Parse and execute command / USB受信データ取り出し⇒コマンド解析・実行
	FRM_Main();

	// UART main process / UARTメイン処理
	UART_Main();
}

// CPU core 1 setup / CPUコア1のセットアップ
void setup1() 
{
	// Wait for CPU core 0 setup completion / CPUコア0のセットアップ完了を待つ
	while (!f_isCpu0SetupCompleted) {
		tight_loop_contents();
	}

	// Initialize wireless / 無線を初期化
	WL_Init();
}

// CPU core 1 main loop / CPUコア1のメインループ
void loop1()
{	
	if (f_isCore1TurnToClearWdt) { // If it is CPU core 1's turn to clear WDT timer / CPUコア1によってWDTタイマをクリアする番の場合	
		// Clear WDT timer / WDTタイマをクリア
		TMR_WdtClear();
		f_isCore1TurnToClearWdt = false;
	}	

	// Control LED / LEDを制御する
	MN_ControlLed();

	// Wireless main process / 無線のメイン処理
	WL_Main();

	// Dequeue wireless send data ⇒ Wireless send / 無線送信データをデキュー⇒無線送信
	WL_SendMain();
}

// Control LED / LEDを制御する
static void MN_ControlLed()
{
	static volatile bool bLedOn = false;

	// Get whether it is time to change LED ON/OFF / LEDのON/OFFを変更するタイミングか否かを取得
	if (true == TMR_IsLedChangeTiming()) {
		// Determine LED ON/OFF / LEDのON/OFFを決定
		if (true == WL_IsConnected()) { // If wirelessly connected / 無線で接続済みの場合
			bLedOn = true;
		} 
		else {
			bLedOn = !bLedOn;	
		}
		
		// Output ON/OFF to LED / LEDにON/OFFを出力			
		digitalWrite(LED_BUILTIN, bLedOn);	
		// Clear LED blink timer count / LED点滅のタイマカウントをクリア
		TMR_ClearLedTimer();
	}
}

// Register exception handler / 例外ハンドラを登録
static void MN_RegisterExceptionHandler()
{
	exception_set_exclusive_handler(NMI_EXCEPTION, MN_ExceptionHandler);
	exception_set_exclusive_handler(HARDFAULT_EXCEPTION, MN_ExceptionHandler);
	//exception_set_exclusive_handler(SVCALL_EXCEPTION, MN_ExceptionHandler);
	//exception_set_exclusive_handler(PENDSV_EXCEPTION, MN_ExceptionHandler);
	//exception_set_exclusive_handler(SYSTICK_EXCEPTION, MN_ExceptionHandler);	
}

// Exception handler / 例外ハンドラ
static void MN_ExceptionHandler()
{
	// Reboot immediately by WDT timeout using watchdog_enable() / watchdog_enable()を使用して即WDTタイムアウトで再起動する
	CMN_WdtEnableReboot();
}
