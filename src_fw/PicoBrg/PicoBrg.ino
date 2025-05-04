// Copyright © 2024 Shiomachi Software. All rights reserved.
#include "Common.h"

// [ファイルスコープ変数]
static bool f_isCpu0SetupCompleted = false; // CPUコア0のセットアップが完了済みか否か
static bool f_isWillClearWdtByCore1 = false; // CPUコア1によってWDTタイマをクリアする番か否か

// [関数プロトタイプ宣言]
static void MAIN_ControlLed();

// CPUコア0のセットアップ
void setup() 
{
	ST_FLASH_DATA *pstFlashData; // 電源起動時のFLASHデータ

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

	f_isCpu0SetupCompleted = true;
}

// CPUコア0のメインループ
void loop()
{
	if (f_isCpu0SetupCompleted) { // CPUコア0のセットアップが完了済み
		if (!f_isWillClearWdtByCore1) { // CPUコア0によってWDTタイマをクリアする番の場合
			// WDTタイマをクリア
			TIMER_WdtClear();
			f_isWillClearWdtByCore1 = true;
		}	

		// USB受信データ取り出し⇒コマンド解析・実行
		FRM_Main();

		// UARTメイン処理
		UART_Main();
	}
}

// CPUコア1のセットアップ
void setup1() 
{
}

// CPUコア1のメインループ
void loop1()
{	
	if (f_isWillClearWdtByCore1) { // CPUコア1によってWDTタイマをクリアする番の場合	
		// WDTタイマをクリア
		TIMER_WdtClear();
		f_isWillClearWdtByCore1 = false;
	}	

	// LEDを制御する
	MAIN_ControlLed();

	// 無線のメイン処理
	WL_Main();

	// 無線送信データをデキュー⇒無線送信
	WL_SendMain();
}

// LEDを制御する
static void MAIN_ControlLed()
{
	static bool bLedOn = false;

	// LEDのON/OFFを変更するタイミングか否かを取得
	if (true == TIMER_IsLedChangeTiming()) {
		// LEDのON/OFFを決定
		if (true == tcp_is_ap_connected() || (true == BLE_IsConnected())) { 
			// WiFiモードでAPと接続済みか否か ,または, BLEモードでセントラルと接続済みか否か
			bLedOn = true;
		} 
		else {
			bLedOn = !bLedOn;	
		}
		
		// LEDにON/OFFを出力			
		digitalWrite(LED_BUILTIN, bLedOn);	
		// LED点滅のタイマカウントをクリア
		TIMER_ClearLedTimer();
	}
}



