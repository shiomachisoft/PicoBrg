# FWビルド方法

- 下記のバージョンのArduino IDEをWindowsにインストールします。

  - arduino-ide_2.3.8_Windows_64bit.exe

- src_fwフォルダ内のPicoBrgフォルダをPCの適当な場所にコピーします。

- PicoBrgフォルダ内のPicoBrg.inoをArduino IDEで開きます。

- 「ファイル」⇒「基本設定」で、「追加のボードマネージャのURL」に下記を入力します。

`https://github.com/earlephilhower/arduino-pico/releases/download/global/package_rp2040_index.json`

![board_manager](images/Pictures/board_manager.png)

- 下記のボードマネージャをインストールします。

  - ※念のため、バージョンも合わせます。

![version](images/Pictures/version.png)

- ボードとポートの選択

  - ボード：「ツール」⇒「ボード」⇒「Raspberry Pi Pico/RP2040/RP2350」⇒「Raspberry Pi Pico W」を選択します。
  - ポート：Pico WをPCにUSB接続後、「ツール」⇒「ポート」⇒Pico WのCOM番号を選択します。

![board_port](images/Pictures/board_port.png)

- 「IP/Bluetooth Stack」の選択

  - 「ツール」⇒「IP/Bluetooth Stack」⇒「IPv4 + Bluetooth」を選択します。

![ip_bluetooth](images/Pictures/ip_bluetooth.png)

- 「スケッチ」⇒「検証・コンパイル」を実行します。
