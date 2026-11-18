// 수집왕 — 게임 상태와 규칙
'use strict';

const SAVE_KEY = 'sujipking_save_v1';
const META_KEY = 'sujipking_meta_v1';
const TOTAL_TURNS = 36;
const SPEED = { modem: 150, adsl: 2000 };      // 수집 활동 1칸당 MB
const CD_MB = 700;
const MANGA_PRICE = 3500;

const G = { s: null };

function rand(a, b) { return a + Math.random() * (b - a); }
function chance(p) { return Math.random() < p; }
function pick(arr) { return arr[Math.floor(Math.random() * arr.length)]; }
function clamp(v, lo = 0, hi = 100) { return Math.max(lo, Math.min(hi, v)); }
function won(n) { return '₩' + Math.round(n).toLocaleString('ko-KR'); }
function mb(n) { return n >= 1000 ? (n / 1000).toFixed(1) + 'GB' : Math.round(n) + 'MB'; }

function dateOf(turn) {
  const y = 2000 + Math.floor((turn + 2) / 12);
  const m = ((turn + 2) % 12) + 1;
  return { y, m, ym: y * 100 + m, grade: Math.floor(turn / 12) + 1 };
}
function dateLabel(turn) {
  const d = dateOf(turn);
  return `${d.y}년 ${d.m}월`;
}

function newState(name) {
  return {
    v: 1, name, turn: 0, startDone: -1,
    heroine: {
      name: pick(HEROINE_SURNAMES) + pick(HEROINE_GIVEN),
      taste: pick(Object.keys(HEROINE_TASTES)),
      sameClass: true,
    },
    money: 10000, blank: 3,
    hddCap: 3000,
    net: 'modem',
    pc: { frag: 10, line: 80, drive: 100, driveBroken: false, burner: false, scanner: false, usb: false },
    mom: 70, friend: 30, love: 10,
    col: {},          // id -> { hdd, cd, part }
    seen: {},
    method: { download: 0, scan: 0, trade: 0, share: 0 },
    stats: { downloads: 0, modemDownloads: 0, burns: 0, discs: 0, defrags: 0, formats: 0, club: 0, gifts: 0, lost: 0 },
    flags: {},
    banTurn: -1,
    screen: 'desktop',
  };
}

// ---------- 컬렉션 ----------
function entry(s, id) {
  if (!s.col[id]) s.col[id] = { hdd: 0, cd: 0, part: 0 };
  return s.col[id];
}
function owned(s, id) { const e = s.col[id]; return e ? e.hdd + e.cd : 0; }
function remaining(s, w) { return Math.max(0, w.units - owned(s, w.id)); }
function isComplete(s, w) { return owned(s, w.id) >= w.units; }
function hddUsed(s) {
  let t = 0;
  for (const id in s.col) { const e = s.col[id]; t += e.hdd * WORK_BY_ID[id].size + e.part; }
  return t;
}
function hddFree(s) { return Math.max(0, s.hddCap - hddUsed(s)); }
function availableWorks(s) {
  const ym = dateOf(s.turn).ym;
  return WORKS.filter(w => w.appear <= ym && remaining(s, w) > 0);
}
function ownedWorks(s) { return WORKS.filter(w => owned(s, w.id) > 0); }
function isNew(s, w) { return w.appear === dateOf(s.turn).ym && w.appear > 200003; }
function markSeen(s, id) { s.seen[id] = true; Meta.seeWork(id); }

function addUnits(s, w, n, where, method) {
  if (n <= 0) return;
  const e = entry(s, w.id);
  e[where] += n;
  markSeen(s, w.id);
  const pts = n * TYPES[w.type].weight;
  if (method === 'download') s.method.download += pts;
  if (method === 'scan') s.method.scan += pts * 3;
  if (method === 'trade') s.method.trade += pts * 1.5;
}

// 다운로드: queue 순서대로 budget(MB)만큼 받는다
function doDownload(s, ids, budget) {
  const res = [];
  let full = false;
  for (const id of ids) {
    const w = WORK_BY_ID[id];
    const e = entry(s, id);
    let got = 0;
    while (budget > 0 && remaining(s, w) > 0) {
      const free = hddFree(s);
      if (free <= 0) { full = true; break; }
      const take = Math.min(w.size - e.part, budget, free);
      e.part += take; budget -= take;
      if (e.part >= w.size - 0.001) { e.part = 0; addUnits(s, w, 1, 'hdd', 'download'); got++; }
    }
    if (e.part > 0 || got > 0) markSeen(s, id);
    res.push({ w, got, part: e.part });
    if (full) break;
  }
  s.stats.downloads++;
  if (s.net === 'modem') s.stats.modemDownloads++;
  return { res, full, leftover: budget };
}

// 굽기: 하드의 파일을 CD로 옮긴다
function doBurn(s, maxDiscs) {
  const discs = Math.min(s.blank, maxDiscs);
  let cap = discs * CD_MB;
  const moved = [];
  const list = Object.keys(s.col).map(id => WORK_BY_ID[id]).filter(w => s.col[w.id].hdd > 0)
    .sort((a, b) => b.size - a.size);
  let usedMB = 0;
  for (const w of list) {
    const e = s.col[w.id];
    let n = 0;
    while (e.hdd > 0 && cap >= w.size) { e.hdd--; e.cd++; cap -= w.size; usedMB += w.size; n++; }
    if (n) moved.push({ w, n });
  }
  const used = Math.ceil(usedMB / CD_MB);
  s.blank -= used;
  s.stats.discs += used;
  s.stats.burns++;
  if (used) s.method.download += used * 1.5; // 정리하는 습관은 아카이버의 덕목
  return { moved, used, usedMB };
}

// 포맷: 하드에만 있던 파일이 전부 사라진다
function doFormat(s) {
  let lost = 0;
  for (const id in s.col) { const e = s.col[id]; lost += e.hdd; e.hdd = 0; e.part = 0; }
  s.pc.frag = 0;
  s.stats.formats++;
  s.stats.lost += lost;
  return lost;
}

// 바이러스: 하드의 무작위 작품 일부 손상(소실)
function virusDamage(s) {
  const ids = Object.keys(s.col).filter(id => s.col[id].hdd > 0);
  if (!ids.length) return null;
  const id = pick(ids);
  const e = s.col[id];
  const n = Math.max(1, Math.ceil(e.hdd * 0.3));
  e.hdd -= n; s.stats.lost += n;
  return { w: WORK_BY_ID[id], n };
}

// CD 한 장 분량으로 받기 (교환/선물/중고)
function receiveDisc(s, w, method = 'trade') {
  const n = Math.max(1, Math.min(remaining(s, w), Math.floor(CD_MB / w.size)));
  addUnits(s, w, n, 'cd', method);
  s.stats.discs++;
  return n;
}

// ---------- 성향 ----------
function computeTraits(s) {
  const genre = Object.fromEntries(Object.keys(GENRES).map(k => [k, 0]));
  let total = 0, completed = 0, ownedCount = 0;
  for (const w of WORKS) {
    const n = owned(s, w.id);
    if (!n) continue;
    ownedCount++;
    if (n >= w.units) completed++;
    const pts = n * TYPES[w.type].weight;
    total += pts;
    for (const g of w.genres) genre[g] += pts / w.genres.length * (w.genres.length > 1 ? 1.4 : 1);
  }
  const gSum = Object.values(genre).reduce((a, b) => a + b, 0) || 1;
  const sorted = Object.entries(genre).sort((a, b) => b[1] - a[1]);
  const top = sorted[0][1] > 0 ? sorted[0][0] : null;
  const topShare = sorted[0][1] / gSum;
  const ratio = ownedCount ? completed / ownedCount : 0;
  const method = {
    archiver: s.method.download,
    artisan: s.method.scan,
    sharer: s.method.share + s.method.trade,
    completer: completed * 14 * (0.4 + ratio),
  };
  const mTop = Object.entries(method).sort((a, b) => b[1] - a[1])[0];
  const methodKey = mTop[1] > 0 ? mTop[0] : null;
  let breadth = null;
  if (total > 20) breadth = topShare < 0.28 ? 'omni' : topShare > 0.42 ? 'deep' : null;
  return { genre, total, top, topShare, completed, ownedCount, method, methodKey, breadth };
}

const METHOD_NAMES = { archiver: '아카이버', artisan: '손품장인', sharer: '나눔꾼', completer: '완결주의자' };

function traitLabel(t) {
  if (!t.top) return '아직 없음';
  const parts = [GENRES[t.top].trait];
  if (t.methodKey) parts.push(METHOD_NAMES[t.methodKey]);
  if (t.breadth === 'omni') parts.push('잡식가');
  if (t.breadth === 'deep') parts.push('한 우물');
  return parts.join(' · ');
}

function decideEnding(s) {
  const t = computeTraits(s);
  if (t.total < 45 || !t.top) return 'ordinary';
  const g = t.top;
  switch (t.methodKey) {
    case 'artisan': return (g === 'sf' || g === 'hot') ? 'gamedev' : 'webtoon';
    case 'sharer':
      if (g === 'hot' || g === 'sf') return 'yongsan';
      if (g === 'life' || g === 'fan') return 'teacher';
      return 'radio';
    case 'completer':
      if (t.breadth === 'deep') return 'fanclub';
      if (g === 'fan') return 'novelist';
      return 'editor';
    default: // archiver
      if (t.breadth === 'omni' && t.total >= 150) return 'legend';
      if (g === 'sf') return 'it';
      if (g === 'mys') return 'reporter';
      return t.total >= 150 ? 'legend' : 'it';
  }
}

function decideRomance(s) {
  const c = s.flags.gradChoice;
  const L = s.love;
  if (c === 'confess' && L >= 75) return 'together';
  if ((c === 'confess' || c === 'letter') && L >= 50) return 'letter';
  if (L >= 30) return 'reunion';
  return 'memory';
}

// ---------- 저장 ----------
const Save = {
  has() { try { return !!localStorage.getItem(SAVE_KEY); } catch (e) { return false; } },
  write(s) { try { localStorage.setItem(SAVE_KEY, JSON.stringify(s)); } catch (e) { /* 저장 불가 환경 */ } },
  read() { try { return JSON.parse(localStorage.getItem(SAVE_KEY)); } catch (e) { return null; } },
  clear() { try { localStorage.removeItem(SAVE_KEY); } catch (e) { /* noop */ } },
};

const Meta = {
  data: null,
  load() {
    try { this.data = JSON.parse(localStorage.getItem(META_KEY)); } catch (e) { this.data = null; }
    if (!this.data) this.data = { endings: {}, romance: {}, seen: {}, plays: 0 };
    return this.data;
  },
  save() { try { localStorage.setItem(META_KEY, JSON.stringify(this.data)); } catch (e) { /* noop */ } },
  seeWork(id) { if (!this.data) this.load(); if (!this.data.seen[id]) { this.data.seen[id] = true; this.save(); } },
  addEnding(e, r) { this.data.endings[e] = true; this.data.romance[r] = true; this.data.plays++; this.save(); },
};
