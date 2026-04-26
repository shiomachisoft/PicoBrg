// Copyright © 2024 Shiomachi Software. All rights reserved.
#include "Common.h"

// [define] / [マクロ定義]
// For Pico: / Picoの場合:
// PICO_FLASH_SIZE_BYTES = 0x200000
// FLASH_SECTOR_SIZE = 0x1000
// FLASH_PAGE_SIZE = 256
#define FLASH_OFFSET  (PICO_FLASH_SIZE_BYTES - FLASH_SECTOR_SIZE)   // Start offset address of last sector of FLASH / FLASHの最後のセクタの先頭オフセットアドレス
#define FLASH_WRITE_BUF_SIZE (FLASH_PAGE_SIZE * 2)                  // FLASH data write size *Must be multiple of FLASH_PAGE_SIZE / FLASHデータ書き込みサイズ ※ FLASH_PAGE_SIZEの倍数とする

// File scope variables / [ファイルスコープ変数]
static ST_FLASH_DATA f_stFlashData = {0};            // FLASH data at power on / 電源起動時のFLASHデータ
static UCHAR f_writeBuf[FLASH_WRITE_BUF_SIZE] = {0}; // FLASH data write buffer / FLASHデータ書き込みバッファ

// Return FLASH data at power on / 電源起動時のFLASHデータを返す
ST_FLASH_DATA* FLASH_GetDataAtPowerOn()
{
    return &f_stFlashData;
}

// Read configuration data from sector 15 of last block (Block31) of FLASH / FLASHの最終ブロック(Block31)のセクタ15から設定データを読み込む
void FLASH_Read(ST_FLASH_DATA *pstFlashData)
{
    const PVOID pSrc = (const PVOID) (XIP_BASE + FLASH_OFFSET); // Start address of sector 15 of last block (Block31). XIP_BASE is program start. / 最終ブロック(Block31)のセクタ15の先頭アドレス。XIP_BASEはプログラムの先頭。
    char szFwName[FW_NAME_BUF_SIZE];
    USHORT checksum;       // Checksum / チェックサム
    bool bDefault = false; // Whether to use default config data / デフォルトの設定データを採用するか否か
    
    // Read start address of sector 15 of last block / 最終ブロック(Block31)のセクタ15の先頭アドレスを読み込む
    memcpy(pstFlashData, pSrc, sizeof(ST_FLASH_DATA));

    // [If FW name, FW version, and checksum are all valid, use read data as is] / [FW名、FWバージョン、チェックサムが全て正常な場合、読み込んだデータがそのまま使用される]

    // Check FW name / FW名のチェック
    memset(szFwName, 0, sizeof(szFwName));
    strcpy(szFwName, FW_NAME);
    // Do not use strcmp since pstFlashData->szFwName may not be null-terminated / pstFlashData->szFwNameがNULL文字で終わっているとは限らないのでstrcmpは使用しないで比較する
    if (memcmp(pstFlashData->szFwName, szFwName, FW_NAME_BUF_SIZE) != 0) {
        bDefault = true;  // Use default config data / デフォルトの設定データを採用
    }

    // Check FW version / FWバージョンのチェック
    if (!bDefault) {
        if (pstFlashData->fwVer != FW_VER) {
            // If FW version is invalid / FWバージョンが不正値の場合
            bDefault = true;  // Use default config data / デフォルトの設定データを採用
        }
    }

    // Checksum verification / チェックサム検査
    if (!bDefault) {
        // Calculate checksum / チェックサムを計算する
        checksum = CMN_CalcChecksum(pstFlashData, sizeof(ST_FLASH_DATA) - sizeof(pstFlashData->checksum));        
        if (pstFlashData->checksum != checksum) {
            // If checksum verification fails / チェックサム検査がNGの場合
            bDefault = true; // Use default config data / デフォルトの設定データを採用
        }
    }

    if (bDefault) {
        // Use default config data / デフォルトの設定データを採用
        
        // Initialize entire structure and set FW name and version / 構造体全体を初期化し、FW名やバージョンを設定
        memset(pstFlashData, 0, sizeof(ST_FLASH_DATA));
        strcpy(pstFlashData->szFwName, FW_NAME);
        pstFlashData->fwVer = FW_VER;
        UART_SetDefault(&pstFlashData->stUartConfig);
        WL_SetDefault(&pstFlashData->stNwConfig);
    }
}

// Write configuration data to sector 15 of last block (Block31) of FLASH / FLASHの最終ブロック(Block31)のセクタ15に設定データを書き込む
void FLASH_Write(ST_FLASH_DATA *pstFlashData)
{
    USHORT checksum;   // Checksum / チェックサム
    //ULONG ints;
    
    // Initialize FLASH data write buffer with 0xFF (erased state) / FLASHデータ書き込みバッファを0xFF(消去状態)で初期化
    memset(f_writeBuf, 0xFF, sizeof(f_writeBuf));
    // Set FW name / FW名を設定
    memset(pstFlashData->szFwName, 0, sizeof(pstFlashData->szFwName));
    strcpy(pstFlashData->szFwName, FW_NAME);
    // Set FW version / FWバージョンを設定
    pstFlashData->fwVer = FW_VER;
    // Calculate and set checksum / チェックサムを計算して設定
    checksum = CMN_CalcChecksum(pstFlashData, sizeof(ST_FLASH_DATA) - sizeof(pstFlashData->checksum));
    pstFlashData->checksum = checksum;
    // Copy argument data to FLASH data write buffer / FLASHデータ書き込みバッファに引数データをコピー
    memcpy(f_writeBuf, pstFlashData, sizeof(ST_FLASH_DATA));
   
    // Block CPU core 1 / CPUコア1をブロック
    rp2040.idleOtherCore();
    // Disable interrupts / 割り込み禁止
    //ints = save_and_disable_interrupts();
    (void)save_and_disable_interrupts();

    // FLASH erase / FLASH消去
    // Erase unit must be multiple of FLASH_SECTOR_SIZE (4096byte) defined in flash.h / 消去単位はflash.hで定義されている FLASH_SECTOR_SIZE(4096byte)の倍数とする
    flash_range_erase(FLASH_OFFSET, FLASH_SECTOR_SIZE);
    // FLASH write / FLASH書き込み
    // Write unit must be multiple of FLASH_PAGE_SIZE (256byte) defined in flash.h / 書込単位はflash.hで定義されている FLASH_PAGE_SIZE(256byte)の倍数とする
    flash_range_program(FLASH_OFFSET, f_writeBuf, sizeof(f_writeBuf)); // f_writeBuf size is multiple of FLASH_PAGE_SIZE / f_writeBufのサイズはFLASH_PAGE_SIZEの倍数になっている
    
    // Reboot immediately on WDT timeout without using watchdog_enable() / watchdog_enable()を使用しないで即WDTタイムアウトで再起動する
    CMN_WdtRebootWithoutEnable();

#if 0   
    // Enable interrupts / 割り込み許可
    restore_interrupts(ints); 
    // Unblock CPU core 1 / CPUコア1のブロックを解除
    rp2040.resumeOtherCore();
#endif
}

// Erase data in sector 15 of last block (Block31) of FLASH / FLASHの最終ブロック(Block31)のセクタ15のデータを消去
void FLASH_Erase()
{
    //ULONG ints;

    // Block CPU core 1 / CPUコア1をブロック
    rp2040.idleOtherCore();
    // Disable interrupts / 割り込み禁止
    //ints = save_and_disable_interrupts();
    (void)save_and_disable_interrupts();

    // FLASH erase / FLASH消去
    // Erase unit must be multiple of FLASH_SECTOR_SIZE (4096byte) defined in flash.h / 消去単位はflash.hで定義されている FLASH_SECTOR_SIZE(4096byte)の倍数とする
    flash_range_erase(FLASH_OFFSET, FLASH_SECTOR_SIZE);
    
    // Reboot immediately on WDT timeout without using watchdog_enable() / watchdog_enable()を使用しないで即WDTタイムアウトで再起動する
    CMN_WdtRebootWithoutEnable();

#if 0
    // Enable interrupts / 割り込み許可
    restore_interrupts(ints);    
    // Unblock CPU core 1 / CPUコア1のブロックを解除
    rp2040.resumeOtherCore();
#endif
}

// Initialize FLASH library / FLASHライブラリを初期化
void FLASH_Init()
{
    // Read FLASH data at power on / 電源起動時のFLASHデータを読み込み
    FLASH_Read(&f_stFlashData);
}