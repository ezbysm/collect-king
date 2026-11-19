// 작품 등장 시기를 36개월에 고르게 다시 배치해서 js/data.js의 등장(YYYYMM) 값을 고쳐 쓴다.
//
// 규칙
//  - 실제 발매·유행 시기보다 앞당기지 않는다 (지금 data.js의 등장 값이 하한).
//  - 전체 용량 1GB 이상(큰 작품)은 게임 중반(2001년 3월, 13번째 달) 이후에 등장.
//  - 300MB~1GB(중간)는 2000년 6월 이후, 300MB 미만(작은 작품)은 처음부터 가능.
//  - 첫 달 6개, 이후 2002년 11월까지 매달 비슷한 수가 되도록 채운다. 같은 달에는 종류가 겹치지 않게 섞는다.
//
// 사용법: node tools/schedule.js         (미리 보기)
//        node tools/schedule.js --write (data.js에 반영)
'use strict';
const fs = require('fs');
const path = require('path');
const vm = require('vm');

const DATA = path.join(__dirname, '..', 'js', 'data.js');
const src = fs.readFileSync(DATA, 'utf8');
const ctx = {};
vm.createContext(ctx);
vm.runInContext(src + ';this.W = WORKS; this.T = TYPES;', ctx);
const { W, T } = ctx;

const LAST_TURN = 32;          // 2002년 11월
const FIRST_MONTH = 6;         // 첫 달 작품 수
const LARGE_MB = 1000, MID_MB = 300;
const turnOf = ym => (Math.floor(ym / 100) - 2000) * 12 + (ym % 100) - 3;
const ymOf = t => { const m = (t + 2) % 12 + 1; const y = 2000 + Math.floor((t + 2) / 12); return y * 100 + m; };

const works = W.map(w => {
  const mb = w.units * w.size;
  const tier = mb >= LARGE_MB ? 12 : mb >= MID_MB ? 3 : 0;
  return { w, mb, earliest: Math.min(LAST_TURN, Math.max(turnOf(w.appear), tier)) };
});

// 달마다 몇 개를 낼지: 첫 달 FIRST_MONTH, 나머지는 고르게
const rest = works.length - FIRST_MONTH;
const quota = [FIRST_MONTH];
for (let t = 1; t <= LAST_TURN; t++) quota.push(Math.round(rest * t / LAST_TURN) - Math.round(rest * (t - 1) / LAST_TURN));

const left = new Set(works);
const assigned = new Map();
let carry = 0;
const typeCount = Array.from({ length: LAST_TURN + 1 }, () => ({}));
const hash = str => { let h = 7; for (const ch of str) h = (h * 31 + ch.charCodeAt(0)) >>> 0; return h; };
for (let t = 0; t <= LAST_TURN; t++) {
  let need = quota[t] + carry;
  const typesThisMonth = new Set();
  while (need > 0) {
    const eligible = [...left].filter(x => x.earliest <= t);
    if (!eligible.length) break;
    // 1) 8달 넘게 기다린 것 우선 2) 이번 달·지난달에 적게 나온 종류 우선 3) 고정된 섞기 순서
    const recent = k => (typeCount[t][k] || 0) * 2 + (t > 0 ? typeCount[t - 1][k] || 0 : 0);
    eligible.sort((a, b) => ((t - b.earliest >= 8) - (t - a.earliest >= 8))
      || (recent(a.w.type) - recent(b.w.type))
      || (hash(a.w.id) - hash(b.w.id)));
    // 실제 시기가 이번 달인 작품은 미루지 않는다
    const pick = eligible.find(x => x.earliest === t && turnOf(x.w.appear) === t) || eligible[0];
    left.delete(pick);
    assigned.set(pick.w.id, t);
    typesThisMonth.add(pick.w.type);
    typeCount[t][pick.w.type] = (typeCount[t][pick.w.type] || 0) + 1;
    need--;
  }
  carry = need;
}
for (const x of left) assigned.set(x.w.id, LAST_TURN); // 남은 것은 마지막 달

// 결과
const byTurn = {};
for (const x of works) (byTurn[assigned.get(x.w.id)] = byTurn[assigned.get(x.w.id)] || []).push(x);
for (let t = 0; t <= LAST_TURN; t++) {
  const list = (byTurn[t] || []).map(x => `${x.w.title}(${T[x.w.type].name}${x.mb >= LARGE_MB ? ',큼' : ''})`);
  console.log(`${String(ymOf(t)).replace(/(\d{4})(\d{2})/, '$1.$2')} [${list.length}] ${list.join(', ')}`);
}

if (process.argv.includes('--write')) {
  let out = src;
  for (const x of works) {
    const re = new RegExp(`(\\['${x.w.id}', '[a-z]+', '[^']*', \\[[^\\]]*\\], )(\\d{6})`);
    if (!re.test(out)) throw new Error('찾지 못함: ' + x.w.id);
    out = out.replace(re, `$1${ymOf(assigned.get(x.w.id))}`);
  }
  fs.writeFileSync(DATA, out);
  console.log('\njs/data.js에 반영했습니다.');
}
