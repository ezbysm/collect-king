// 수집왕 헤드리스 시뮬레이터 — 엔딩 밸런스 확인용
// 실제 게임 코드(js/data·game·story·main.js)를 그대로 불러오고, 화면(UI)·소리만 자동 플레이어로 바꿔 끼운다.
//
// 사용법: node tools/sim.js [목표당 판 수=2] [결과 JSON 경로]
//   목표: 다운로드(장르×매체 24가지), 전설, 스캔, 교환, 완결, 무작위 플레이
'use strict';
const fs = require('fs');
const vm = require('vm');
const path = require('path');
const ROOT = path.join(__dirname, '..', 'js') + path.sep;

function makeRng(seed) {
  let v = seed >>> 0 || 1;
  return () => { v = (v * 1664525 + 1013904223) >>> 0; return v / 4294967296; };
}

// ---------- 목표별 효율 플레이어 ----------
// goal: { kind: 'legend' | 'archiver' | 'artisan' | 'sharer' | 'completer', genre?, media? }
function makePolicy(goal, api) {
  const { S, W } = api; // S(): 현재 상태, W: 게임 전역
  const MEDIA = { video: ['anime', 'movie'], paper: ['manga', 'novel'], game: ['game'], music: ['music'] };
  const matchMedia = w => !goal.media || MEDIA[goal.media].includes(w.type);
  const matchGenre = w => !goal.genre || w.genres.includes(goal.genre);

  function scoreWork(w) {
    const s = S();
    const rem = W.remaining(s, w);
    const pts = W.TYPES[w.type].weight;
    let sc = 0;
    if (goal.kind === 'legend') {
      // 장르를 고르게: 지금 가장 약한 장르를 우선, 완결이 쉬운 것 우선
      const tr = W.computeTraits(s);
      const low = Object.entries(tr.genre).sort((a, b) => a[1] - b[1]).map(x => x[0]);
      sc += w.genres.reduce((a, g) => a + (6 - low.indexOf(g)), 0) * 3;
      sc += rem <= 12 ? 25 : 0;
      sc += W.owned(s, w.id) > 0 ? 10 : 0; // 시작한 건 끝내기
    } else if (goal.kind === 'completer') {
      sc += Math.min(w.units * pts, 60) * 0.5; // 큰 시리즈를 골라 끝까지
      if (goal.genre && w.genres.length === 1 && w.genres[0] === goal.genre) sc += 40; // 순수하게 그 장르인 작품
      sc += W.owned(s, w.id) > 0 ? 30 : 0;
      sc += matchGenre(w) ? 20 : 0;
    } else {
      if (matchGenre(w)) sc += w.genres[0] === goal.genre ? 40 : 25;
      else if (goal.genre) sc -= 30;
      if (matchMedia(w)) sc += 35;
      else if (goal.media) sc -= 10;
      sc += W.owned(s, w.id) > 0 ? 5 : 0;
    }
    sc += pts * 0.1;
    return sc;
  }
  const best = (works, n) => works.slice().sort((a, b) => scoreWork(b) - scoreWork(a)).slice(0, n);

  // 한 달 일정 (시험 달 첫 칸은 공부로 고정)
  function plan() {
    const s = S();
    if (goal.kind === 'random') {
      const d0 = W.dateOf(s.turn);
      const keys = ['download', 'download', 'burn', 'scan', 'pc', 'yongsan', 'club', 'friend', 'chores', 'study', 'rest'];
      const out = W.EXAM_MONTHS.includes(d0.m) ? ['study'] : [];
      while (out.length < 3) out.push(keys[Math.floor(Math.random() * keys.length)]);
      return out;
    }
    const d = W.dateOf(s.turn);
    const exam = W.EXAM_MONTHS.includes(d.m);
    const slots = exam ? ['study'] : [];
    const free = () => 3 - slots.length;
    const can = k => !W.activityState(s, k);
    const usedPct = W.hddUsed(s) / s.hddCap;
    // 시험 달: 성적 3점(공부 2번 + 엄마 인내심 60) 노리기
    if (exam && free()) slots.push('study');
    // 엄마 인내심 관리
    if (s.mom < 40 && free()) slots.push(s.money < 10000 ? 'chores' : 'study');
    // 장비/공CD 쇼핑이 필요하면 용산
    const wantShop = (s.blank < 4 && s.pc.burner && s.money >= 12000) ||
      (!s.pc.hdd20 && s.turn >= 12 && s.money >= 82000) || (!s.pc.hdd40 && s.turn >= 24 && s.money >= 112000) ||
      (goal.kind === 'artisan' && !s.pc.scanner && s.turn >= 10 && s.money >= 47000) ||
      (!s.pc.usb && s.turn >= 24 && s.money >= 37000 && s.turn <= 25) ||
      (goal.kind !== 'artisan' && goal.kind !== 'completer' && s.turn >= 18 && s.money >= 60000);
    if (wantShop && free() && can('yongsan')) slots.push('yongsan');
    // 하드가 차면 굽기
    if (usedPct > 0.55 && can('burn') && free()) slots.push('burn');
    if (s.pc.frag >= 65 && can('pc') && free()) slots.push('pc');
    // 전설: 서점·용산·동호회·친구를 골고루 (가장 덜 한 것부터 한 달에 하나)
    if (goal.kind === 'legend' && free()) {
      const acts = s.stats.acts || {};
      const need = ['scan', 'friend', 'club', 'yongsan'].filter(k => (acts[k] || 0) < 5 && can(k)).sort((a, b) => (acts[a] || 0) - (acts[b] || 0));
      if (need.length) slots.push(need[0]);
    }
    // 주 활동 채우기
    // 받을 목표 작품이 다 떨어졌으면 다운로드 대신 심부름(돈)·공부(엄마)
    const targetLeft = !goal.genre || W.availableWorks(s).some(w => matchGenre(w));
    const main = !targetLeft && goal.kind === 'archiver' ? ['chores', 'study'] : goal.kind === 'artisan' ? ['scan', 'download'] :
      goal.kind === 'sharer' ? ['club', 'friend', 'club'] : ['download'];
    let guard = 0;
    while (free() && guard++ < 10) {
      let k = main.find(m => can(m) && !(m === 'scan' && (s.money < 3500 * 3 || (goal.kind === 'artisan' && !s.pc.scanner))));
      if (!k && goal.kind === 'artisan' && !s.pc.scanner) k = 'chores'; // 스캐너 살 돈 모으기
      if (k === 'download' && slots.filter(x => x === 'download').length >= 2 && W.hddUsed(s) / s.hddCap > 0.3 && can('burn') && !slots.includes('burn')) k = 'burn';
      if (!k) k = s.money < 8000 ? 'chores' : (can('download') ? 'download' : 'chores');
      slots.push(k);
    }
    return slots;
  }

  const PREF = ['open', 'join', 'share', 'yes', 'have', 'sit', 'go', 'both', 'gift', 'her', 'buy', 'y', 'resume', 'trick', 'hyung', 'scan', 'diy', 'dad', 'sorry', 'retry', 'reply', 'burn', 'col', 'note', 'later'];
  let shopVisits = 0;
  async function choose(prompt, opts) {
    const s = S();
    const ok = opts.filter(o => !o.disabled);
    const p = prompt || '';
    if (goal.kind === 'random') {
      if (p.includes('가진 돈')) { shopVisits++; if (shopVisits > 2) { shopVisits = 0; return 'back'; } }
      if (p.includes('정말 포맷')) return Math.random() < 0.5 ? 'y' : 'n';
      return ok[Math.floor(Math.random() * ok.length)].value;
    }
    if (p.includes('가진 돈')) { // 용산 상점
      shopVisits++;
      const order = [];
      if (goal.kind === 'artisan' && !s.pc.scanner) order.push('scanner');
      order.push('hdd40', 'hdd20');
      if (s.pc.driveBroken) order.unshift('repair');
      if (!s.pc.usb && s.turn <= 26) order.push('usb');
      if (s.pc.burner && !s.pc.fastBurner && s.money > 60000) order.push('burner24');
      if (s.blank < 15) order.push(s.money > 40000 ? 'cd30' : 'cd10');
      if (s.pc.drive < 50) order.push('lens');
      const gearDone = s.pc.hdd40 || s.turn < 24;
      if (gearDone && s.money > 35000 && goal.kind !== 'artisan') order.push('bundle');
      for (const v of order) { const o = ok.find(x => x.value === v); if (o && shopVisits < 6) return v; }
      shopVisits = 0;
      return 'back';
    }
    if (p.includes('정말 포맷')) return 'n';
    if (p.includes('몇 권')) return Math.max(...ok.map(o => o.value));
    if (p.includes('마지막으로')) return s.love >= 90 ? 'confess' : s.love >= 60 ? 'letter' : 'confess';
    if (opts.some(o => o.value === 'used') && false) return 'used';
    for (const v of PREF) { const o = ok.find(x => x.value === v); if (o) return v; }
    return ok[0].value;
  }

  async function pickWorks(title, works, o) {
    const s = S();
    if (!works.length) return o.allowCancel ? null : [];
    if (title.includes('다운로드')) {
      // 목표가 있으면 목표 장르 작품만 받는다 (없으면 목표 매체, 그것도 없으면 아무거나)
      let pool = works;
      if (goal.genre && (goal.kind === 'archiver' || goal.kind === 'completer')) {
        const g = works.filter(matchGenre);
        pool = g.length ? g : (works.filter(matchMedia).length ? works.filter(matchMedia) : works);
        if (goal.kind === 'archiver' && goal.media) {
          // 목표 매체가 1위를 유지하도록: 목표 장르×매체 우선, 그다음은 목표 매체를 넘어서지 않을 매체만
          const both = g.filter(matchMedia);
          if (both.length) pool = both;
          else {
            const rank = W.computeTraits(s).mediaRank;
            const media = W.computeTraits(s).media;
            const safe = g.filter(w => { const m = Object.keys(MEDIA).find(k => MEDIA[k].includes(w.type)); return media[m] < Math.min(0.5, media[goal.media] - 0.2); });
            pool = safe.length ? safe : [];
          }
        }
        if (!pool.length) return [];
      }
      // 완결주의자: 작품 수를 15개 안쪽으로 유지하고, 시작한 것만 끝낸다
      if (goal.kind === 'completer' && W.computeTraits(s).ownedCount >= 8) {
        const started = works.filter(w => W.owned(s, w.id) > 0);
        pool = started; // 새 작품은 시작하지 않는다
        if (!pool.length) return [];
      }
      if (goal.kind === 'random') return works.slice().sort(() => Math.random() - 0.5).slice(0, 1 + Math.floor(Math.random() * 3)).map(w => w.id);
      return best(pool, 3).map(w => w.id);
    }
    if (goal.kind === 'random') return [works[Math.floor(Math.random() * works.length)].id];
    if (title.includes('묶음') && goal.kind === 'archiver' && goal.media) {
      const both = works.filter(w => matchGenre(w) && matchMedia(w));
      return both.length ? best(both, 1).map(w => w.id) : null;
    }
    if (title.includes('서점') && goal.kind === 'artisan' && s.pc.scanner) { const m = works.filter(w => w.type === 'manga'); if (m.length) return best(m, 1).map(w => w.id); }
    if (title.includes('서점') || title.includes('교환') || title.includes('묶음')) return best(works, 1).map(w => w.id);
    // 히로인 선물: 히로인 취향에 맞는 것(대사 힌트로 알 수 있다고 가정)
    const T = W.HEROINE_TASTES[s.heroine.taste];
    const liked = works.filter(w => T.likes(w));
    return [(liked[0] || works[0]).id];
  }
  return { plan, choose, pickWorks };
}

// ---------- 한 판 실행 ----------
async function runGame(goal, seed) {
  const ctx = {
    console, setTimeout, clearTimeout, setInterval, clearInterval, performance: { now: () => 0 },
    localStorage: { getItem: () => null, setItem() {}, removeItem() {} },
    document: { fonts: { load: async () => {} }, getElementById: () => ({}), addEventListener() {} },
    window: {},
  };
  vm.createContext(ctx);
  vm.runInContext(`Math.random = (${makeRng.toString()})(${seed});`, ctx);
  const stubEl = {};
  let policy;
  ctx.UI = {
    h: () => stubEl, init() {}, setScene() {}, setPanel() {}, updateStatus() {}, win: () => stubEl, workRow: () => stubEl,
    say: async () => {}, talk: async () => {}, notice: async () => {}, progress: async () => {}, runDefrag: async () => {},
    choose: (p, o) => policy.choose(p, o), pickWorks: (t, w, o) => policy.pickWorks(t, w, o || {}),
  };
  ctx.Sound = { sfx() {}, bgm() {}, unlock() {}, setMuted() {}, isMuted: () => true };
  ctx.Art = {};
  for (const f of ['data.js', 'game.js', 'story.js']) vm.runInContext(fs.readFileSync(ROOT + f, 'utf8'), ctx, { filename: f });
  // main.js는 마지막 줄 boot() 호출을 빼고 불러온다
  const mainSrc = fs.readFileSync(ROOT + 'main.js', 'utf8').replace(/\nboot\(\);\s*$/, '\n');
  vm.runInContext(mainSrc, ctx, { filename: 'main.js' });
  // const/function 선언을 밖에서 쓰기 위한 다리
  const W = vm.runInContext(`({ G, Meta, newState, dateOf, EXAM_MONTHS, activityState, hddUsed, remaining, owned, computeTraits,
    TYPES, HEROINE_TASTES, availableWorks, decideEnding, decideRomance, allowance, runActivity, monthEndUpkeep, Story, MEDIA_NAMES, METHOD_NAMES, GENRES })`, ctx);
  W.Meta.load();
  const S = () => W.G.s;
  policy = makePolicy(goal, { S, W });

  W.G.s = W.newState('테스트');
  const s = W.G.s;
  await W.Story.prologue();
  const spent = { min: 1e9 };
  while (s.turn < 36) {
    s.monthStudy = 0;
    s.lastAllowance = W.allowance(s);
    s.money += s.lastAllowance;
    await W.Story.monthStart();
    s.startDone = s.turn;
    const plan = policy.plan();
    for (let i = 0; i < 3; i++) await W.runActivity(plan[i], i);
    await W.monthEndUpkeep();
    await W.Story.monthEnd();
    spent.min = Math.min(spent.min, s.money);
    s.turn++;
  }
  await W.Story.graduation();
  const t = W.computeTraits(s);
  return {
    goal: goal.label, seed,
    ending: W.Story.ENDINGS[W.decideEnding(s)].title,
    romance: W.Story.ROMANCE[W.decideRomance(s)].title,
    love: s.love, taste: s.heroine.taste,
    top: t.top ? W.GENRES[t.top].name : '-', topScore: Math.round(t.topScore), topShare: +(t.topShare * 100).toFixed(0),
    media: t.mediaTop ? W.MEDIA_NAMES[t.mediaTop] : '-', mediaPct: Object.fromEntries(Object.entries(t.media).map(([k, v]) => [k, Math.round(v * 100)])),
    method: t.methodKey ? W.METHOD_NAMES[t.methodKey] : '-', methodScores: Object.fromEntries(Object.entries(t.method).map(([k, v]) => [k, Math.round(v)])),
    total: Math.round(t.total), completed: t.completed, works: t.ownedCount, discs: s.stats.discs, formats: s.stats.formats,
    moneyEnd: s.money, moneyMin: spent.min, gear: ['scanner', 'fastBurner', 'hdd20', 'hdd40', 'usb'].filter(k => s.pc[k]),
    mom: s.mom, bans: s.banTurn >= 0,
  };
}

// ---------- 실행 목록 ----------
const GOALS = [];
const G6 = ['hot', 'love', 'sf', 'mys', 'fan', 'life'];
const GN = { hot: '열혈', love: '순정', sf: '메카SF', mys: '미스터리', fan: '판타지', life: '일상' };
const MN = { game: '게임', video: '영상', paper: '책', music: '음악' };
for (const g of G6) for (const m of ['game', 'video', 'paper', 'music']) GOALS.push({ kind: 'archiver', genre: g, media: m, label: `다운로드:${GN[g]}×${MN[m]}` });
GOALS.push({ kind: 'legend', label: '전설 노리기(잡식·완결)' });
for (const g of ['hot', 'love', 'sf', 'fan']) GOALS.push({ kind: 'artisan', genre: g, media: 'paper', label: `스캔:${GN[g]}` });
for (const g of ['hot', 'life', 'love']) GOALS.push({ kind: 'sharer', genre: g, label: `교환:${GN[g]}` });
for (const g of ['fan', 'love', 'hot']) GOALS.push({ kind: 'completer', genre: g, label: `완결:${GN[g]}` });
GOALS.push({ kind: 'completer', label: '완결:장르 섞어서' });
for (let i = 0; i < 4; i++) GOALS.push({ kind: 'random', label: `무작위 플레이 ${i + 1}` });

(async () => {
  const seeds = +(process.argv[2] || 2);
  const results = [];
  for (const goal of GOALS) for (let k = 0; k < seeds; k++) {
    try { results.push(await runGame(goal, 1000 + k * 7919 + GOALS.indexOf(goal) * 31)); }
    catch (e) { results.push({ goal: goal.label, seed: k, error: String(e.stack).split('\n').slice(0, 3).join(' / ') }); }
  }
  if (process.argv[3]) fs.writeFileSync(process.argv[3], JSON.stringify(results, null, 1));
  const ok = results.filter(r => !r.error);
  const count = key => Object.entries(ok.reduce((a, r) => ((a[r[key]] = (a[r[key]] || 0) + 1), a), {})).sort((a, b) => b[1] - a[1]);
  for (const r of ok) console.log(`${r.goal.padEnd(20)} → ${r.ending.padEnd(14)} | ${r.top}${r.topScore}(${r.topShare}%) ${r.media} ${r.method} | 점수 ${r.total} 완결 ${r.completed}/${r.works} | 호감 ${r.love} ${r.romance} | 남은 돈 ${r.moneyEnd}`);
  console.log('\n판 수', results.length, '오류', results.length - ok.length);
  console.log('엔딩', count('ending').length + '종', JSON.stringify(count('ending')));
  console.log('연애', JSON.stringify(count('romance')));
  for (const r of results.filter(r => r.error)) console.log('ERROR', r.goal, r.error);
})();
