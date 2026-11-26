# 플래시 카트리지·DS 메뉴에 보이는 아이콘(32x32, 16색)과 제목을 담은 배너를 만든다.
# 웹판 홈 화면 아이콘(assets/icons, 36x36 도트를 키운 것)을 줄여 쓴다.
# 사용법: python3 nds/tools/make-banner.py assets/icons/icon-36.png nds/build/banner.bin
import struct, sys
from PIL import Image

src, out = sys.argv[1], sys.argv[2]
im = Image.open(src).convert('RGB')
im = im.resize((36, 36), Image.NEAREST).crop((2, 2, 34, 34))
q = im.quantize(colors=15, method=Image.Quantize.MEDIANCUT, dither=Image.Dither.NONE)
pal = q.getpalette()[:45]
idx = [p + 1 for p in q.tobytes()]           # 0번은 투명 → 1번부터 쓴다

def crc16(b):
    crc = 0xFFFF
    for x in b:
        crc ^= x
        for _ in range(8):
            crc = (crc >> 1) ^ 0xA001 if crc & 1 else crc >> 1
    return crc

bitmap = bytearray()
for ty in range(4):
    for tx in range(4):
        for y in range(8):
            for x in range(0, 8, 2):
                a = idx[(ty * 8 + y) * 32 + tx * 8 + x]
                b = idx[(ty * 8 + y) * 32 + tx * 8 + x + 1]
                bitmap.append(a | (b << 4))
palette = bytearray(struct.pack('<H', 0))
for i in range(15):
    r, g, b = pal[i * 3:i * 3 + 3]
    palette += struct.pack('<H', (r >> 3) | ((g >> 3) << 5) | ((b >> 3) << 10))
title = 'Collect King\n2000-2003\nhomebrew'
titles = bytearray()
for _ in range(6):
    t = title.encode('utf-16-le')
    titles += t + bytes(256 - len(t))
body = bitmap + palette + titles                # 0x20 ~ 0x83F
banner = struct.pack('<HH', 1, crc16(body)) + bytes(0x1C) + body
assert len(banner) == 0x840
open(out, 'wb').write(banner)
print(out, len(banner), 'bytes')
