# PicoBrg

[English](#english) | [日本語](#japanese)

<a id="english"></a>
## Overview
PicoBrg is a package consisting of firmware and a configuration PC app that uses the Raspberry Pi Pico W to bridge communications in the following two modes:

- **BLE Mode**: BLE ⇔ UART conversion
- **Wi-Fi Mode**: Wi-Fi (TCP socket communication) ⇔ UART conversion

## System Configuration
PicoBrg consists of two phases: "Configuration" via a PC application and the actual "Bridge Communication".

### During configuration

Use the dedicated PC application to configure various settings for the Pico W (communication mode, Wi-Fi, and UART).

<img width="509" height="115" alt="image" src="https://github.com/user-attachments/assets/6053c85c-94a2-4efb-89e9-94664699e260" />

- **Notes**
  - Connect the PC and Pico W using a USB cable to perform the configuration.
  - Since various settings (communication mode, Wi-Fi, UART) are saved in the flash memory inside the Pico W, they are retained even if the power is turned off once configured.

### During bridge communication

After configuration is complete, the Pico W bridges data according to the configured communication mode.

<img width="1136" height="202" alt="image" src="https://github.com/user-attachments/assets/e0523d91-ef66-44c0-9047-86c1db45315e" />

- **Notes on Communication Modes**
  - The default is BLE mode.
  - In BLE mode, the Pico W operates as a BLE "peripheral".
  - In Wi-Fi mode, the Pico W can operate as either a "TCP server" or a "TCP client".

## Usage
For setup instructions and usage, please refer to the manual below.
- [PicoBrg Manual (English)](docs/en/01_Manual/README.md)

## Source Code
Source code for both the firmware (FW) and the PC application is publicly available.
- **Firmware**: Developed using the Arduino IDE.
- **PC Application**: Developed in C# for Windows environments.

---

<a id="japanese"></a>
## 概要
PicoBrgは、Raspberry Pi Pico Wを使用して、以下の2つのモードで通信の変換（ブリッジ）を行うファームウェアおよび設定用PCアプリのセットです。

- **BLEモード**: BLE ⇔ UART の変換
- **Wi-Fiモード**: Wi-Fi（TCPソケット通信） ⇔ UART の変換

## システム構成
PicoBrgは、PCアプリによる「設定」と、実際の「ブリッジ通信」の2つのフェーズで構成されています。

### 設定時の構成

専用のPCアプリを使用して、Pico Wの各種設定（通信モード・Wi-Fi・UART）を行います。

<img width="509" height="115" alt="image" src="https://github.com/user-attachments/assets/6053c85c-94a2-4efb-89e9-94664699e260" />

- **補足**
  - PCとPico WをUSBケーブルで接続して設定を行います。
  - 各種設定（通信モード、Wi-Fi、UART）はPico W内のFlashメモリに保存されるため、一度設定すれば電源を切っても保持されます。

### ブリッジ通信時の構成

設定完了後、Pico Wは設定された通信モードに従ってデータの橋渡し（ブリッジ）を行います。

<img width="1136" height="202" alt="image" src="https://github.com/user-attachments/assets/e0523d91-ef66-44c0-9047-86c1db45315e" />

- **通信モードの補足** 
  - デフォルトはBLEモードです。
  - BLEモードの場合、Pico WはBLEの「ペリフェラル」として動作します。
  - Wi-Fiモードの場合、Pico Wは「TCPサーバー」と「TCPクライアント」のどちらかとして動作可能です。

## 使い方
セットアップ手順や使用方法については、以下のマニュアルをご参照ください。
- [PicoBrg マニュアル (日本語)](docs/ja/01_マニュアル/README.md)

## ソースコード
ファームウェア（FW）とPCアプリの両方ともソースコードを公開しています。
- **ファームウェア**: Arduino IDEで作成しています。
- **PCアプリ**: C#で作成したWindows環境用アプリです。
