import machine
import time
import struct

class ModbusRTU:
    def __init__(self, uart_id: int, baudrate: int = 9600,
                 tx: int = None, rx: int = None, de_pin: int = None):
        
        self.baudrate = baudrate  # 送信待ち計算に使える
        self.uart = machine.UART(uart_id, baudrate=baudrate,
                                 tx=machine.Pin(tx), rx=machine.Pin(rx),
                                 timeout=10)

        # DEピンがある場合は初期状態を受信モード(Low)にする
        self.de_pin = machine.Pin(de_pin, machine.Pin.OUT, value=0) if de_pin is not None else None

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
            time.sleep_ms(int((len(request) * 11 / self.baudrate) * 1000) + 2)

        if self.de_pin:
            self.de_pin.value(0)  # 受信モードに戻す

    def _probe(self, slave: int, timeout_ms: int) -> bool:
        """
        内部用: 指定スレーブに FC03 で 1 レジスタを読み出し、
        有効な応答が返ってくるか確認する。
        返り値: 応答あり(正常 or Modbus例外) → True / タイムアウト・CRCエラー → False
        """
        pdu = struct.pack(">BBHH", slave, 3, 0x0000, 1)
        crc = self._crc16(pdu)
        request = pdu + struct.pack("<H", crc)

        self._send_request(request)

        # 応答待ち
        start_time = time.ticks_ms()
        while not self.uart.any():
            if time.ticks_diff(time.ticks_ms(), start_time) > timeout_ms:
                return False
            time.sleep_ms(5)

        time.sleep_ms(30)  # データが揃うのを少し待つ
        response = self.uart.read()

        if not response or len(response) < 5:
            return False

        # CRC が一致しない場合はノイズとみなす
        resp_crc = self._crc16(response[:-2])
        expected_crc, = struct.unpack("<H", response[-2:])
        if resp_crc != expected_crc:
            return False

        # 正常応答 or Modbus例外応答(0x80 フラグ)どちらも「存在する」と判定
        return True

    def scan(self, start: int = 1, end: int = 247, timeout_ms: int = 100) -> list:
        """
        スレーブアドレスをスキャンし、応答したスレーブの一覧を返します。

        Parameters
        ----------
        start      : スキャン開始アドレス (デフォルト: 1)
        end        : スキャン終了アドレス (デフォルト: 247)
        timeout_ms : 1スレーブあたりの応答待ちタイムアウト [ms] (デフォルト: 100)

        Returns
        -------
        list : 応答があったスレーブアドレスのリスト
        """
        start = max(1, min(start, 247))
        end   = max(1, min(end,   247))
        if start > end:
            start, end = end, start

        found = []
        total = end - start + 1

        print(f"\n=== Modbus RTU スレーブスキャン ===")
        print(f"範囲: {start} 〜 {end}  (合計 {total} アドレス)")
        print(f"タイムアウト: {timeout_ms} ms / アドレス")
        print("-----------------------------------")

        for addr in range(start, end + 1):
            # 進捗表示 (キャリッジリターンで上書き)
            print(f"  スキャン中... [{addr - start + 1}/{total}] アドレス {addr:3d}", end="\r")

            if self._probe(addr, timeout_ms):
                found.append(addr)
                # 見つかった場合は進捗行を確定して通知
                print(f"  [発見] スレーブアドレス {addr:3d} (0x{addr:02X}) が応答しました。")

        # サマリー
        print("\n-----------------------------------")
        if found:
            print(f"スキャン完了: {len(found)} 台のスレーブを検出しました。")
            print(f"  検出アドレス: {found}")
        else:
            print("スキャン完了: 応答したスレーブが見つかりませんでした。")
        print("===================================\n")

        return found
        
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

"""
# 1. ハードウェアの準備
modbus = ModbusRTU(uart_id=1, baudrate=9600, tx=5, rx=6, de_pin=7)


# ---- スキャン ----
# 全アドレスをスキャン (1〜247)
found = modbus.scan()

# 範囲を絞ってスキャン (例: 1〜10 のみ)
found = modbus.scan(start=1, end=10)

# タイムアウトを長めにしてスキャン (低速デバイス向け)
found = modbus.scan(start=1, end=247, timeout_ms=300)


# ---- あとはスッキリ対話型実行！ ----
# レジスタ読み出し (FC03)
modbus.execute(slave=1, fc=3, address=0x0010, value_or_len=3)

# レジスタ書き込み (FC06)
modbus.execute(slave=1, fc=6, address=0x0012, value_or_len=555)
"""
