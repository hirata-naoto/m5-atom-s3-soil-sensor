/*
 * M5 Atom S3 + DFROBOT SENS0604 & SEN0605 RS485 Dual Soil Sensor System
 * * 土壌水分・温度・pH・EC ＋ NPK（窒素・リン・カリウム）測定システム
 * M5Unified + ModbusMasterライブラリ版
 * 画面表示対応
 * * ピン配置 (M5 Atom S3):
 * GPIO 5 (RX)  -> RS485 RO (Data Out)
 * GPIO 6 (TX)  -> RS485 DI (Data In)
 * GPIO 7 (TXE) -> RS485 DE (Data Out Enable)
 * 5V -> RS485 VCC
 * GND -> RS485 GND
 * * 必要なライブラリ:
 * - M5Unified
 * - ModbusMaster (Rob Tillaart版)
 * * 作成日: 2026-06-15
 */

#include <M5Unified.h>
#include <ModbusMaster.h>

// ===== 設定値 =====
#define SERIAL_BAUD 9600          // RS485シリアル通信速度
#define INTERVAL_MS 3000          // 計測間隔（複数センサーのため3秒に延長）

// 各センサーのModbusスレーブアドレス
#define ADDR_SOIL_4IN1 1           // SENS0604 (水分, 温度, EC, pH)
#define ADDR_SOIL_NPK  2           // SEN0605 (窒素, リン, カリウム) ※要事前設定

// RS485 UART設定
#define RS485_RX_PIN  5             // RX ピン
#define RS485_TX_PIN  6             // TX ピン
#define RS485_TXE_PIN 7             // DE ピン

// SENS0604 レジスタアドレス
#define REG_MOISTURE 0x0000        // 土壌水分 (0-100%)
#define REG_TEMPERATURE 0x0001     // 温度 (-10 ~ +60°C)
#define REG_EC 0x0002              // EC値 (0-20 mS/cm)
#define REG_PH 0x0003              // pH値 (0-14)

// SEN0605 レジスタアドレス
#define REG_NITROGEN  0x001E       // 窒素 (N) mg/kg
#define REG_PHOSPHORUS 0x001F      // リン (P) mg/kg
#define REG_POTASSIUM 0x0020       // カリウム (K) mg/kg

// ===== グローバル変数 =====
ModbusMaster node;                 // 1つのノードインスタンスをアドレスを切り替えて使い回します
unsigned long lastReadTime = 0;

// SENS0604 データ構造体
struct SensorData4in1 {
  float moisture;
  float temperature;
  float ec;
  float ph;
  bool success;
} data4in1 = {0.0, 0.0, 0.0, 0.0, false};

// SEN0605 データ構造体
struct SensorDataNPK {
  uint16_t nitrogen;
  uint16_t phosphorus;
  uint16_t potassium;
  bool success;
} dataNPK = {0, 0, 0, false};

// ===== RS485プリ/ポストトランスミッション制御 =====
void preTransmission() {
    digitalWrite(RS485_TXE_PIN, HIGH);   // RS485ドライバを送信モードに設定
}

void postTransmission() {
    Serial1.flush();
    delay(2);
    digitalWrite(RS485_TXE_PIN, LOW);    // RS485ドライバを受信モードに設定
}

// ===== LED表示用ヘルパー関数 =====
void setLEDColor(uint32_t color) {
  M5.Led.setColor(0, color);
}

// ===== 画面表示関数 =====
void displaySensorData() {
  auto& display = M5.Display;
  display.setTextSize(1);
  display.startWrite();
  display.fillScreen(TFT_BLACK);
  display.setTextColor(TFT_WHITE, TFT_BLACK);
  
  display.setCursor(0, 0);
  display.println("== Dual Soil Sen ==");
  
  // SENS0604 のデータ表示
  if (data4in1.success) {
    display.setTextColor(TFT_GREEN, TFT_BLACK);
    display.printf("Mst:%.1f%% T:%.1fC\n", data4in1.moisture, data4in1.temperature);
    display.printf("EC :%.1fmS pH:%.1f\n", data4in1.ec, data4in1.ph);
  } else {
    display.setTextColor(TFT_RED, TFT_BLACK);
    display.println("4in1: Read Error");
  }

  display.setTextColor(TFT_WHITE, TFT_BLACK);
  display.println("-------------------");

  // SEN0605 (NPK) のデータ表示
  if (dataNPK.success) {
    display.setTextColor(TFT_GREEN, TFT_BLACK);
    display.printf("N: %d mg/kg\n", dataNPK.nitrogen);
    display.printf("P: %d mg/kg\n", dataNPK.phosphorus);
    display.printf("K: %d mg/kg\n", dataNPK.potassium);
  } else {
    display.setTextColor(TFT_RED, TFT_BLACK);
    display.println("NPK : Read Error");
  }
  
  // 総合ステータスによるLED制御
  if (data4in1.success && dataNPK.success) {
    setLEDColor(0x00FF00); // 両方OKなら緑
  } else if (data4in1.success || dataNPK.success) {
    setLEDColor(0xFFFF00); // 片方NGなら黄
  } else {
    setLEDColor(0xFF0000); // 両方NGなら赤
  }
  
  // 更新時刻
  display.setTextColor(TFT_CYAN, TFT_BLACK);
  display.println("");
  display.printf("Last: %lu s\n", millis() / 1000);

  display.endWrite();
}

// ===== SENS0604 読み込み関数 =====
bool readSENS0604() {
  node.begin(ADDR_SOIL_4IN1, Serial1); // 対象のアドレスに切り替え
  delay(10); // 安定のためのわずかなウエイト
  
  uint8_t result = node.readHoldingRegisters(REG_MOISTURE, 4);
  
  if (result == node.ku8MBSuccess) {
    data4in1.moisture = node.getResponseBuffer(0) / 10.0;
    data4in1.temperature = ((int16_t)node.getResponseBuffer(1)) / 10.0;
    data4in1.ec = node.getResponseBuffer(2) / 10.0;
    data4in1.ph = node.getResponseBuffer(3) / 10.0;
    data4in1.success = true;
    return true;
  } else {
    data4in1.success = false;
    return false;
  }
}

// ===== SEN0605 (NPK) 読み込み関数 =====
bool readSEN0605() {
  node.begin(ADDR_SOIL_NPK, Serial1); // 対象のアドレスに切り替え
  delay(10);
  
  // 0x001E(N) から 3つのレジスタ(N, P, K)を読み出す
  uint8_t result = node.readHoldingRegisters(REG_NITROGEN, 3);
  
  if (result == node.ku8MBSuccess) {
    dataNPK.nitrogen   = node.getResponseBuffer(0); // 窒素
    dataNPK.phosphorus = node.getResponseBuffer(1); // リン
    dataNPK.potassium  = node.getResponseBuffer(2); // カリウム
    dataNPK.success = true;
    return true;
  } else {
    dataNPK.success = false;
    return false;
  }
}

// ===== シリアル出力 =====
void printSerialData() {
  Serial.println("====== Sensor Readings ======");
  
  if (data4in1.success) {
    Serial.printf("[SENS0604] Moist: %.1f %%, Temp: %.1f C, EC: %.1f mS/cm, pH: %.1f\n", 
                  data4in1.moisture, data4in1.temperature, data4in1.ec, data4in1.ph);
  } else {
    Serial.println("[SENS0604] Read Failed!");
  }
  
  if (dataNPK.success) {
    Serial.printf("[SEN0605 ] Nitrogen: %d mg/kg, Phosphorus: %d mg/kg, Potassium: %d mg/kg\n", 
                  dataNPK.nitrogen, dataNPK.phosphorus, dataNPK.potassium);
  } else {
    Serial.println("[SEN0605 ] Read Failed!");
  }
  Serial.println("=============================\n");
}

// ===== 初期化 =====
void setup() {
  auto cfg = M5.config();
  M5.begin(cfg);
  
  // ディスプレイ初期化
  auto& display = M5.Display;
  display.setRotation(0);
  display.fillScreen(TFT_BLACK);
  
  // シリアル通信初期化（デバッグ用）
  Serial.begin(115200);
  delay(100);
  
  M5_LOGI("\n\n=== M5 Atom S3 Dual Soil Sensor System ===");
  
  // RS485 UART初期化
  Serial1.begin(SERIAL_BAUD, SERIAL_8N1, RS485_RX_PIN, RS485_TX_PIN);
  pinMode(RS485_TXE_PIN, OUTPUT);
  digitalWrite(RS485_TXE_PIN, LOW);

  // ModbusMasterのコールバック設定
  node.preTransmission(preTransmission);
  node.postTransmission(postTransmission);
  
  display.fillScreen(TFT_BLACK);
  display.setCursor(0, 0);
  display.setTextColor(TFT_BLUE);
  display.println("Initializing...");
  
  setLEDColor(0x0000FF); // 青色
  delay(1000);
  setLEDColor(0x000000);
  
  M5_LOGI("Ready!");
  displaySensorData();
}

// ===== メインループ =====
void loop() {
  M5.update();
  
  unsigned long currentTime = millis();
  
  if (currentTime - lastReadTime >= INTERVAL_MS) {
    lastReadTime = currentTime;
    
    // 1. SENS0604の読み込み
    readSENS0604();
    delay(100); // 連続通信によるバスの衝突を防ぐため、センサー間に少し待ち時間を入れる
    
    // 2. SEN0605の読み込み
    readSEN0605();
    
    // 3. 出力処理
    printSerialData();
    displaySensorData();
  }
  
  // ボタン押下時は即座に計測を実行
  if (M5.BtnA.wasPressed()) {
    M5_LOGI("Button pressed! Reading sensors now...");
    lastReadTime = currentTime - INTERVAL_MS;
    setLEDColor(0xFFFF00);
    delay(100);
    setLEDColor(0x000000);
  }
  
  delay(10);
}
