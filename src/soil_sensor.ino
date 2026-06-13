/*
 * M5 Atom S3 + DFROBOT SENS0604 RS485 Soil Sensor System
 * 
 * 土壌水分・温度・pH測定システム
 * M5Unified + ModbusMasterライブラリ版
 * 画面表示対応
 * 
 * ピン配置 (M5 Atom S3):
 *   GPIO 5 (TX) -> RS485 DI (Data In)
 *   GPIO 6 (RX) -> RS485 RO (Data Out)
 *   5V -> RS485 VCC
 *   GND -> RS485 GND
 * 
 * 必要なライブラリ:
 *   - M5Unified
 *   - ModbusMaster (Rob Tillaart版)
 * 
 * 作成日: 2026-06-13
 */

#include <M5Unified.h>
#include <ModbusMaster.h>
#include <HardwareSerial.h>

// ===== 設定値 =====
#define SERIAL_BAUD 9600          // RS485シリアル通信速度
#define SENSOR_ADDRESS 1           // センサーのModbusスレーブアドレス
#define INTERVAL_MS 2000           // 計測間隔（ミリ秒）

// RS485 UART設定
#define RS485_RX_PIN 6             // RX ピン
#define RS485_TX_PIN 5             // TX ピン

// Modbusレジスタアドレス
#define REG_MOISTURE 0x0000        // 土壌水分 (0-100%)
#define REG_TEMPERATURE 0x0001     // 温度 (-10 ~ +60°C)
#define REG_EC 0x0002              // EC値 (0-20 mS/cm)
#define REG_PH 0x0003              // pH値 (0-14)

// ===== グローバル変数 =====
HardwareSerial RS485Serial(1);     // UART_NUM_1
ModbusMaster node;
unsigned long lastReadTime = 0;

// データ格納用
struct SensorData {
  float moisture;
  float temperature;
  float ec;
  float ph;
  uint8_t status;  // 0=OK, 1=Error
  String errorMsg;
} sensorData = {0.0, 0.0, 0.0, 0.0, 0, ""};

// ===== RS485プリ/ポストトランスミッション制御 =====
void preTransmission() {
  // RS485ドライバを送信モードに設定（必要に応じて）
}

void postTransmission() {
  // RS485ドライバを受信モードに設定（必要に応じて）
}

// ===== LED表示用ヘルパー関数 =====
void setLEDColor(uint32_t color) {
  // M5 Atom S3の内蔵LEDを指定色で点灯
  // color: 0xRRGGBB形式
  M5.Led.setColor(color);
}

// ===== 画面表示関数 =====
void displaySensorData() {
  auto& display = M5.Display;
  display.setTextSize(1);
  display.fillScreen(TFT_BLACK);
  display.setTextColor(TFT_WHITE, TFT_BLACK);
  
  // タイトル
  display.setCursor(0, 0);
  display.println("=== Soil Sensor ===");
  display.println("");
  
  if (sensorData.status == 0) {
    // データ表示（成功時）
    display.setTextColor(TFT_GREEN, TFT_BLACK);
    
    display.print("Moisture: ");
    display.print(sensorData.moisture);
    display.println(" %");
    
    display.print("Temp: ");
    display.print(sensorData.temperature);
    display.println(" C");
    
    display.print("EC: ");
    display.print(sensorData.ec);
    display.println(" mS/cm");
    
    display.print("PH: ");
    display.print(sensorData.ph);
    display.println("");
    
    display.setTextColor(TFT_GREEN);
    display.println("[OK]");
    
    // LED: 緑色
    setLEDColor(0x00FF00);
  } else {
    // エラー表示
    display.setTextColor(TFT_RED, TFT_BLACK);
    display.println("ERROR!");
    display.println(sensorData.errorMsg);
    
    display.setTextColor(TFT_WHITE);
    display.println("");
    display.println("Check wiring...");
    
    // LED: 赤色
    setLEDColor(0xFF0000);
  }
  
  // 更新時刻
  display.setTextColor(TFT_CYAN);
  display.print("Last: ");
  display.println(millis() / 1000);
}

// ===== センサー読み込み関数 =====
bool readSensorData() {
  bool success = true;
  
  // 土壌水分を読む (レジスタアドレス 0x0000)
  uint8_t result = node.readInputRegisters(REG_MOISTURE, 1);
  
  if (result == node.ku8MBSuccess) {
    uint16_t moisture_raw = node.getResponseBuffer(0);
    sensorData.moisture = moisture_raw / 10.0;
  } else {
    sensorData.status = 1;
    sensorData.errorMsg = "Moisture read failed";
    success = false;
  }
  
  delay(100); // センサーの応答待ち
  
  // 温度を読む (レジスタアドレス 0x0001)
  result = node.readInputRegisters(REG_TEMPERATURE, 1);
  
  if (result == node.ku8MBSuccess) {
    int16_t temp_raw = (int16_t)node.getResponseBuffer(0);
    sensorData.temperature = temp_raw / 10.0;
  } else {
    sensorData.status = 1;
    sensorData.errorMsg = "Temp read failed";
    success = false;
  }
  
  delay(100);
  
  // EC値を読む (レジスタアドレス 0x0002)
  result = node.readInputRegisters(REG_EC, 1);
  
  if (result == node.ku8MBSuccess) {
    uint16_t ec_raw = node.getResponseBuffer(0);
    sensorData.ec = ec_raw / 10.0;
  } else {
    sensorData.status = 1;
    sensorData.errorMsg = "EC read failed";
    success = false;
  }
  
  delay(100);
  
  // PH値を読む (レジスタアドレス 0x0003)
  result = node.readInputRegisters(REG_PH, 1);
  
  if (result == node.ku8MBSuccess) {
    uint16_t ph_raw = node.getResponseBuffer(0);
    sensorData.ph = ph_raw / 10.0;
  } else {
    sensorData.status = 1;
    sensorData.errorMsg = "PH read failed";
    success = false;
  }
  
  if (success) {
    sensorData.status = 0;
    sensorData.errorMsg = "";
  }
  
  return success;
}

// ===== シリアル出力 =====
void printSerialData() {
  Serial.println("=== Sensor Reading ===");
  
  if (sensorData.status == 0) {
    Serial.print("Moisture: ");
    Serial.print(sensorData.moisture);
    Serial.println(" %");
    
    Serial.print("Temperature: ");
    Serial.print(sensorData.temperature);
    Serial.println(" C");
    
    Serial.print("EC: ");
    Serial.print(sensorData.ec);
    Serial.println(" mS/cm");
    
    Serial.print("PH: ");
    Serial.println(sensorData.ph);
  } else {
    Serial.println("ERROR: " + sensorData.errorMsg);
  }
  
  Serial.println("");
}

// ===== 初期化 =====
void setup() {
  auto cfg = M5.config();
  M5.begin(cfg);
  
  // ディスプレイ初期化
  auto& display = M5.Display;
  display.setRotation(1);
  display.fillScreen(TFT_BLACK);
  display.setTextSize(1);
  display.setTextColor(TFT_WHITE);
  
  // シリアル通信初期化（デバッグ用）
  Serial.begin(115200);
  delay(100);
  
  M5_LOGI("\n\n=== M5 Atom S3 Soil Sensor System ===");
  M5_LOGI("M5Unified + ModbusMaster version");
  M5_LOGI("Starting up...");
  M5_LOGI("RX Pin: 6, TX Pin: 5");
  
  // RS485 UART初期化
  RS485Serial.begin(SERIAL_BAUD, SERIAL_8N1, RS485_RX_PIN, RS485_TX_PIN);
  
  // ModbusMaster初期化
  node.begin(SENSOR_ADDRESS, RS485Serial);
  node.preTransmission(preTransmission);
  node.postTransmission(postTransmission);
  
  // 画面に初期化中と表示
  display.fillScreen(TFT_BLACK);
  display.setCursor(0, 0);
  display.setTextColor(TFT_BLUE);
  display.println("Initializing...");
  
  // LED: 青色（初期化中）
  setLEDColor(0x0000FF);
  delay(500);
  
  // LED: オフ（初期状態）
  setLEDColor(0x000000);
  
  M5_LOGI("Ready!");
  M5_LOGI("");
  
  // 画面にReady表示
  displaySensorData();
}

// ===== メインループ =====
void loop() {
  M5.update();
  
  unsigned long currentTime = millis();
  
  // 計測間隔チェック
  if (currentTime - lastReadTime >= INTERVAL_MS) {
    lastReadTime = currentTime;
    
    // センサー読み込み
    readSensorData();
    
    // シリアル出力
    printSerialData();
    
    // 画面更新
    displaySensorData();
  }
  
  // ボタン処理（オプション）
  if (M5.Btn.wasPressed()) {
    // ボタン押下時は即座に計測を実行
    M5_LOGI("Button pressed! Reading sensor now...");
    lastReadTime = 0; // 次の計測をすぐに実行
    
    // LED: 黄色点灯
    setLEDColor(0xFFFF00);
    delay(100);
    
    // LED: オフ
    setLEDColor(0x000000);
  }
  
  delay(10);
}

/*
 * ===== トラブルシューティング =====
 * 
 * 1. データが読めない場合:
 *    - wiring.mdの配線を確認
 *    - RX(6)/TX(5)のピンが正しいか確認
 *    - センサーのボーレートが9600 bpsか確認
 * 
 * 2. 画面に表示されない場合:
 *    - M5 Atom S3のディスプレイ接続を確認
 *    - ライブラリが正しくインストールされているか確認
 * 
 * 3. Modbusエラーが出る場合:
 *    - RS485接続を確認
 *    - 終端抵抗（120Ω）を追加してみる
 *    - ケーブルの接触確認
 * 
 * 4. センサーの応答がない場合:
 *    - センサーの電源供給を確認
 *    - SENSOR_ADDRESSが正しいか確認
 *    - センサーが初期化中でないか確認（起動後1-2秒待つ）
 */
