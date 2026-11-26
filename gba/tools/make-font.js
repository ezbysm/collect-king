// Galmuri11 BDF 글꼴에서 게임에 쓰이는 글자만 뽑아 GBA용 1비트 글꼴 C 파일을 만든다.
//
// 글자 목록: 웹판 게임 코드(js/*.js)와 GBA 소스(gba/src/*.c)에 나오는 모든 글자 + ASCII
// 사용법: node gba/tools/make-font.js [BDF 경로]
//   BDF가 없으면: (cd gba/tools && npm pack galmuri@2.40.3 && tar xzf galmuri-2.40.3.tgz package/dist/Galmuri11.bdf --strip-components=2)
'use strict';
const fs = require('fs');
const path = require('path');

const ROOT = path.join(__dirname, '..', '..');
const BDF = process.argv[2] || path.join(__dirname, 'Galmuri11.bdf');
const OUT = path.join(__dirname, '..', 'src', 'font_data.c');

// 1) 필요한 글자 모으기
const chars = new Set();
for (let c = 0x20; c < 0x7f; c++) chars.add(c);
for (const ch of '…·▼▶◀★♥☾①②③「」『』~—–“”‘’') chars.add(ch.codePointAt(0));
// 이름 입력 키보드로 어떤 글자든 만들 수 있도록 한글 음절 전체와 자모를 넣는다
for (let cp = 0xac00; cp <= 0xd7a3; cp++) chars.add(cp);
for (let cp = 0x3131; cp <= 0x3163; cp++) chars.add(cp);
for (const ch of '₩→←↑↓○●□■◆♪☑☐') chars.add(ch.codePointAt(0));
const sources = [
  ...fs.readdirSync(path.join(ROOT, 'js')).filter(f => f.endsWith('.js')).map(f => path.join(ROOT, 'js', f)),
  ...fs.readdirSync(path.join(ROOT, 'gba', 'src')).filter(f => f.endsWith('.c') && f !== 'font_data.c').map(f => path.join(ROOT, 'gba', 'src', f)),
];
for (const f of sources) for (const ch of fs.readFileSync(f, 'utf8')) {
  const cp = ch.codePointAt(0);
  if (cp >= 0xac00 && cp <= 0xd7a3) chars.add(cp);       // 한글 음절
  else if (cp >= 0x3131 && cp <= 0x318e) chars.add(cp);  // 한글 자모
}

// 2) BDF 읽기
const lines = fs.readFileSync(BDF, 'utf8').split('\n');
let ascent = 14;
const glyphs = new Map();
for (let i = 0; i < lines.length; i++) {
  const l = lines[i];
  if (l.startsWith('FONT_ASCENT')) ascent = +l.split(' ')[1];
  if (!l.startsWith('STARTCHAR')) continue;
  let enc = -1, dw = 0, bbx = [0, 0, 0, 0];
  const rows = [];
  for (i++; i < lines.length && !lines[i].startsWith('ENDCHAR'); i++) {
    const t = lines[i];
    if (t.startsWith('ENCODING')) enc = +t.split(' ')[1];
    else if (t.startsWith('DWIDTH')) dw = +t.split(' ')[1];
    else if (t.startsWith('BBX')) bbx = t.split(' ').slice(1).map(Number);
    else if (t === 'BITMAP') {
      for (i++; !lines[i].startsWith('ENDCHAR'); i++) rows.push(parseInt(lines[i], 16) || 0);
      i--;
    }
  }
  if (chars.has(enc)) glyphs.set(enc, { dw, bbx, rows });
}

// 3) 공통 칸 높이 정하기 (모든 글자의 위·아래 끝)
let top = 99, bottom = -99;
for (const g of glyphs.values()) {
  const [w, h, , yoff] = g.bbx;
  if (!h) continue;
  top = Math.min(top, ascent - (h + yoff));
  bottom = Math.max(bottom, ascent - yoff);
}
const CELL_H = bottom - top;

// 4) 글자마다 16비트 x CELL_H줄로 저장 (왼쪽 비트가 맨 왼쪽 픽셀)
const list = [...glyphs.entries()].sort((a, b) => a[0] - b[0]);
const codes = [], widths = [], bits = [];
for (const [cp, g] of list) {
  const [w, h, xoff, yoff] = g.bbx;
  const rowBytes = Math.ceil(w / 8);
  const cell = new Array(CELL_H).fill(0);
  for (let r = 0; r < h; r++) {
    const y = ascent - (h + yoff) + r - top;
    let v = g.rows[r] || 0;
    // BDF 줄은 rowBytes*8비트 왼쪽 정렬 → 16비트 왼쪽 정렬 + xoff
    v = (v << (16 - rowBytes * 8)) >>> Math.max(0, xoff);
    if (y >= 0 && y < CELL_H) cell[y] = v & 0xffff;
  }
  codes.push(cp); widths.push(g.dw); bits.push(...cell);
}

const missing = [...chars].filter(c => !glyphs.has(c));
let c = `// 자동 생성: gba/tools/make-font.js — Galmuri11 (SIL OFL 1.1, gba/tools/Galmuri-LICENSE.txt)\n`;
c += `// 글자 ${codes.length}개, 칸 높이 ${CELL_H}px\n#include "font.h"\n\n`;
c += `const int FONT_COUNT = ${codes.length};\nconst int FONT_CELL_H = ${CELL_H};\n\n`;
c += `const unsigned short FONT_CODES[${codes.length}] = {\n${chunk(codes.map(v => '0x' + v.toString(16)), 16)}\n};\n\n`;
c += `const unsigned char FONT_WIDTHS[${codes.length}] = {\n${chunk(widths, 32)}\n};\n\n`;
c += `const unsigned short FONT_BITS[${bits.length}] = {\n${chunk(bits.map(v => '0x' + v.toString(16)), 16)}\n};\n`;
fs.writeFileSync(OUT, c);
console.log(`글자 ${codes.length}개 (칸 ${CELL_H}px, 약 ${Math.round((codes.length * (3 + CELL_H * 2)) / 1024)}KB) → gba/src/font_data.c`);
if (missing.length) console.log('글꼴에 없는 글자:', missing.map(m => String.fromCodePoint(m)).join(''));

function chunk(arr, n) {
  const out = [];
  for (let i = 0; i < arr.length; i += n) out.push('  ' + arr.slice(i, i + n).join(', ') + ',');
  return out.join('\n');
}
