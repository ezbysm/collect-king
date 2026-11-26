// 웹판 데이터(js/data.js, js/story.js의 엔딩 문장)를 GBA용 C 표로 바꾼다.
// 사용법: node gba/tools/make-data.js  → gba/src/data_gen.c, gba/src/data_gen.h
'use strict';
const fs = require('fs');
const path = require('path');
const vm = require('vm');

const ROOT = path.join(__dirname, '..', '..');
const ctx = {
  console, Math,
  localStorage: { getItem: () => null, setItem() {}, removeItem() {} },
  UI: {}, Sound: {}, Art: {}, document: {}, window: {},
};
vm.createContext(ctx);
for (const f of ['data.js', 'game.js', 'story.js']) vm.runInContext(fs.readFileSync(path.join(ROOT, 'js', f), 'utf8'), ctx, { filename: f });
const D = vm.runInContext('({ WORKS, TYPES, GENRES, HEROINE_SURNAMES, HEROINE_GIVEN, HEROINE_TASTES, ENDINGS: Story.ENDINGS, ROMANCE: Story.ROMANCE, ENDING_TABLE })', ctx);

const q = s => JSON.stringify(s);   // C 문자열 (UTF-8 그대로)
const GKEYS = Object.keys(D.GENRES);
const TKEYS = Object.keys(D.TYPES);
const turnOf = ym => (Math.floor(ym / 100) - 2000) * 12 + (ym % 100) - 3;
const rgb555 = hex => { const n = parseInt(hex.slice(1), 16); return ((n >> 19) & 31) | (((n >> 11) & 31) << 5) | (((n >> 3) & 31) << 10); };

let h = `// 자동 생성: gba/tools/make-data.js — 웹판 데이터에서 만든 표\n#pragma once\n\n`;
h += `#define NWORKS ${D.WORKS.length}\n#define NTYPES ${TKEYS.length}\n#define NGENRES ${GKEYS.length}\n#define NENDINGS ${Object.keys(D.ENDINGS).length}\n\n`;
h += `enum { ${TKEYS.map(k => 'T_' + k.toUpperCase()).join(', ')} };\n`;
h += `enum { ${GKEYS.map(k => 'G_' + k.toUpperCase()).join(', ')} };\n`;
h += `enum { ${Object.keys(D.ENDINGS).map(k => 'E_' + k.toUpperCase()).join(', ')} };\n\n`;
h += `typedef struct { const char *title; unsigned char type, g0, g1, appear, units; } Work;   // g1=255: 장르 하나\n`;
h += `typedef struct { const char *name, *ext, *unit, *folder; unsigned short size, weight10; } TypeInfo;   // weight10: 성향 가중치 x10\n`;
h += `typedef struct { const char *name, *trait; unsigned short color; } GenreInfo;\n`;
h += `typedef struct { const char *title; unsigned char nlines; const char *lines[6]; } EndingInfo;\n`;
h += `typedef struct { const char *name, *hint_item, *hint_line; } TasteInfo;\n\n`;
h += `extern const Work WORKS[NWORKS];\nextern const TypeInfo TYPES[NTYPES];\nextern const GenreInfo GENRES[NGENRES];\nextern const EndingInfo ENDINGS[NENDINGS];\n`;
h += `extern const unsigned char ENDING_TABLE[NGENRES][4];   // [장르][매체: 0게임 1영상 2책 3음악]\n`;
h += `extern const char *const HER_SURNAMES[]; extern const int N_HER_SURNAMES;\nextern const char *const HER_GIVEN[]; extern const int N_HER_GIVEN;\nextern const TasteInfo TASTES[3];   // 0 순정 만화, 1 판타지 소설, 2 음악\n`;

let c = `// 자동 생성: gba/tools/make-data.js\n#include "data_gen.h"\n\n`;
c += `const Work WORKS[NWORKS] = {\n`;
for (const w of D.WORKS) {
  const g0 = GKEYS.indexOf(w.genres[0]), g1 = w.genres[1] ? GKEYS.indexOf(w.genres[1]) : 255;
  c += `  { ${q(w.title)}, T_${w.type.toUpperCase()}, ${g0}, ${g1}, ${turnOf(w.appear)}, ${w.units} },\n`;
}
c += `};\n\nconst TypeInfo TYPES[NTYPES] = {\n`;
for (const k of TKEYS) { const t = D.TYPES[k]; c += `  { ${q(t.name)}, ${q(t.ext)}, ${q(t.unit)}, ${q(t.folder)}, ${t.size}, ${Math.round(t.weight * 10)} },\n`; }
c += `};\n\nconst GenreInfo GENRES[NGENRES] = {\n`;
for (const k of GKEYS) { const g = D.GENRES[k]; c += `  { ${q(g.name)}, ${q(g.trait)}, 0x${rgb555(g.color).toString(16)} },\n`; }
c += `};\n\nconst EndingInfo ENDINGS[NENDINGS] = {\n`;
for (const k of Object.keys(D.ENDINGS)) { const e = D.ENDINGS[k]; c += `  { ${q(e.title)}, ${e.lines.length}, { ${e.lines.map(q).join(', ')} } },\n`; }
c += `};\n\nconst unsigned char ENDING_TABLE[NGENRES][4] = {\n`;
const MORDER = ['game', 'video', 'paper', 'music'];
for (const g of GKEYS) c += `  { ${MORDER.map(m => 'E_' + D.ENDING_TABLE[g][m].toUpperCase()).join(', ')} },\n`;
c += `};\n\n`;
c += `const char *const HER_SURNAMES[] = { ${D.HEROINE_SURNAMES.map(q).join(', ')} };\nconst int N_HER_SURNAMES = ${D.HEROINE_SURNAMES.length};\n`;
c += `const char *const HER_GIVEN[] = { ${D.HEROINE_GIVEN.map(q).join(', ')} };\nconst int N_HER_GIVEN = ${D.HEROINE_GIVEN.length};\n`;
c += `const TasteInfo TASTES[3] = {\n`;
for (const k of ['love', 'fan', 'music']) { const t = D.HEROINE_TASTES[k]; c += `  { ${q(t.name)}, ${q(t.hintItem)}, ${q(t.hintLine)} },\n`; }
c += `};\n`;

fs.writeFileSync(path.join(__dirname, '..', 'src', 'data_gen.h'), h);
fs.writeFileSync(path.join(__dirname, '..', 'src', 'data_gen.c'), c);
console.log(`작품 ${D.WORKS.length}개, 엔딩 ${Object.keys(D.ENDINGS).length}종 → gba/src/data_gen.c`);
