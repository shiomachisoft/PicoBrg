# PicoBrg

[English](#english) | [日本語](#japanese)

<a id="english"></a>
## Overview
PicoBrg is a set of firmware and configuration tools that uses the Raspberry Pi Pico W to convert (bridge) communication in the following two modes:

- **BLE Mode**: BLE ⇔ UART conversion
- **Wi-Fi Mode**: Wi-Fi (TCP socket communication) ⇔ UART conversion

The communication mode, Wi-Fi, and UART settings for the Pico W are configured from a dedicated Windows PC application, and the settings are saved in the Flash memory of the Pico W. Once configured, the settings are retained even after the power is turned off.

## Usage
For detailed setup instructions and usage, please refer to the manual below.
- [PicoBrg Manual (English)](docs/en/01_Manual/README.md)

## Source Code
Source code for both the firmware (FW) and the PC application is publicly available.
- **Firmware**: Developed using the Arduino IDE.
- **PC Application**: Developed in C# for Windows environments (requires `.NET Framework 4.6.2` or later).

---

<a id="japanese"></a>
## 概要
PicoBrgは、Raspberry Pi Pico Wを使用して、以下の2つのモードで通信の変換（ブリッジ）を行うファームウェアおよび設定ツールのセットです。

- **BLEモード**: BLE ⇔ UART の変換
- **Wi-Fiモード**: Wi-Fi（TCPソケット通信） ⇔ UART の変換

Pico Wに対する通信モード、Wi-Fi、UARTの設定は専用のWindows向けPCアプリから行い、設定内容はPico WのFlashメモリに保存されます。一度設定すれば、電源を切っても設定は保持されます。

## 使い方
詳細なセットアップ手順や使用方法については、以下のマニュアルをご参照ください。
- [PicoBrg マニュアル (日本語)](docs/ja/01_マニュアル/README.md)

## ソースコード
ファームウェア（FW）とPCアプリの両方ともソースコードを公開しています。
- **ファームウェア**: Arduino IDEで作成しています。
- **PCアプリ**: C#で作成したWindows環境用アプリです（動作には `.NET Framework 4.6.2` 以上が有効になっている必要があります）。
