# DFROBOT SEN0604 Modbus RTU レジスタ仕様

## センサー基本情報

- **モデル**: DFROBOT SEN0604 RS485 4-in-1 Soil Sensor
- **通信方式**: Modbus RTU
- **ボーレート**: 2400, 4800, 9600 (デフォルト), 19200, 38400 bps
- **パリティ**: None
- **データビット**: 8
- **ストップビット**: 1
- **CRC**: 16-bit
- **デフォルトスレーブアドレス**: 1

## Modbusレジスタマップ

### センサーデータレジスタ（読み取り専用 - Function Code 0x03）

| レジスタアドレス | Decimal | 説明 | 単位 | 備考 |
|-----------------|---------|------|------|------|
| 0x0000 | 40001 | 土壌水分量 | % | 値 ÷ 10 (例: 658 → 65.8%) |
| 0x0001 | 40002 | 温度 | °C | 値 ÷ 10、符号付き (例: 251 → 25.1°C、-101 → -10.1°C) |
| 0x0002 | 40003 | 電気伝導度 (EC) | µS/cm | 直接値 (例: 1000 → 1000 µS/cm) |
| 0x0003 | 40004 | pH値 | - | 値 ÷ 10 (例: 56 → 5.6) |
| 0x0007 | 40008 | 塩分 (Salinity) | - | 参照値 |
| 0x0008 | 40009 | 総溶解��形物 (TDS) | mg/L | 参照値 |

### 設定レジスタ（読み書き可 - Function Code 0x03, 0x06）

| レジスタアドレス | Decimal | 説明 | 範囲 | デフォルト | 備考 |
|-----------------|---------|------|------|----------|------|
| 0x0022 | 40035 | EC温度係数 | 0-100 | 0 | 0.0%-10.0% |
| 0x0023 | 40036 | 塩分係数 | 0-100 | 55 | 0.00-1.00 (55=0.55) |
| 0x0024 | 40037 | TDS係数 | 0-100 | 50 | 0.00-1.00 (50=0.5) |
| 0x0050 | 40081 | 温度キャリブレーション | 符号付き | 0 | 値 ÷ 10 |
| 0x07d0 | 42001 | デバイススレーブアドレス | 1-254 | 1 | Modbus通信用アドレス |
| 0x07d1 | 42002 | ボーレート設定 | 0-2 | 2 (9600 bps) | 下表参照 |

### ボーレート設定値（レジスタ 0x0021）

| 値 | ボーレート |
|----|----------|
| 0  | 2400 bps |
| 1  | 4800 bps |
| 2  | 9600 bps (デフォルト) |

## データ形式の詳細

### 土壌水分（レジスタ 0x0000）
- **型**: 16ビット符号なし整数
- **計算**: 受信値 ÷ 10 = %
- **例**: 0x028E (658) → 65.8%
- **範囲**: 0-100%

### 温度（レジスタ 0x0001）
- **型**: 16ビット符号付き整数 (補数表現)
- **計算**: 受信値 ÷ 10 = °C
- **例1**: 0x00FB (251) → 25.1°C
- **例2**: 0xFF9B (-101) → -10.1°C
- **範囲**: -10～+60°C

### 電気伝導度 (EC)（レジスタ 0x0002）
- **型**: 16ビット符号なし整数
- **単位**: µS/cm (マイクロシーメンス/cm)
- **例**: 0x03E8 (1000) → 1000 µS/cm
- **範囲**: 0～65535 µS/cm

### pH値（レジスタ 0x0003）
- **型**: 16ビット符号なし整数
- **計算**: 受信値 ÷ 10 = pH
- **例**: 0x0038 (56) → 5.6
- **範囲**: 0～14 pH

## Modbus RTUコマンド例

### すべてのセンサーデータを読む（moisture, temp, EC, pH）

**リクエスト** (16進数):
```
01 03 00 00 00 04 44 09
```

| 要素 | 値 | 説明 |
|------|-----|------|
| スレーブアドレス | `01` | デバイス1 |
| Function Code | `03` | Read Holding Registers |
| レジスタ開始アドレス | `00 00` | 0x0000から開始 |
| 読み取り数 | `00 04` | 4個のレジスタ (0x0000-0x0003) |
| CRC | `44 09` | チェックサム |

**レスポンス（例）**:
```
01 03 08 02 92 FF 9B 03 E8 00 38 C4 F8
```

| データ | 値 | 説明 |
|--------|-----|------|
| スレーブアドレス | `01` | デバイス1 |
| Function Code | `03` | Read Holding Registers |
| バイト数 | `08` | 8バイト (4レジスタ × 2) |
| **土壌水分** | `02 92` | 0x0292 = 658 ��� 65.8% |
| **温度** | `FF 9B` | 0xFF9B = -101 → -10.1°C |
| **EC** | `03 E8` | 0x03E8 = 1000 → 1000 µS/cm |
| **pH値** | `00 38` | 0x0038 = 56 → 5.6 |
| CRC | `C4 F8` | チェックサム |

### 土壌水分のみを読む

**リクエスト**:
```
01 03 00 00 00 01 84 0A
```

**レスポンス（例）**:
```
01 03 02 02 8E 39 CB
```
- 0x028E (658) → 65.8%

### 温度のみを読む

**リクエスト**:
```
01 03 00 01 00 01 B5 CB
```

**レスポンス（例）**:
```
01 03 02 00 FB B8 48
```
- 0x00FB (251) → 25.1°C

### 複数レジスタに対応した負の温度値

**レスポンス（負の温度例）**:
```
01 03 02 FF 9B 78 4C
```
- 0xFF9B (-101 in two's complement) → -10.1°C

### デバイスアドレスを変更する（1 → 2）

**リクエスト** (Function Code 0x06):
```
01 06 00 20 00 02 88 04
```

| 要素 | 値 | 説明 |
|------|-----|------|
| スレーブアドレス | `01` | 現在のアドレス |
| Function Code | `06` | Write Single Register |
| レジスタアドレス | `00 20` | 0x0020 (デバイスアドレスレジスタ) |
| 値 | `00 02` | 新しいアドレス 2 |
| CRC | `88 04` | チェックサム |

**レスポンス** (エコーバック):
```
01 06 00 20 00 02 88 04
```

> **注意**: アドレス変更後、センサーを再起動して設定を反映させる必要があります。

### ボーレートを変更する（9600 → 19200 bps）

**リクエスト** (Function Code 0x06):
```
01 06 00 21 00 03 48 44
```

| 要素 | 値 | 説明 |
|------|-----|------|
| スレーブアドレス | `01` | デバイス1 |
| Function Code | `06` | Write Single Register |
| レジスタアドレス | `00 21` | 0x0021 (ボーレートレジスタ) |
| 値 | `00 03` | 19200 bps |
| CRC | `48 44` | チェックサム |

**レスポンス** (エコーバック):
```
01 06 00 21 00 03 48 44
```

> **注意**: ボーレート変更後、M5側の `SERIAL_BAUD` を同じボーレートに設定してから、センサーを再起動してください。

### EC温度係数を変更する（0 → 20）

**リクエスト** (Function Code 0x06):
```
01 06 00 22 00 14 C8 44
```

| 要素 | 値 | 説明 |
|------|-----|------|
| スレーブアドレス | `01` | デバイス1 |
| Function Code | `06` | Write Single Register |
| レジスタアドレス | `00 22` | 0x0022 (EC温度係数) |
| 値 | `00 14` | 20 (2.0%) |
| CRC | `C8 44` | チェックサム |

## Arduino IDEでの使用

### ModbusMasterライブラリを使う場���

```cpp
#include <ModbusMaster.h>

ModbusMaster node;

void setup() {
    Serial.begin(115200);
    node.begin(1, Serial);  // スレーブアドレス1、Serialポート
}

void loop() {
    // すべてのセンサーデータを読む
    uint8_t result = node.readHoldingRegisters(0x0000, 4);
    
    if (result == node.ku8MBSuccess) {
        // 土壌水分
        int moisture_raw = node.getResponseBuffer(0);
        float moisture = moisture_raw / 10.0;
        
        // 温度 (符号付き)
        int16_t temp_raw = (int16_t)node.getResponseBuffer(1);
        float temperature = temp_raw / 10.0;
        
        // EC
        uint16_t ec_raw = node.getResponseBuffer(2);
        float ec = ec_raw;
        
        // pH
        int ph_raw = node.getResponseBuffer(3);
        float ph = ph_raw / 10.0;
        
        Serial.print("Moisture: ");
        Serial.print(moisture);
        Serial.println(" %");
        Serial.print("Temperature: ");
        Serial.print(temperature);
        Serial.println(" C");
        Serial.print("EC: ");
        Serial.print(ec);
        Serial.println(" uS/cm");
        Serial.print("pH: ");
        Serial.println(ph);
    } else {
        Serial.println("Error reading registers");
    }
    
    delay(2000);
}
```

### デバイスアドレスを変更する

```cpp
#include <ModbusMaster.h>

ModbusMaster node;

void changeDeviceAddress(uint8_t oldAddress, uint8_t newAddress) {
    node.begin(oldAddress, Serial);
    
    // アドレスレジスタ (0x0020) に新しいアドレスを書き込み
    node.writeSingleRegister(0x0020, newAddress);
    
    if (node.ku8MBSuccess == node.u8lastError) {
        Serial.print("Address changed to: ");
        Serial.println(newAddress);
    } else {
        Serial.println("Failed to change address");
    }
}

void setup() {
    Serial.begin(115200);
    // 現在のアドレス1から新しいアドレス2に変更
    changeDeviceAddress(1, 2);
    delay(1000);  // センサー再起動待ち
}

void loop() {}
```

### ボーレートを変更する

```cpp
#include <ModbusMaster.h>

ModbusMaster node;

void changeBaudRate(uint8_t baudRateCode) {
    // baudRateCode: 0=2400, 1=4800, 2=9600, 3=19200, 4=38400
    node.begin(1, Serial);  // 現在のボーレート（9600）で通信
    
    // ボーレートレジスタ (0x0021) に値を書き込み
    node.writeSingleRegister(0x0021, baudRateCode);
    
    if (node.ku8MBSuccess == node.u8lastError) {
        Serial.print("Baud rate change command sent: ");
        Serial.println(baudRateCode);
    } else {
        Serial.println("Failed to change baud rate");
    }
}

void setup() {
    Serial.begin(115200);
    // ボーレート設定値 3 = 19200 bps に変更
    changeBaudRate(3);
    delay(2000);  // センサー再起動待ち
    
    // M5側のボーレートも変更
    Serial.end();
    Serial.begin(19200);
}

void loop() {}
```

### 注意：温度の符号付き処理

温度は負の値に対応しているため、**符号付き整数として処理する必要があります**：

```cpp
// 間違い（符号なしで処理）
uint16_t temp_raw = node.getResponseBuffer(1);  // -101 が符号なしになる
float temperature = temp_raw / 10.0;  // 誤った値になる

// 正しい（符号付きで処理）
int16_t temp_raw = (int16_t)node.getResponseBuffer(1);  // -101 として解釈される
float temperature = temp_raw / 10.0;  // -10.1 になる
```

## トラブルシューティング

### CRCエラーが頻発する場合

1. ボーレート設定を確認
   - M5側: コードの `SERIAL_BAUD` がセンサーと一致しているか？
   - デフォルト: 9600 bps
   - 現在の設定はレジスタ 0x0021 で確認可能

2. RS485接続の安定性を確認
   - ケーブルが正しく接続されているか
   - 接触不良がないか

3. 終端抵抗を追加
   - 長いケーブルの場合、120Ω 抵抗をセンサーの A-B 間に接続

### センサーが応答しない場合

- スレーブアドレスが正しいか確認（デフォルト: 1、現在の設定はレジスタ 0x0020 で確認）
- センサーの電源供給を確認（5V, GND）
- RS485 A/B線が正しく接続されているか

### 異常な値が読める場合

1. **温度が異常に大きい値の場合**
   - 温度を符号付き整数として処理しているか確認
   - 補数表現の負の値が正の大きな値として解釈されていないか

2. **EC値が不安定な場合**
   - 温度係数 (0x0022) を確認
   - キャリブレーションが必要な場合がある

3. **pH値が読めない場合**
   - センサーの埋め込み深度を確認
   - キャリブレーション値を確認

## 設定値読み取り例

### 現在のデバイスアドレスを確認

```cpp
uint8_t result = node.readHoldingRegisters(0x0020, 1);
if (result == node.ku8MBSuccess) {
    uint16_t address = node.getResponseBuffer(0);
    Serial.print("Current device address: ");
    Serial.println(address);
}
```

### 現在のボーレート設定を確認

```cpp
uint8_t result = node.readHoldingRegisters(0x0021, 1);
if (result == node.ku8MBSuccess) {
    uint16_t baudRateCode = node.getResponseBuffer(0);
    Serial.print("Baud rate code: ");
    Serial.println(baudRateCode);
    // 0=2400, 1=4800, 2=9600, 3=19200, 4=38400
}
```

## 参考資料

- [DFROBOT SEN0604 公式Wiki](https://wiki.dfrobot.com/sen0604/docs/20297)
- [Modbus RTU仕様](http://www.modbus.org/)
- [ModbusMaster ライブラリ](https://github.com/4-20ma/ModbusMaster)

---

**更新日**: 2026-06-13
**ドキュメントバージョン**: 2.1
