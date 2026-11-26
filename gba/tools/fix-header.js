// 롬 헤더 체크섬(0xBD)을 채우고 크기를 4바이트 단위로 맞춘다.
// 사용법: node gba/tools/fix-header.js build/collect-king.gba
'use strict';
const fs = require('fs');
const file = process.argv[2];
let rom = fs.readFileSync(file);
let sum = 0;
for (let i = 0xa0; i <= 0xbc; i++) sum += rom[i];
rom[0xbd] = (-(sum + 0x19)) & 0xff;
const pad = (4 - (rom.length % 4)) % 4;
if (pad) rom = Buffer.concat([rom, Buffer.alloc(pad, 0xff)]);
fs.writeFileSync(file, rom);
console.log(`${file}: 헤더 체크섬 0x${rom[0xbd].toString(16)}, ${(rom.length / 1024).toFixed(0)}KB`);
