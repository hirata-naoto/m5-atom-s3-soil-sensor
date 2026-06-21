#include <Arduino.h>
#include <M5Unified.h>
#include <ModbusMaster.h> // ModbusMaster ライブラリをインクルード

// ----------------------------------------------------
// 設定項目
// ----------------------------------------------------
#define RX_PIN  5  // 環境に合わせて調整してください
#define TX_PIN  6  // 環境に合わせて調整してください
#define TXE_PIN 7  // 環境に合わせて調整してください

// 変更前の古いアドレス（ModbusMasterノードオブジェクトの初期化に使用）
const uint8_t OLD_ADDRESS = 0x01; 

// 変更後の新しいアドレス
const uint8_t NEW_ADDRESS = 0x02; 

// アドレス書き込み用のレジスタアドレス (0x07D0 = 十進数で 2000)
const uint16_t REG_ADDRESS = 0x07D0; 

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
  M5.Display.println("ModbusMaster Config");
  M5.Display.println("-----------------");

  // PCモニター用シリアル
  Serial.begin(115200);
  delay(500);
  Serial.println("\n--- SEN0605 Address Configurator (ModbusMaster) ---");

  // センサー通信用シリアル初期化
  SensorSerial.begin(9600, SERIAL_8N1, RX_PIN, TX_PIN);
  pinMode(TXE_PIN, OUTPUT);
  digitalWrite(TXE_PIN, LOW);
    
  // ModbusMasterにシリアル通信と初期（変更前）のSlave IDを設定
  node.begin(OLD_ADDRESS, SensorSerial);

  // ModbusMasterのコールバック設定
  node.preTransmission(preTransmission);
  node.postTransmission(postTransmission);
  
  // 画面とシリアルへ送信開始を表示
  Serial.printf("Changing Address: 0x%02X -> 0x%02X ...\n", OLD_ADDRESS, NEW_ADDRESS);
  M5.Display.setTextColor(YELLOW);
  M5.Display.printf("Changing Addr:\n0x%02X -> 0x%02X\n\n", OLD_ADDRESS, NEW_ADDRESS);
  M5.Display.println("Sending Modbus cmd...");

  // Write Single Register (Function Code 0x06) を実行
  // 指定したレジスタ(0x07D0)に、新しいアドレス値を書き込みます
  uint8_t result = node.writeSingleRegister(REG_ADDRESS, NEW_ADDRESS);
  delay(100);  // センサー応答時間待ち

  // 結果の判定 (ku8MBSuccess = 0x00 が成功)
  if (result == node.ku8MBSuccess) {
    Serial.println("Response: Success!");
    Serial.println("Configuration command sent successfully.");
    Serial.println("Verifying new address by reading the same register...");

    M5.Display.setTextColor(GREEN);
    M5.Display.println("\n[SUCCESS]");
    M5.Display.println("Write: OK");
    M5.Display.println("Verifying...");

    delay(200);

    // 新しいアドレスで再初期化して、設定レジスタを読み返して確認
    node.begin(NEW_ADDRESS, SensorSerial);
    node.preTransmission(preTransmission);
    node.postTransmission(postTransmission);
  
    uint8_t verifyResult = node.readHoldingRegisters(REG_ADDRESS, 1);

    if (verifyResult == node.ku8MBSuccess) {
      uint16_t readBackAddress = node.getResponseBuffer(0);
      Serial.printf("Read back register 0x%04X = 0x%04X\n", REG_ADDRESS, readBackAddress);

      if ((readBackAddress & 0x00FF) == NEW_ADDRESS) {
        Serial.printf("Verification OK: address changed to 0x%02X\n", NEW_ADDRESS);
        Serial.println("Please power cycle the sensor to apply changes.");

        M5.Display.setTextColor(GREEN);
        M5.Display.println("Verify: OK");
        M5.Display.printf("Addr=0x%02X\n", NEW_ADDRESS);
        M5.Display.println("Power cycle\nthe sensor!");
      } else {
        Serial.printf("Verification NG: expected 0x%02X but got 0x%02X\n", NEW_ADDRESS, readBackAddress & 0x00FF);

        M5.Display.setTextColor(RED);
        M5.Display.println("Verify: NG");
        M5.Display.printf("Read=0x%02X\n", readBackAddress & 0x00FF);
      }
    } else {
      Serial.printf("Verification failed. Error Code: 0x%02X\n", verifyResult);
      Serial.println("The address may have changed, but readback using the new address failed.");

      M5.Display.setTextColor(RED);
      M5.Display.println("Verify: ERR");
      M5.Display.printf("Err=0x%02X\n", verifyResult);
    }
  } else {
    // 失敗した場合はエラーコードを表示
    Serial.printf("Error: Modbus communication failed. Error Code: 0x%02X\n", result);
    Serial.println("Check wiring, power, and pins.");

    M5.Display.setTextColor(RED);
    M5.Display.println("\n[ERROR]");
    M5.Display.printf("Err Code: 0x%02X\n", result);
    M5.Display.println("Check wiring\nand power.");
  }
}

void loop() {
  M5.update();
}
