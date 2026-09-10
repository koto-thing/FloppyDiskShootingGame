# Switch / Switch 2 Proコントローラーの直接接続

USBとBluetooth LEの入力処理をゲームに組み込み、Windows標準APIを使用する
Steam Inputや外部の変換ソフト、追加DLLの同梱は不要
対象はNintendo純正Switch Pro（VID 057E / PID 2009）とSwitch 2 Pro（VID 057E / PID 2069）
製品IDごとに検出・初期化・レポート形式を切り替える

## USB

1. データ通信対応のUSBケーブルでPCへ接続する
2. ゲームを起動する（起動後の接続も検出する）
3. ボタンとスティックから手を離して中心補正の完了を待つ

初代Proは標準HID、Switch 2 ProはHIDおよびWinUSBインターフェイスを使用する
別のドライバーへ変更されている場合や他アプリが占有している場合は直接接続できないことがある

## Bluetooth

初代Switch ProはWindowsの「デバイスの追加」で通常のBluetoothペアリングを行う
接続後はゲームがHIDデバイスとして検出する
以下はSwitch 2 Pro専用の手順

1. WindowsのBluetoothを有効にしてゲームを起動する
2. USBケーブルを外し、コントローラーのSYNCボタンを長押ししてLEDを点滅させる
3. ボタンとスティックから手を離して中心補正の完了を待つ

Windowsの「デバイスの追加」からのペアリングは行わず、ゲームがNintendoの広告を検出して直接接続する
本体のペアリング情報は書き換えないため、ゲーム再起動後や再接続時はSYNC操作が必要になる場合がある
Bluetooth LE対応アダプターが必要で、応答速度はWindowsとアダプターに依存する

## 操作

| 操作 | ボタン |
| --- | --- |
| 移動 | 左スティック / 十字キー |
| 決定 / 会話送り | A |
| 射撃 | A / R / ZR |
| 低速移動 | L / ZL |
| 視点切替 / 会話スキップ | X |
| ボム | Y |
| メニュー / 取消 | ＋ / − / B |
| UIポインター | 右スティック、移動後にAでクリック・ドラッグ |

ボタンはNintendoの印字基準
C・HOME・キャプチャー・GL・GR・ジャイロ・振動は割り当てていない
中心補正は接続ごとに手を離した安定入力16報を使用する
スティックを倒したまま接続した場合は一度手を離す

XInputコントローラーが接続中ならそちらを優先する
直接接続の中ではUSBを優先する
他の変換アプリやSteamと競合する場合は、それらを終了して再接続する

## 検証

`powershell -ExecutionPolicy Bypass -File Tests/RunInputTests.ps1`

0x09のボタン割り当て、軸の端点・個体差の中心補正、不正長の拒否、既存入力への接続、切断時の解放を実行する
既存のキーボード・マウス・XInput回帰テストも同時に実行する
初代Proでは実機から取得した0x30レポートも再生し、予約ビットとUSB/Bluetoothの長さの違いを検証する

実機の入力を30秒間測定する場合：
`powershell -ExecutionPolicy Bypass -File Tests/RunInputTests.ps1 -Hardware`

実機で確認する項目：USB起動前/起動後の接続、BluetoothのSYNC接続、各ボタン・上下左右、UIクリック、抜線/電源OFF時の解除、再接続、Alt+Tabからの復帰
PID 2009のUSB接続では、初期化・中心補正・A/B/X/Yと左スティックの入力を実機で確認済み
修正後の30秒間測定では接続番号が一度も切り替わらず、連続受信を確認済み
Switch 2 Proおよび無線通信は実機未検証

## 通信形式の参照

- [Switch 2 HIDレポート仕様](https://github.com/ndeadly/switch2_controller_research/blob/master/hid_reports.md)
- [Bluetooth広告・GATT・ペアリング仕様](https://github.com/ndeadly/switch2_controller_research/blob/master/bluetooth_interface.md)
- [USB初期化コマンド](https://github.com/ndeadly/switch2_controller_research/blob/master/commands.md)
- [WindowsでのWinUSB初期化例](https://github.com/Altureus/Windows-Switch2-Pro-Controller/blob/main/procon2/winusb.py)
- [初代Switch ProのHID処理](https://github.com/libsdl-org/SDL/blob/main/src/joystick/hidapi/SDL_hidapi_switch.c)

上記の通信形式に基づく独立実装で、外部ライブラリのソースコードは同梱していない
