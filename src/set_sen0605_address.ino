#include <Arduino.h>

// ----------------------------------------------------
// 設定項目
// ----------------------------------------------------
#define RX_PIN 20  // M5Stamp C3U の RXピン
#define TX_PIN 21  // M5Stamp C3U の TXピン

// 変更前の古いアドレス（不明な場合は 0x00 を指定すると強制変更できますが、1台接続が絶対条件です）
const uint8_t OLD_ADDRESS = 0x01; 

// 変更後の新しいアドレス（例として 0x02 に変更）
const uint8_t NEW_ADDRESS = 0x02; 

// ----------------------------------------------------
// センサー通信用シリアル設定
// ----------------------------------------------------
HardwareSerial SensorSerial(1); // 任意のシリアルチャンネル

// CRC16 (Modbus RTU) 計算関数
uint16_t calculateCRC(uint8_t *buf, int len) {
  uint16_t crc = 0xFFFF;
  for (int pos = 0; pos < len; pos++) {
    crc ^= (uint16_t)buf[pos];
    for (int i = 8; i != 0; i--) {
      if ((crc & 0x0001) != 0) {
        crc >>= 1;
        crc ^= 0xA001;
      } else {
        crc >>= 1;
      }
    }
  }
  return crc;
}

void setup() {
  // PCモニター用シリアル
  Serial.begin(115200);
  delay(2000);
  Serial.println("\n--- SEN0605 アドレス設定プログラム ---");

  // センサー通信用シリアル初期化 (SEN0605のデフォルト：9600bps, 8N1)
  SensorSerial.begin(9600, SERIAL_8N1, RX_PIN, TX_PIN);
  
  // Modbus アドレス書き込みコマンドフレームの生成
  // SEN0605のアドレス保持レジスタ（通常 0x0100 または 0x0000、仕様に準拠）
  // 多くのRS485センサーは 0x0000 への単一レジスタ書き込み(0x06)でアドレスを保持します
  uint8_t cmd[8] = {
    OLD_ADDRESS, // 現在のアドレス (または 0x00)
    0x06,        // ファンクションコード: Write Single Register
    0x00, 0x00,  // レジスタアドレス (アドレス変更用のレジスタ：通常0x0000)
    0x00, NEW_ADDRESS, // 書き込む新しいアドレス値
    0x00, 0x00   // CRC (後で計算)
  };

  // CRCの計算とセット
  uint16_t crc = calculateCRC(cmd, 6);
  cmd[6] = crc & 0xFF;        // 低位バイト
  cmd[7] = (crc >> 8) & 0xFF; // 高位バイト

  // コマンドの送信
  Serial.printf("アドレスを変更中: 0x%02X -> 0x%02X ...\n", OLD_ADDRESS, NEW_ADDRESS);
  SensorSerial.write(cmd, 8);
  SensorSerial.flush();

  // レスポンスの待機と受信
  delay(500); 
  if (SensorSerial.available()) {
    Serial.print("センサーからの応答: ");
    while (SensorSerial.available()) {
      uint8_t res = SensorSerial.read();
      Serial.printf("%02X ", res);
    }
    Serial.println("\n設定コマンドの送信が完了しました。");
    Serial.println("センサーの電源を再投入して、新しいアドレスで通信できるか確認してください。");
  } else {
    Serial.println("エラー: センサーからの応答がありません。配線や電源、ピン番号を確認してください。");
  }
}

void loop() {
  // 処理は一度だけ実行するため、ループ内は空です
}
