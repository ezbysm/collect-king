// js/data.js의 작품 목록으로 works.md를 다시 만든다.  사용법: node tools/works.js
'use strict';
const fs = require('fs');
const path = require('path');
const vm = require('vm');

const ROOT = path.join(__dirname, '..');
const ctx = {};
vm.createContext(ctx);
vm.runInContext(fs.readFileSync(path.join(ROOT, 'js', 'data.js'), 'utf8') + ';this.W = WORKS; this.T = TYPES; this.G = GENRES;', ctx);
const { W, T, G } = ctx;

const ORDER = ['anime', 'manga', 'novel', 'movie', 'music', 'game'];
const yr = w => String(w.appear).slice(0, 4);
const size = mb => (mb >= 1000 ? (mb / 1000).toFixed(1) + 'GB' : mb + 'MB');
const ym = a => String(a).slice(0, 4) + '.' + String(a).slice(4);

let md = '# 수집왕 — 등장 작품 목록\n\n';
md += '> `js/data.js`에서 자동 생성 (`node tools/works.js`). 등장 시기는 게임 안에서 그 달부터 구할 수 있다는 뜻입니다.\n';
md += '> 등장 시기는 `tools/schedule.js`로 36개월에 고르게 배치했습니다. 실제 시기보다 앞당기지 않고, 전체 용량 1GB 이상인 작품은 2001년 3월 이후에 나옵니다.\n\n';

md += '## 요약\n\n| 종류 | 단위 · 용량 | 2000 | 2001 | 2002 | 합계 |\n|---|---|---|---|---|---|\n';
const tot = { 2000: 0, 2001: 0, 2002: 0 };
for (const t of ORDER) {
  const ws = W.filter(w => w.type === t);
  const n = y => ws.filter(w => yr(w) === y).length;
  for (const y of ['2000', '2001', '2002']) tot[y] += n(y);
  md += `| ${T[t].name} (${T[t].ext}) | ${T[t].unit}당 ${T[t].size}MB | ${n('2000')} | ${n('2001')} | ${n('2002')} | ${ws.length} |\n`;
}
md += `| **합계** | | ${tot[2000]} | ${tot[2001]} | ${tot[2002]} | **${W.length}** |\n\n`;

md += '## 달별 등장\n\n| 달 | 수 | 작품 (★ 전체 1GB 이상) |\n|---|---|---|\n';
const months = [...new Set(W.map(w => w.appear))].sort();
for (const m of months) {
  const ws = W.filter(w => w.appear === m);
  md += `| ${ym(m)} | ${ws.length} | ${ws.map(w => `${w.title}(${T[w.type].name})${w.units * w.size >= 1000 ? '★' : ''}`).join(', ')} |\n`;
}
md += '\n';

for (const t of ORDER) {
  const ws = W.filter(w => w.type === t).sort((a, b) => a.appear - b.appear || a.title.localeCompare(b.title));
  md += `## ${T[t].name} (${T[t].ext}, ${T[t].unit}당 ${T[t].size}MB) — ${ws.length}개\n\n| 등장 | 작품 | 장르 | 분량 | 전체 용량 |\n|---|---|---|---|---|\n`;
  for (const w of ws) md += `| ${ym(w.appear)} | ${w.title} | ${w.genres.map(g => G[g].name).join(', ')} | ${w.units}${T[t].unit} | ${size(w.units * w.size)} |\n`;
  md += '\n';
}

const M = { video: ['anime', 'movie'], paper: ['manga', 'novel'], game: ['game'], music: ['music'] };
md += '## 장르 × 매체별 작품 수\n\n(작품 하나가 장르 두 개를 가지면 양쪽에 모두 셉니다)\n\n| 장르 | 영상 | 책 | 게임 | 음악 | 합계 |\n|---|---|---|---|---|---|\n';
for (const g in G) {
  const n = Object.values(M).map(ts => W.filter(w => ts.includes(w.type) && w.genres.includes(g)).length);
  md += `| ${G[g].name} | ${n.join(' | ')} | ${W.filter(w => w.genres.includes(g)).length} |\n`;
}
fs.writeFileSync(path.join(ROOT, 'works.md'), md);
console.log('works.md를 만들었습니다.');
