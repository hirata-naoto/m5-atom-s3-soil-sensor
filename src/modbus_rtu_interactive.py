import machine
import time
import struct

class ModbusRTU:
    def __init__(self, uart: machine.UART, de_pin: machine.Pin = None):
        """
        uart: 初期化済みのUARTオブジェクト
        de_pin: DE/REを制御する machine.Pin オブジェクト (省略時はRS-232C等の標準UARTとして動作)
        """
        self.uart = uart
        self.de_pin = de_pin
        
        # DEピンがある場合は初期状態を受信モード(Low)にする
        if self.de_pin:
            self.de_pin.init(mode=machine.Pin.OUT, value=0)

    def _crc16(self, data: bytes) -> int:
        """内部用: CRC16計算"""
        crc = 0xFFFF
        for pos in data:
            crc ^= pos
            for _ in range(8):
                if (crc & 1) != 0:
                    crc >>= 1
                    crc ^= 0xA001
                else:
                    crc >>= 1
        return crc

    def _send_request(self, request: bytes):
        """内部用: DEピン制御を含めたデータ送信"""
        # 受信バッファのクリア
        while self.uart.any():
            self.uart.read()

        if self.de_pin:
            self.de_pin.value(1)  # 送信モード
            
        self.uart.write(request)
        
        # 送信完了待ち (txdoneが未実装の場合は一瞬待つ)
        if hasattr(self.uart, "txdone"):
            while not self.uart.txdone():
                pass
        else:
            # 9600bps基準のおおよその安全マージン待ち (1バイト≒1.1ms)
            time.sleep_ms(int((len(request) * 11 / 9600) * 1000) + 2)

        if self.de_pin:
            self.de_pin.value(0)  # 受信モードに戻す

    def execute(self, slave: int, fc: int, address: int, value_or_len: int, timeout_ms=500):
        """
        Modbusコマンドを実行し、結果を対話型（インタラクティブ）に表示します。
        slave: スレーブアドレス (1-247)
        fc: ファンクションコード (3 または 6)
        address: 開始アドレス / レジスタアドレス
        value_or_len: FC03なら「長さ」、FC06なら「書き込み値(u16)」
        """
        if fc not in [3, 6]:
            print("エラー: ファンクションコードは 3 または 6 のみ対応しています。")
            return

        # PDUとCRCの組み立て
        pdu = struct.pack(">BBHH", slave, fc, address, value_or_len)
        crc = self._crc16(pdu)
        request = pdu + struct.pack("<H", crc)

        # 送信
        self._send_request(request)
        
        # --- レスポンス待ち ---
        start_time = time.ticks_ms()
        while not self.uart.any():
            if time.ticks_diff(time.ticks_ms(), start_time) > timeout_ms:
                print("エラー: タイムアウト（応答なし）")
                return
            time.sleep_ms(10)
            
        time.sleep_ms(50)  # データが揃うのを少し待つ
        response = self.uart.read()
        
        if not response or len(response) < 5:
            print(f"エラー: 不正なレスポンス長 ({len(response) if response else 0} bytes)")
            return

        # CRCチェック
        resp_crc = self._crc16(response[:-2])
        expected_crc, = struct.unpack("<H", response[-2:])
        if resp_crc != expected_crc:
            print("エラー: CRCエラー（データ破損の可能性）")
            return

        # 例外レスポンスチェック
        if response[1] & 0x80:
            print(f"Modbus例外エラーレスポンスを受信: 例外コード = 0x{response[2]:02X}")
            return

        # --- 結果の表示 ---
        print("\n--- Modbus 実行結果 ---")
        if fc == 3:
            byte_count = response[2]
            reg_count = byte_count // 2
            print(f"[FC03] スレーブ {slave} から {reg_count} 個のレジスタを読み出しました:")
            
            for i in range(reg_count):
                idx = 3 + (i * 2)
                val = struct.unpack(">H", response[idx:idx+2])[0]
                reg_addr = address + i
                print(f"  レジスタ 0x{reg_addr:04X} ({reg_addr:5d}) : 0x{val:04X} / {val:5d}")
                
        elif fc == 6:
            resp_addr, resp_val = struct.unpack(">HH", response[2:6])
            print(f"[FC06] スレーブ {slave} のレジスタ書き込み成功:")
            print(f"  レジスタ 0x{resp_addr:04X} ({resp_addr:5d}) <- 0x{resp_val:04X} / {resp_val:5d}")
        print("-----------------------\n")
