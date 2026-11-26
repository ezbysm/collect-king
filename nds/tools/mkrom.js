// ARM9·ARM7 바이너리(와 배너)를 묶어 .nds 롬을 만든다.
// 사용법: node tools/mkrom.js out.nds arm9.bin arm7.bin [banner.bin]
'use strict';
const fs = require('fs');
const [out, a9p, a7p, bannerp] = process.argv.slice(2);
const a9 = fs.readFileSync(a9p), a7 = fs.readFileSync(a7p);
const banner = bannerp && fs.existsSync(bannerp) ? fs.readFileSync(bannerp) : null;

function crc16(buf, start, end) {   // CRC-16 (MODBUS: 다항식 0xA001, 초기값 0xFFFF)
  let crc = 0xFFFF;
  for (let i = start; i < end; i++) {
    crc ^= buf[i];
    for (let b = 0; b < 8; b++) crc = crc & 1 ? (crc >>> 1) ^ 0xA001 : crc >>> 1;
  }
  return crc;
}
const align = (n, a) => (n + a - 1) & ~(a - 1);

// 배치: 헤더(0x200) | ARM9 | ARM7 | 파일 이름 표(FNT) | 배너
let pos = 0x200;
const a9off = pos; pos = align(pos + a9.length, 0x200);
const a7off = pos; pos = align(pos + a7.length, 0x200);
const fntoff = pos; const fnt = Buffer.from([8, 0, 0, 0, 0, 0, 1, 0, 0]); pos = align(pos + fnt.length, 0x200);
const fatoff = pos;
let banoff = 0;
if (banner) { banoff = pos; pos = align(pos + banner.length, 0x200); }
const used = pos;
let cap = 0; while ((0x20000 << cap) < used) cap++;
const rom = Buffer.alloc(align(used, 4), 0);

const h = rom;
h.write('COLLECTKING', 0, 'ascii');   // 제목 (12바이트)
h.write('####', 0x0C, 'ascii');        // 게임 코드: 홈브루 관례
h.write('00', 0x10, 'ascii');          // 제작사
h[0x12] = 0;                           // NDS 전용
h[0x14] = cap;                         // 롬 크기 = 128KB << cap
h.writeUInt32LE(a9off, 0x20); h.writeUInt32LE(0x02000000, 0x24); h.writeUInt32LE(0x02000000, 0x28); h.writeUInt32LE(a9.length, 0x2C);
h.writeUInt32LE(a7off, 0x30); h.writeUInt32LE(0x03800000, 0x34); h.writeUInt32LE(0x03800000, 0x38); h.writeUInt32LE(a7.length, 0x3C);
h.writeUInt32LE(fntoff, 0x40); h.writeUInt32LE(fnt.length, 0x44);
h.writeUInt32LE(fatoff, 0x48); h.writeUInt32LE(0, 0x4C);
h.writeUInt32LE(0x00586000, 0x60);     // 카드 읽기 설정 (보통 값)
h.writeUInt32LE(0x001808F8, 0x64);
h.writeUInt32LE(banoff, 0x68);
h.writeUInt16LE(0x051E, 0x6E);         // 보안 영역 지연
h.writeUInt32LE(used, 0x80);           // 쓰는 롬 크기
h.writeUInt32LE(0x4000, 0x84);         // 헤더 크기
// 0xC0~0x15B 닌텐도 로고 자리는 비워 둔다 (플래시 카트리지·에뮬레이터는 확인하지 않음)
h.writeUInt16LE(0xCF56, 0x15C);        // 로고 CRC 값 자리
a9.copy(rom, a9off); a7.copy(rom, a7off); fnt.copy(rom, fntoff);
if (banner) banner.copy(rom, banoff);
h.writeUInt16LE(crc16(rom, 0, 0x15E), 0x15E);
fs.writeFileSync(out, rom);
console.log(`${out}: ARM9 ${(a9.length / 1024).toFixed(0)}KB, ARM7 ${(a7.length / 1024).toFixed(1)}KB, 전체 ${(rom.length / 1024).toFixed(0)}KB`);
