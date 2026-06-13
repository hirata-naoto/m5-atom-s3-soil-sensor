# M5 Atom S3 + DFROBOT SEN0604 RS485 Soil Sensor System

M5 Atom S3とDFROBOT SEN0604(RS485土壌センサー)を組み合わせた土壌水分・温度・pH測定システムです。

## 概要

- **マイコン**: M5 Atom S3
- **センサー**: DFROBOT SEN0604 (RS485 Modbus RTU)
- **計測項目**: 土壌水分量、温度、EC値、pH
- **通信方式**: RS485 / Modbus RTU
- **表示**: M5 Atom S3内蔵ディスプレイ + シリアルモニタ
- **開発環境**: Arduino IDE (M5Unified + ModbusMaster)

## プロジェクト構成

```
.
├── README.md
├── wiring.md                 # 配線図と接続方法
├── docs/
│   └── modbus-registers.md   # SEN0604 Modbusレジスタ仕様
└── src/
    └── soil_sensor.ino       # メインスケッチ (M5Unified版)
```

## クイックスタート

### 準備するもの

- M5 Atom S3
- DFROBOT SEN0604 RS485 土壌センサー
- RS485ケーブル（またはジャンパーワイヤー）
- USB-Cケーブル（M5 Atom S3用）

### インストール手順

1. **Arduino IDEを開く**

2. **必要なライブラリをインストール**
   - スケッチ → ライブラリを含める → ライブラリを管理
   - 以下を検索してインストール:
     - `M5Unified` (M5Stack官式)
     - `ModbusMaster` (Rob Tillaart版)

3. **ボード設定**
   - ツール → ボード → M5Stack → M5Atom-S3を選択

4. **配線を確認** 
   - [wiring.md](./wiring.md)を参照
   - GPIO 5 (TX) / GPIO 6 (RX) を確認

5. **`src/soil_sensor.ino`をArduino IDEで開く**

6. **シリアルモニタ設定**
   - ボーレート: 115200 bps
   - 改行コード: NL または CR+LF

7. **書き込み**
   - スケッチ → マイコンボードに書き込む
   - ボード: M5Stack Atom S3
   - ���ート: 使用しているCOMポート

8. **動作確認**
   - M5 Atom S3の画面にセンサーデータが表示されることを確認
   - シリアルモニタにも同じデータが出力されることを確認

## 使用方法

### 画面表示例

```
=== Soil Sensor ===

Moisture: 65.3 %
Temp: 28.5 C
EC: 1.2 mS/cm
PH: 6.8
[OK]

Last: 123
```

### シリアルモニタでの出力例

```
=== Sensor Reading ===
Moisture: 65.3 %
Temperature: 28.5 C
EC: 1.2 mS/cm
PH: 6.8

```

### 計測の流れ

1. **初期化時** (起動時)
   - M5 Atom S3のLEDが青く点灯
   - ディスプレイに「Initializing...」と表示
   - ModbusMasterライブラリが初期化される
   - 0.5秒後に通常状態へ

2. **計測実行** (2秒ごと、デフォルト)
   - 土壌水分値をレジスタ 0x0000 から読み取り
   - 温度値をレジスタ 0x0001 から読み取り
   - EC値をレジスタ 0x0002 から読み取り
   - pH値をレジスタ 0x0003 から読み取り
   - ディスプレイを更新
   - 成功時: LED が緑色に点灯
   - 失敗時: LED が赤色に点灯、エラーメッセージを表示

3. **ボタン押下**
   - フロントボタンを押すと即座に計測を実行
   - LED が黄色に点灯

### カスタマイズ

**計測間隔を変更:**
```cpp
#define INTERVAL_MS 2000  // ← この値を変更（ミリ秒単位）
```

**センサーアドレスを変更:**
```cpp
#define SENSOR_ADDRESS 1  // ← 複数センサーの場合はアドレスを変更
```

**ボーレートを変更:**
```cpp
#define SERIAL_BAUD 9600  // ← センサー設定に合わせる
```

**ピン設定を変更:**
```cpp
#define RS485_RX_PIN 6    // ← RXピンを変更
#define RS485_TX_PIN 5    // ← TXピンを変更
```

## トラブルシューティング

| 症状 | 原因 | 解決方法 |
|------|------|--------|
| 画面に何も表示されない | ディスプレイ接続不良 | M5 Atom S3の再起動、ライブラリの再インストール |
| データが読めない | RS485接続が不安定 | [wiring.md](./wiring.md)の配線を確認、終端抵抗を追加 |
| 画面に「ERROR!」表示 | Modbus通信エラー | エラーメッセージを確認、シリアルモニタで詳細確認 |
| "ERROR: Moisture read failed" | センサーが応答しない | 電源供給確認、ケーブル接触確認 |
| "ERROR: PH read failed" | pH値が読めない | センサー埋め込み深度確認、キャリブレーション確認 |
| 初期化エラー | M5 Atom S3��初期化失敗 | USBケーブルの接続確認、ドライバの再インストール |
| 異常な値が読める | センサー設定の問題 | センサーのアドレス確認、キャリブレーション確認 |

### Modbusエラーコード

| コード | 意味 | 対応 |
|--------|------|------|
| 0 | Success | 正常 |
| 1 | Illegal Function | 関数コードエラー（稀） |
| 2 | Illegal Data Address | レジスタアドレス不正 |
| 3 | Illegal Data Value | データ値不正 |
| 4 | Device Failure | センサー応答なし |
| 5 | Acknowledge | 確認待ち |
| 6 | Device Busy | センサービジー |

## 参考資料

- [M5 Atom S3 ドキュメント](https://docs.m5stack.com/en/core/AtomS3)
- [DFROBOT SEN0604 Wiki](https://wiki.dfrobot.com/sen0604/docs/20297)
- [Modbus RTU仕様](http://www.modbus.org/)
- [ModbusMaster ライブラリ](https://github.com/4-20ma/ModbusMaster)
- [M5Unified ドキュメント](https://github.com/m5stack/M5Unified)

## ライセンス

MIT License

## 作成者

hirata-naoto

---

**更新日**: 2026-06-13
**バージョン**: 2.1.0 (M5Unified + ModbusMaster版、ディスプレイ+PH対応)
