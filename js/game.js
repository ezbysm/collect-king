// 수집왕 — 게임 상태와 규칙
'use strict';

const SAVE_KEY = 'sujipking_save_v1';
const META_KEY = 'sujipking_meta_v1';
const TOTAL_TURNS = 36;
const SPEED = { modem: 150, adsl: 2000 };      // 수집 활동 1칸당 MB
const CD_MB = 700;
const MANGA_PRICE = 3500;
const NOVEL_PRICE = 6000;
const PHYS_BONUS = 0.5;   // 실물(산 책, 받은·산 CD)은 성향 점수 1.5배

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

// 공CD 한 장 가격: 2000년 2,000원 → 2001년 1,200원 → 2002년 이후 700원
function cdPrice(turn) {
  const y = dateOf(turn).y;
  return y === 2000 ? 2000 : y === 2001 ? 1200 : 700;
}
const PRICES = {
  friend: 4000,     // PC방·떡볶이
  club: 1000,       // 우편 교환 우표값 (공CD 1장 별도)
  yongsan: 2000,    // 지하철 왕복 차비
  phone: 2000,      // 모뎀으로 밤새 받은 날 하루치 전화요금 몫
};

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
    pc: { frag: 10, line: 80, drive: 100, driveBroken: false, burner: false, scanner: false, usb: false, fastBurner: false, hdd20: false, hdd40: false },
    allowanceMod: 0, monthStudy: 0, phoneNights: 0, lastAllowance: 0,
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
  if (!s.col[id]) s.col[id] = { hdd: 0, cd: 0, part: 0, book: 0, phys: 0 };
  return s.col[id];
}
// 책장에 꽂힌 실물 책(book)도 소장으로 친다
function owned(s, id) { const e = s.col[id]; return e ? e.hdd + e.cd + (e.book || 0) : 0; }
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
  // 실물: 서점에서 산 책, 친구·동호회에게 받은 CD, 용산에서 산 CD
  if (method === 'scan' || method === 'trade' || method === 'buy' || method === 'book') e.phys = (e.phys || 0) + n;
  if (method === 'download') s.method.download += pts * 0.6;
  if (method === 'scan') s.method.scan += pts * 3;
  if (method === 'trade') s.method.trade += pts * 1.5;
  // 'buy'(용산에서 돈으로 산 것)는 수집 방식 점수에 넣지 않는다
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
  if (used) s.method.download += used * 0.5; // 정리하는 습관은 아카이버의 덕목
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
  let total = 0, completed = 0, ownedCount = 0, completedPts = 0, physPts = 0;
  for (const w of WORKS) {
    const n = owned(s, w.id);
    if (!n) continue;
    ownedCount++;
    const phys = Math.min((s.col[w.id] && s.col[w.id].phys) || 0, n) * TYPES[w.type].weight;
    physPts += phys;
    const pts = n * TYPES[w.type].weight + phys * PHYS_BONUS;
    if (n >= w.units) { completed++; completedPts += pts; }
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
    // 완결주의자: 적게 모으되 끝까지 (모은 작품 수가 많으면 후보가 아님)
    completer: completed >= ENDING_RULES.completerMinDone && ownedCount <= ENDING_RULES.completerMaxWorks && ratio >= 0.8 ? completedPts * 0.8 * ratio : 0,
  };
  const mTop = Object.entries(method).sort((a, b) => b[1] - a[1])[0];
  const methodKey = mTop[1] > 0 ? mTop[0] : null;
  let breadth = null;
  if (total > 20) breadth = topShare < ENDING_RULES.omniShare ? 'omni' : topShare > ENDING_RULES.deepShare ? 'deep' : null;
  // 매체 비교: 1위 장르의 작품 중, 매체별로 모을 수 있는 것의 몇 %를 모았는지
  // (예: 열혈 게임은 거의 다 모았고 열혈 애니는 절반이면 → 게임)
  const media = {}, mediaPts = {}, mediaRank = {};
  for (const [k, types] of Object.entries(MEDIA)) {
    let got = 0, pool = 0;
    for (const w of WORKS) {
      if (!types.includes(w.type) || !top || !w.genres.includes(top)) continue;
      pool += w.units * TYPES[w.type].weight;
      got += owned(s, w.id) * TYPES[w.type].weight;
    }
    media[k] = pool ? got / pool : 0;
    mediaPts[k] = got;
    // 조금만 모은 매체는 1위가 될 수 없다 (게임 CD 몇 장으로 프로게이머가 되지 않도록)
    mediaRank[k] = got > 0 && got >= Math.min(ENDING_RULES.minMediaPts, pool * 0.7) ? media[k] : -1;
  }
  const mediaTop = Object.entries(mediaRank).sort((a, b) => (b[1] - a[1]) || (mediaPts[b[0]] - mediaPts[a[0]]))[0];
  return { genre, total, top, topShare, topScore: sorted[0][1], completed, ownedCount, method, methodKey, breadth, media, mediaRank, physPts, mediaTop: mediaTop[1] > 0 ? mediaTop[0] : (top ? Object.entries(mediaPts).sort((a, b) => b[1] - a[1])[0][0] : null) };
}

const METHOD_NAMES = { archiver: '아카이버', artisan: '손품장인', sharer: '나눔꾼', completer: '완결주의자' };

// 매체 묶음
const MEDIA = { video: ['anime', 'movie'], paper: ['manga', 'novel'], game: ['game'], music: ['music'] };
const MEDIA_NAMES = { video: '영상(애니·영화)', paper: '책(만화·소설)', game: '게임', music: '음악' };
const MEDIA_MAX = Object.fromEntries(Object.entries(MEDIA).map(([k, types]) =>
  [k, WORKS.filter(w => types.includes(w.type)).reduce((a, w) => a + w.units * TYPES[w.type].weight, 0)]));

// 엔딩 판정 기준
const ENDING_RULES = {
  minTopGenre: 110,      // 1위 장르 점수가 이보다 낮으면 평범한 직장인
  legendTotal: 500,      // 전설의 수집왕: 수집 점수
  legendCompleted: 20,   // 전설의 수집왕: 완결 작품 수
  legendPhys: 120,       // 전설의 수집왕: 실물 점수 (산 책·받은 CD·산 CD)
  legendActs: 4,         // 전설의 수집왕: 주요 활동을 각각 이 횟수 이상
  legendFriend: 60,      // 전설의 수집왕: 친구와의 우정
  omniShare: 0.22,       // 1위 장르 비율이 이보다 낮으면 잡식가
  deepShare: 0.55,       // 1위 장르 비율이 이보다 높으면 한 우물
  completerMaxWorks: 12, // 완결주의자 후보: 모은 작품 수 상한
  completerMinDone: 8,   // 완결주의자 후보: 완결 작품 수 하한
  minMediaPts: 40,       // 매체 1위가 되려면 그 매체에서 최소 이만큼 (게임 CD 6장, 음악 10장, 애니 27화, 만화 40권 정도)
};
// 아카이버(기본)일 때: 1위 장르 × 가장 많이 모은 매체
const ENDING_TABLE = {
  hot:  { game: 'progamer', video: 'stunt',    paper: 'pe_teacher', music: 'rocker' },
  love: { game: 'vn_writer', video: 'drama_pd', paper: 'webnovel',  music: 'lyricist' },
  sf:   { game: 'gamedev',  video: 'vfx',      paper: 'it',         music: 'edm' },
  mys:  { game: 'escape',   video: 'docu',     paper: 'reporter',   music: 'forensic' },
  fan:  { game: 'planner',  video: 'animator', paper: 'novelist',   music: 'composer' },
  life: { game: 'streamer', video: 'vlogger',  paper: 'bookshop',   music: 'cafe' },
};

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
  // 완결주의자는 '적게, 끝까지'라서 수집량 기준 대신 완결로 판정한다
  if (!t.top || (t.methodKey !== 'completer' && t.topScore < ENDING_RULES.minTopGenre)) return 'ordinary';
  const g = t.top;
  if (legendChecklist(s, t).every(c => c.ok)) return 'legend';
  switch (t.methodKey) {
    case 'artisan': return g === 'sf' ? 'gamedev' : 'webtoon';
    case 'sharer':
      if (g === 'hot' || g === 'sf') return 'yongsan';
      if (g === 'life' || g === 'fan') return 'teacher';
      return 'radio';
    case 'completer':
      if (t.breadth === 'deep') return 'fanclub';
      if (g === 'fan') return 'novelist';
      return 'editor';
    default: // archiver: 장르 × 매체
      return ENDING_TABLE[g][t.mediaTop || 'video'];
  }
}

// 전설의 수집왕 조건 — '내 컴퓨터'에서 진행 상황을 볼 수 있다
const LEGEND_ACTS = { download: '다운로드', burn: 'CD 굽기', scan: '서점', yongsan: '용산', club: '동호회', friend: '친구' };
function legendChecklist(s, t = computeTraits(s)) {
  const R = ENDING_RULES;
  const acts = (s.stats && s.stats.acts) || {};
  const list = [
    { label: '장르를 고르게', ok: t.breadth === 'omni', now: `1위 장르 ${Math.round(t.topShare * 100)}%`, need: `${Math.round(R.omniShare * 100)}% 미만` },
    { label: '수집 점수', ok: t.total >= R.legendTotal, now: Math.round(t.total), need: R.legendTotal },
    { label: '완결 작품', ok: t.completed >= R.legendCompleted, now: t.completed, need: R.legendCompleted },
    { label: '실물 점수', ok: t.physPts >= R.legendPhys, now: Math.round(t.physPts), need: R.legendPhys },
    { label: '친구와의 우정', ok: s.friend >= R.legendFriend, now: Math.round(s.friend), need: R.legendFriend },
  ];
  for (const [k, name] of Object.entries(LEGEND_ACTS)) list.push({ label: `${name} ${R.legendActs}번`, ok: (acts[k] || 0) >= R.legendActs, now: acts[k] || 0, need: R.legendActs });
  return list;
}

function decideRomance(s) {
  const c = s.flags.gradChoice;
  const L = s.love;
  if (c === 'confess' && L >= 90) return 'together';
  if ((c === 'confess' || c === 'letter') && L >= 60) return 'letter';
  if (L >= 35) return 'reunion';
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
