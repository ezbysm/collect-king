// 롬 헤더 체크섬(0xBD)을 채우고 크기를 2의 거듭제곱으로 맞춘다 (플래시 카트리지·에뮬레이터 호환).
// 사용법: node gba/tools/fix-header.js build/collect-king.gba
'use strict';
const fs = require('fs');
const file = process.argv[2];
let rom = fs.readFileSync(file);
let sum = 0;
for (let i = 0xa0; i <= 0xbc; i++) sum += rom[i];
rom[0xbd] = (-(sum + 0x19)) & 0xff;
let size = 1 << 18;
while (size < rom.length) size <<= 1;
const pad = size - rom.length;
if (pad) rom = Buffer.concat([rom, Buffer.alloc(pad, 0xff)]);
fs.writeFileSync(file, rom);
console.log(`${file}: 헤더 체크섬 0x${rom[0xbd].toString(16)}, ${(rom.length / 1024).toFixed(0)}KB`);
