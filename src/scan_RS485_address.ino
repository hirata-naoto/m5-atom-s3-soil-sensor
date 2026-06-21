#include <Arduino.h>
#include <M5Unified.h>
#include <ModbusMaster.h> // ModbusMaster ライブラリをインクルード

// ----------------------------------------------------
// 設定項目
// ----------------------------------------------------
#define RX_PIN  5  // 環境に合わせて調整してください
#define TX_PIN  6  // 環境に合わせて調整してください
#define TXE_PIN 7  // 環境に合わせて調整してください

// スキャン対象のアドレス範囲
const uint8_t SCAN_START_ADDRESS = 0x01;
const uint8_t SCAN_END_ADDRESS   = 0xF7;

// スキャン対象のレジスタアドレス (0x0000 = 通常読み込み可能なレジスタ)
const uint16_t PROBE_REGISTER = 0x0000;

// レジスタ数
const uint8_t REGISTER_COUNT = 1;

// スキャン間隔（ミリ秒）
const uint16_t SCAN_INTERVAL = 200;

// タイムアウト時間（ミリ秒）
const uint32_t RESPONSE_TIMEOUT = 1000;

// ----------------------------------------------------
// 通信インスタンス設定
// ----------------------------------------------------
HardwareSerial SensorSerial(1);
ModbusMaster node;

// ===== RS485プリ/ポストトランスミッション制御 =====
void preTransmission() {
    digitalWrite(TXE_PIN, HIGH);   // RS485ドライバを送信モードに設定
}

void postTransmission() {
    SensorSerial.flush();
    delay(6);
    digitalWrite(TXE_PIN, LOW);    // RS485ドライバを受信モードに設定
}

void setup() {
  // M5Unifiedの初期化
  auto cfg = M5.config();
  M5.begin(cfg);

  // 画面の初期設定
  M5.Log.setLogLevel(m5::log_target_display, ESP_LOG_NONE);
  M5.Display.setRotation(2);
  M5.Display.setFont(&fonts::Font0);
  M5.Display.setTextSize(1.5);
  
  M5.Display.clear(BLACK);
  M5.Display.setTextColor(GREEN);
  M5.Display.println("SEN0605 Address Scanner");
  M5.Display.println("-----------------------");

  // PCモニター用シリアル
  Serial.begin(115200);
  delay(500);
  Serial.println("\n--- SEN0605 Address Scanner ---");
  Serial.printf("Scanning addresses 0x%02X to 0x%02X...\n", SCAN_START_ADDRESS, SCAN_END_ADDRESS);
  Serial.println("");

  // センサー通信用シリアル初期化
  SensorSerial.begin(9600, SERIAL_8N1, RX_PIN, TX_PIN);
  pinMode(TXE_PIN, OUTPUT);
  digitalWrite(TXE_PIN, LOW);
  
  // 画面に情報を表示
  M5.Display.setTextColor(YELLOW);
  M5.Display.printf("Scanning:\n0x%02X - 0x%02X\n\n", SCAN_START_ADDRESS, SCAN_END_ADDRESS);
  M5.Display.setTextColor(WHITE);
  M5.Display.println("Found devices:");

  uint8_t foundCount = 0;
  uint32_t startTime = millis();

  // アドレススキャン実行
  for (uint8_t address = SCAN_START_ADDRESS; address <= SCAN_END_ADDRESS; address++) {
    // ModbusMasterをこのアドレスで初期化
    node.begin(address, SensorSerial);
    node.preTransmission(preTransmission);
    node.postTransmission(postTransmission);

    // Holding Register読み込みを試行
    uint8_t result = node.readHoldingRegisters(PROBE_REGISTER, REGISTER_COUNT);

    // 成功または不正レジスタアドレスなら「デバイスあり」とみなす
    bool devicePresent =
        (result == node.ku8MBSuccess) ||
        (result == node.ku8MBIllegalDataAddress);

    if (devicePresent) {
      foundCount++;
      if (result == node.ku8MBSuccess) {
        Serial.printf("✓ Device found at address: 0x%02X\n", address);
      } else {
        Serial.printf("✓ Device found at address: 0x%02X (Illegal register address)\n", address);
      }
      M5.Display.setTextColor(GREEN);
      M5.Display.printf("  0x%02X\n", address);
      delay(SCAN_INTERVAL);
    } else {
      // 通信失敗（デバイスなし）
      Serial.printf("  - No device at 0x%02X (Error: 0x%02X)\n", address, result);
      delay(50);  // 短い待機
    }

    M5.update();
  }

  uint32_t scanTime = millis() - startTime;

  // 結果表示
  Serial.println("");
  Serial.println("=== Scan Result ===");
  Serial.printf("Devices found: %d\n", foundCount);
  Serial.printf("Scan time: %lu ms\n", scanTime);

  M5.Display.setTextColor(CYAN);
  M5.Display.println("");
  M5.Display.printf("Found: %d device(s)\n", foundCount);
  M5.Display.printf("Time: %lu ms\n", scanTime);
  
  M5.Display.setTextColor(WHITE);
  M5.Display.println("\nScan completed.");
  M5.Display.println("Press button to exit.");
}

void loop() {
  M5.update();
}
