// 수집왕 — 게임 진행 (타이틀, 허브, 일정, 활동, 위험·회복 이벤트)
'use strict';

const { h } = UI;
const EXAM_MONTHS = [4, 7, 10, 12];

// ---------- 활동 정의 ----------
const ACTIVITIES = {
  download: { name: '밤새 다운로드', icon: '⇩', desc: '작품을 골라 인터넷 속도만큼 받는다.' },
  burn:     { name: 'CD 굽기', icon: '◎', desc: '하드의 파일을 CD로 옮겨 안전하게 보관.' },
  scan:     { name: '만화책 사서 스캔', icon: '▤', desc: `한 권 ${won(MANGA_PRICE)}. 스캐너 필요.` },
  pc:       { name: '컴퓨터 정비', icon: '⚙', desc: '조각 모음, 스캔디스크, 회선 점검.' },
  yongsan:  { name: '용산 원정', icon: '⌂', desc: `차비 ${won(PRICES.yongsan)}. 공CD·부품·하드·중고 CD.` },
  club:     { name: '동호회 활동', icon: '☺', desc: `CD를 우편 교환. 공CD 1장+우표 ${won(PRICES.club)}.` },
  friend:   { name: '친구와 놀기', icon: '♪', desc: `PC방·떡볶이 ${won(PRICES.friend)}. CD를 주고받기도.` },
  chores:   { name: '심부름·집안일', icon: '₩', desc: '용돈 +4,000, 엄마 인내심 회복.' },
  study:    { name: '공부', icon: '✎', desc: '엄마 인내심 회복. 시험 성적→용돈.' },
  rest:     { name: '일찍 자기', icon: '☾', desc: '엄마 인내심 회복. 다운로드는 쉰다.' },
};

function activityState(s, key) {
  const banned = s.banTurn === s.turn;
  switch (key) {
    case 'download':
      if (banned) return '컴퓨터 금지령';
      if (hddFree(s) < 1) return '하드가 꽉 찼다';
      return null;
    case 'burn':
      if (banned) return '컴퓨터 금지령';
      if (!s.pc.burner) return 'CD 버너가 없다';
      if (s.pc.driveBroken) return '드라이브 고장';
      if (s.blank <= 0) return '공CD가 없다';
      if (hddUsed(s) - partsMB(s) < 1) return '구울 파일이 없다';
      return null;
    case 'scan':
      if (!s.pc.scanner) return '스캐너가 없다';
      if (s.money < MANGA_PRICE) return '돈이 없다';
      return null;
    case 'pc': return banned ? '컴퓨터 금지령' : null;
    case 'yongsan': return s.money < PRICES.yongsan ? '차비가 없다' : null;
    case 'club':
      if (s.blank <= 0) return '보낼 공CD가 없다';
      if (s.money < PRICES.club) return '우표값이 없다';
      return null;
    default: return null;
  }
}
function partsMB(s) { let t = 0; for (const id in s.col) t += s.col[id].part; return t; }

// ---------- 시작 ----------
async function boot() {
  Meta.load();
  UI.init();
  try { await document.fonts.load('11px Galmuri11'); } catch (e) { /* 글꼴 없이 진행 */ }
  title();
}

function title() {
  G.s = null;
  Sound.bgm('title');
  UI.setScene('title');
  UI.updateStatus();
  const m = Meta.data;
  const nEnd = Object.keys(m.endings).length;
  UI.setPanel(h('div', { class: 'title-menu' },
    h('h1', { class: 'logo' }, '수집왕', h('small', {}, '2000 - 2003 · 어느 수집가의 중학교 3년')),
    h('button', { class: 'btn big primary', onclick: newGame }, '새로 시작'),
    h('button', { class: 'btn big', disabled: !Save.has(), onclick: continueGame }, '이어하기'),
    h('button', { class: 'btn big', onclick: records }, `기록 보기 (엔딩 ${nEnd}/${Object.keys(Story.ENDINGS).length})`),
    h('p', { class: 'credit' }, '도트·글꼴: Galmuri (OFL) · 개인 프로젝트'),
  ));
}

async function newGame() {
  if (Save.has()) {
    const c = await UI.choose('저장된 게임이 있습니다. 새로 시작하면 지워집니다.', [
      { label: '새로 시작', value: 'y' }, { label: '취소', value: 'n' }]);
    if (c !== 'y') return title();
  }
  const name = await askName();
  if (!name) return title();
  G.s = newState(name);
  Save.clear();
  UI.setScene('room');
  await Story.prologue();
  await gameLoop();
}

function askName() {
  return new Promise(resolve => {
    const input = h('input', { type: 'text', maxlength: 6, placeholder: '이름 (1~6자)', autocomplete: 'off', enterkeyhint: 'done' });
    const err = h('p', { class: 'err' });
    const ok = () => {
      const v = input.value.replace(/\s/g, '');
      if (!v.length) { err.textContent = '이름을 입력해 주세요.'; return; }
      if (v.length > 6) { err.textContent = '6자까지 입력할 수 있어요.'; return; }
      resolve(v);
    };
    input.addEventListener('keydown', e => { if (e.key === 'Enter') ok(); });
    UI.setPanel(UI.win('학생증', h('div', { class: 'idcard' },
      h('div', { class: 'idcard-head' }, '○○중학교 학생증'),
      h('div', { class: 'idcard-row' }, h('span', {}, '학년'), h('b', {}, '1학년')),
      h('div', { class: 'idcard-row' }, h('span', {}, '생년'), h('b', {}, '1987년')),
      h('label', { class: 'idcard-row' }, h('span', {}, '이름'), input),
      err,
    ), [h('button', { class: 'btn', onclick: () => resolve(null) }, '취소'), h('button', { class: 'btn primary', onclick: ok }, '확인')]));
    setTimeout(() => input.focus(), 50);
  });
}

async function continueGame() {
  const s = Save.read();
  if (!s) return title();
  G.s = s;
  UI.setScene('room');
  await gameLoop();
}

// ---------- 메인 루프 ----------
async function gameLoop() {
  const s = G.s;
  while (s.turn < TOTAL_TURNS) {
    if (s.startDone !== s.turn) {
      s.monthStudy = 0;
      s.lastAllowance = allowance(s);
      s.money += s.lastAllowance;
      s.screen = 'desktop';
      await Story.monthStart();
      s.startDone = s.turn;
      Save.write(s);
    }
    const plan = await hub();
    if (plan === 'quit') return title();
    for (let i = 0; i < 3; i++) {
      await runActivity(plan[i], i);
    }
    s.screen = 'off';
    await monthEndUpkeep();
    await Story.monthEnd();
    s.turn++;
    Save.write(s);
  }
  await Story.graduation();
  const r = await Story.ending();
  Save.clear();
  await UI.notice('THE END', [
    `${G.s.name}의 중학교 3년은 이렇게 끝났습니다.`,
    `엔딩: ${r.E.title} / ${r.Rm.title}`,
    '다른 것을 모으면, 다른 사람이 됩니다. 히로인의 이름과 취향도 매번 바뀝니다.',
  ], '타이틀로');
  title();
}

function allowance(s) {
  const y = dateOf(s.turn).y;
  const base = y === 2000 ? 8000 : y === 2001 ? 10000 : 12000;
  return Math.max(0, base + (s.allowanceMod || 0));
}

// ---------- 허브 ----------
function hub() {
  Sound.bgm('room');
  return new Promise(resolve => {
    const s = G.s;
    UI.setScene('room');
    s.screen = 'desktop';
    const d = dateOf(s.turn);
    const render = () => {
      const tr = computeTraits(s);
      UI.setPanel(h('div', { class: 'hub' },
        h('div', { class: 'hub-info' },
          h('div', {}, h('b', {}, `${d.y}년 ${d.m}월`), ` · 중학교 ${d.grade}학년`, EXAM_MONTHS.includes(d.m) ? h('em', { class: 'exam' }, '시험 기간') : null),
          h('div', { class: 'dim' }, `성향: ${traitLabel(tr)}`),
          h('div', { class: 'dim' }, `인터넷: ${s.net === 'adsl' ? 'ADSL' : '56k 모뎀'} · 남은 하드 ${mb(hddFree(s))}`),
          h('div', { class: 'dim' }, `이번 달 용돈 +${won(s.lastAllowance || allowance(s))}${s.allowanceMod > 0 ? ' (성적 보너스 포함)' : s.allowanceMod < 0 ? ' (성적 때문에 깎임)' : ''} · 공CD 장당 ${won(cdPrice(s.turn))}`),
          s.banTurn === s.turn ? h('div', { class: 'warn-text' }, '이번 달은 컴퓨터 금지령!') : null,
        ),
        h('button', { class: 'btn big primary', onclick: async () => { const p = await planMonth(); if (p) resolve(p); else render(); } }, '이번 달 계획'),
        h('div', { class: 'hub-grid' },
          h('button', { class: 'btn', onclick: async () => { await collectionView(); render(); } }, '내 컬렉션'),
          h('button', { class: 'btn', onclick: async () => { await computerView(); render(); } }, '내 컴퓨터'),
          h('button', { class: 'btn', onclick: async () => { const r = await menu(); if (r === 'quit') resolve('quit'); else render(); } }, '메뉴'),
        ),
      ));
      UI.updateStatus();
    };
    render();
  });
}

// ---------- 일정 짜기 ----------
function planMonth() {
  return new Promise(resolve => {
    const s = G.s;
    const d = dateOf(s.turn);
    const exam = EXAM_MONTHS.includes(d.m);
    const plan = [exam ? 'study' : null, null, null];
    let cur = exam ? 1 : 0;
    const SLOT = ['상순', '중순', '하순'];
    const render = () => {
      const slots = h('div', { class: 'slots' }, ...plan.map((a, i) => h('button', {
        class: 'slot' + (i === cur ? ' cur' : '') + (a ? ' set' : ''),
        disabled: exam && i === 0,
        onclick: () => { cur = i; render(); },
      }, h('small', {}, SLOT[i] + (exam && i === 0 ? ' (시험)' : '')), h('span', {}, a ? ACTIVITIES[a].name : '—'))));
      const grid = h('div', { class: 'acts' });
      for (const [k, a] of Object.entries(ACTIVITIES)) {
        const why = activityState(s, k);
        grid.append(h('button', {
          class: 'btn act', disabled: !!why,
          onclick: () => {
            plan[cur] = k;
            const next = plan.findIndex((p, i) => !p && !(exam && i === 0));
            cur = next >= 0 ? next : cur;
            render();
          },
        }, h('b', {}, a.icon + ' ' + a.name), h('small', {}, why || a.desc)));
      }
      const ready = plan.every(Boolean);
      UI.setPanel(UI.win(`${d.y}년 ${d.m}월 계획`, h('div', { class: 'plan' }, slots, grid), [
        h('button', { class: 'btn', onclick: () => resolve(null) }, '뒤로'),
        h('button', { class: 'btn primary', disabled: !ready, onclick: () => resolve(plan.slice()) }, '시작'),
      ]));
      document.getElementById('panel').className = 'tall';
      UI.updateStatus();
    };
    render();
  });
}

// ---------- 활동 실행 ----------
async function runActivity(key, idx) {
  const s = G.s;
  const why = activityState(s, key);
  const label = ['상순', '중순', '하순'][idx];
  if (why && key !== 'scan') {
    await UI.notice(`${label} · ${ACTIVITIES[key].name}`, [`${why}. 할 수 없어서 그냥 일찍 잤다.`]);
    s.mom = clamp(s.mom + 4);
    return;
  }
  switch (key) {
    case 'download': return actDownload(label);
    case 'burn': return actBurn(label);
    case 'scan': return actScan(label);
    case 'pc': return actPC(label);
    case 'yongsan': return actYongsan(label);
    case 'club': return actClub(label);
    case 'friend': return actFriend(label);
    case 'chores':
      s.money += 4000; s.mom = clamp(s.mom + 10);
      Sound.sfx('coin');
      return UI.notice(`${label} · 심부름·집안일`, [pick(['설거지를 하고 분리수거를 했다.', '슈퍼에 두부 심부름을 다녀왔다. 거스름돈은 내 것.', '베란다 청소를 했다. 먼지가 엄청났다.']), `용돈 +${won(4000)} · 엄마 인내심 회복`]);
    case 'study':
      s.mom = clamp(s.mom + 14);
      s.monthStudy = (s.monthStudy || 0) + 1;
      return UI.notice(`${label} · 공부`, [pick(['문제집을 풀었다. 수학은 여전히 어렵다.', '영어 단어를 외웠다. 다음 날 반은 잊었다.', '독서실에 갔다. 반은 잤다.']), '엄마 인내심이 크게 회복되었다.']);
    case 'rest':
      s.mom = clamp(s.mom + 9); s.pc.line = clamp(s.pc.line + 3);
      s.screen = 'off';
      return UI.notice(`${label} · 일찍 자기`, ['오랜만에 열한 시 전에 잤다. 꿈에서도 다운로드 창이 보였다.', '엄마 인내심 회복']);
  }
}

async function actDownload(label) {
  const s = G.s;
  s.screen = 'download';
  const speed = SPEED[s.net];
  const eff = 1 - s.pc.frag / 250;
  const budgetEst = Math.floor(speed * eff);
  const works = availableWorks(s);
  if (!works.length) return UI.notice(label, ['지금 구할 수 있는 건 이미 다 모았다. 오랜만에 푹 잤다.']);
  const ids = await UI.pickWorks(`${label} · 다운로드 목록`, works, {
    max: 3, allowCancel: false, confirmLabel: '받기 시작',
    hint: `이번 밤 예상 ${mb(budgetEst)} · 남은 하드 ${mb(hddFree(s))}`,
    info: w => {
      const e = s.col[w.id];
      const rem = remaining(s, w);
      const part = e && e.part ? ` (+${Math.round(e.part / w.size * 100)}%)` : '';
      return `${owned(s, w.id)}/${w.units}${TYPES[w.type].unit}${part} · ${TYPES[w.type].unit}당 ${mb(w.size)} · 남은 ${mb(rem * w.size - (e ? e.part : 0))}`;
    },
  });
  if (!ids || !ids.length) return;
  let budget = budgetEst;
  const notes = [];

  // 위험: 엄마가 컴퓨터를 끔
  const momP = s.mom < 45 ? (45 - s.mom) / 60 + 0.05 : 0.02;
  if (chance(momP)) budget = await evMomShutdown(budget, notes);
  // 위험: 인터넷 끊김
  else {
    const lineP = (s.net === 'modem' ? 0.16 : 0.07) + (100 - s.pc.line) / 250;
    if (chance(lineP)) budget = await evDisconnect(budget, notes);
  }

  s.screen = 'download';
  const speedTxt = s.net === 'modem' ? '4.8KB/s' : '180KB/s';
  if (s.net === 'modem') { Sound.sfx('modem', true); s.phoneNights = (s.phoneNights || 0) + 1; }
  await UI.progress('다운로드 중…', `${ids.map(id => WORK_BY_ID[id].title).join(', ')} · ${speedTxt}`, 1600, '받기 완료');
  const r = doDownload(s, ids, budget);
  Sound.sfx('done');
  s.pc.frag = clamp(s.pc.frag + (s.net === 'adsl' ? 9 : 5));
  s.pc.line = clamp(s.pc.line - (s.net === 'modem' ? 6 : 4));
  s.mom = clamp(s.mom - 9);

  const lines = r.res.map(x => {
    const ty = TYPES[x.w.type];
    const pct = x.part ? ` · 다음 ${ty.unit} ${Math.round(x.part / x.w.size * 100)}% 받음` : '';
    const done = isComplete(s, x.w) ? ' ★완결' : '';
    return `${x.w.title}: ${x.got ? '+' + x.got + ty.unit : '진행 중'}${pct}${done}`;
  });
  if (r.full) lines.push('하드디스크가 꽉 찼다! CD로 굽지 않으면 더 받을 수 없다.');
  else if (r.leftover > 50 && budget > 0) lines.push(`목록을 다 받고도 ${mb(r.leftover)}만큼 시간이 남았다.`);
  if (s.pc.frag >= 60) lines.push(`디스크 조각화 ${s.pc.frag}% — 다운로드가 점점 느려진다.`);
  await UI.notice(`${label} · 다운로드 결과`, [...notes, ...lines]);

  if (chance(s.net === 'adsl' ? 0.06 : 0.04)) await evVirus();
  else if (hddFree(s) < s.hddCap * 0.1 && !r.full) await UI.notice('경고', ['C: 드라이브의 공간이 부족합니다.', `남은 공간: ${mb(hddFree(s))}`]);
}

async function evMomShutdown(budget, notes) {
  Sound.sfx('thump');
  const s = G.s;
  const tr = computeTraits(s);
  s.screen = 'off';
  await UI.talk([
    '새벽 두 시. 다운로드 창의 진행 막대를 멍하니 보고 있는데—',
    ['mom', `${G.s.name}! 지금 몇 시야! 당장 꺼!`, 'angry'],
    '딸깍. 엄마가 멀티탭 스위치를 내렸다.',
  ]);
  Sound.sfx('off');
  const opts = [
    { label: '사과하고 잔다', value: 'sorry', note: '오늘 받은 것까지만. 엄마 인내심 회복' },
    { label: '엄마가 잠들면 몰래 다시 켠다', value: 'sneak', note: '대부분 받지만 들키면 큰일' },
  ];
  if (tr.top === 'mys' || s.flags.monitorTrick) opts.push({ label: '(탐정파) 모니터만 꺼 두고 본체는 살린다', value: 'trick', note: '조용히 계속 받는다' });
  const c = await UI.choose('어떻게 할까?', opts);
  if (c === 'sorry') { s.mom = clamp(s.mom + 12); notes.push('엄마에게 들켜 다운로드가 중간에 끊겼다.'); return budget * 0.3; }
  if (c === 'trick') { s.mom = clamp(s.mom - 4); notes.push('모니터만 끄고 본체 팬 소리를 이불로 막았다.'); return budget * 0.8; }
  if (chance(0.45)) {
    s.mom = clamp(s.mom - 25);
    s.banTurn = s.turn + 1;
    await UI.talk([['mom', '내가 못 볼 줄 알았니? 다음 달은 컴퓨터 금지야!', 'angry']]);
    notes.push('몰래 켰다가 들켰다. 다음 달은 컴퓨터 금지령.');
    return budget * 0.35;
  }
  s.mom = clamp(s.mom - 8);
  notes.push('숨죽이고 다시 켰다. 들키지 않았다.');
  return budget * 0.85;
}

async function evDisconnect(budget, notes) {
  Sound.sfx('error');
  const s = G.s;
  const tr = computeTraits(s);
  s.screen = 'desktop';
  await UI.talk([s.net === 'modem'
    ? '뚜- 뚜- 뚜-. 누군가 전화기를 들었다. 모뎀 연결이 끊겼다.'
    : '새벽, 모뎀 불빛이 깜빡이더니 꺼졌다. 인터넷이 끊겼다.']);
  const opts = [
    { label: '다시 연결해 본다', value: 'retry', note: '조금은 되찾을 수 있다' },
    { label: '포기하고 잔다', value: 'quit', note: '받은 데까지만' },
  ];
  const canResume = s.flags.resume || tr.methodKey === 'archiver';
  if (canResume) opts.unshift({ label: '이어받기로 다시 시작', value: 'resume', note: s.flags.resume ? '동호회 형에게 배운 기술' : '(아카이버) 손실 없음' });
  const c = await UI.choose('연결이 끊어졌습니다.', opts);
  s.pc.line = clamp(s.pc.line - 5);
  if (c === 'resume') { notes.push('끊긴 곳부터 이어받았다. 손실 없음.'); return budget; }
  if (c === 'retry') {
    if (s.net === 'modem') s.mom = clamp(s.mom - 5);
    notes.push('몇 번 만에 다시 연결됐다. 처음부터 다시 받은 파일도 있었다.');
    return budget * 0.7;
  }
  notes.push('인터넷이 끊겨 절반밖에 받지 못했다.');
  return budget * 0.5;
}

async function evVirus() {
  Sound.sfx('alarm');
  const s = G.s;
  s.screen = 'desktop';
  await UI.talk(['다음 날 아침, 컴퓨터를 켜자 화면에 이상한 창이 수십 개 떴다.', '…바이러스다.']);
  const opts = [
    { label: '백신으로 검사한다', value: 'scan', note: '일부 파일이 손상될 수 있다' },
    { label: '포맷한다', value: 'format', note: '하드에만 있던 파일은 전부 사라진다' },
  ];
  if (s.stats.club > 0) opts.splice(1, 0, { label: '동호회 형에게 도와 달라고 한다', value: 'hyung', note: '동호회 활동을 해서 가능' });
  const c = await UI.choose('어떻게 할까?', opts);
  if (c === 'hyung') {
    await UI.talk([['hyung', '그거 요즘 도는 웜이야. 내가 알려 주는 대로 해 봐. 레지스트리 열고…', 'smile'], '한 시간 만에 깨끗해졌다. 형은 역시 밤의 제왕이었다.']);
    s.pc.frag = clamp(s.pc.frag + 5);
    return;
  }
  if (c === 'scan') {
    await UI.progress('백신 검사', '바이러스를 찾는 중…', 1400, '치료 완료');
    const dmg = virusDamage(s);
    s.pc.frag = clamp(s.pc.frag + 10);
    return UI.notice('검사 결과', dmg ? [`바이러스는 치료했지만, 「${dmg.w.title}」 파일 ${dmg.n}${TYPES[dmg.w.type].unit}이 손상되어 지워졌다.`] : ['다행히 손상된 파일은 없었다.']);
  }
  const hddFiles = Object.values(s.col).reduce((a, e) => a + e.hdd, 0);
  const c2 = await UI.choose(`정말 포맷할까? 하드에만 있는 파일 ${hddFiles}개가 사라진다. CD로 구운 파일은 안전하다.`, [
    { label: '포맷한다', value: 'y' }, { label: '그만두고 백신 검사', value: 'n' }]);
  if (c2 === 'n') {
    const dmg = virusDamage(s);
    return UI.notice('검사 결과', dmg ? [`「${dmg.w.title}」 ${dmg.n}${TYPES[dmg.w.type].unit}이 손상되어 지워졌다.`] : ['손상된 파일은 없었다.']);
  }
  await UI.progress('FORMAT C:', 'C: 드라이브를 포맷하는 중…', 2000, '완료');
  const lost = doFormat(s);
  await UI.notice('포맷 완료', [`하드디스크가 깨끗해졌다. 사라진 파일: ${lost}개.`, '윈도우를 새로 깔았다. 바탕화면이 낯설 만큼 깨끗했다.', 'CD로 구워 둔 것들은 무사했다.']);
}

async function actBurn(label) {
  const s = G.s;
  s.screen = 'burn';
  const maxDiscs = s.pc.fastBurner ? 10 : 6;
  const plannedMB = Math.min(hddUsed(s) - partsMB(s), Math.min(s.blank, maxDiscs) * CD_MB);
  Sound.sfx('tray'); Sound.sfx('burn');
  await UI.progress('CD 굽기', `공CD에 굽는 중… (최대 ${Math.min(s.blank, maxDiscs)}장)`, 1600, '굽기 완료');
  // 드라이브 손상 위험
  const breakP = s.pc.drive < 55 ? (60 - s.pc.drive) / 70 : 0.02;
  let limit = maxDiscs;
  let broke = false;
  if (chance(breakP)) { broke = true; limit = Math.max(0, Math.floor(Math.min(s.blank, maxDiscs) / 2)); }
  const r = doBurn(s, limit);
  s.pc.drive = clamp(s.pc.drive - (s.pc.fastBurner ? 3 + r.used : 6 + r.used * 3));
  s.pc.frag = clamp(s.pc.frag + 3);
  s.mom = clamp(s.mom - 2);
  const lines = [`CD ${r.used}장을 구웠다. (${mb(r.usedMB)})`, ...r.moved.slice(0, 6).map(m => `· ${m.w.title} ${m.n}${TYPES[m.w.type].unit}`)];
  if (r.moved.length > 6) lines.push(`· 외 ${r.moved.length - 6}개 작품`);
  if (!r.used && plannedMB > 0) lines.push('파일이 커서 한 장에 들어가지 않는 것이 있었다.');
  lines.push(`매직으로 CD에 제목을 적었다. 남은 공CD ${s.blank}장 · 드라이브 상태 ${s.pc.drive}%`);
  await UI.notice(`${label} · 굽기 결과`, lines);
  if (broke) await evDriveBroken();
}

async function evDriveBroken() {
  Sound.sfx('error');
  const s = G.s;
  const tr = computeTraits(s);
  s.pc.driveBroken = true;
  await UI.talk(['위이잉- 탁. 탁. 탁.', 'CD 트레이가 열리지도 닫히지도 않는다. 굽던 CD는 은빛 코스터가 되었다.', 'CD 드라이브가 고장 났다.']);
  const opts = [
    { label: '아빠에게 말한다', value: 'dad', note: '고쳐 줄지도 모른다' },
    { label: '나중에 용산에서 고친다', value: 'later', note: `수리비 ${won(15000)}` },
  ];
  if (tr.top === 'sf') opts.unshift({ label: '(메카·SF광) 직접 분해해서 고쳐 본다', value: 'diy', note: '성공할 수도, 아닐 수도' });
  const c = await UI.choose('어떻게 할까?', opts);
  if (c === 'diy') {
    if (chance(0.6)) { s.pc.driveBroken = false; s.pc.drive = 60; return UI.talk(['드라이버로 나사를 풀고, 렌즈를 면봉으로 닦고, 벨트를 다시 걸었다.', '…돌아간다! 나는 천재일지도 모른다.']); }
    return UI.talk(['나사가 하나 남았다. 어디 거였는지 모르겠다. 드라이브는 여전히 고장이다.']);
  }
  if (c === 'dad') {
    if (s.mom >= 50 && chance(0.7)) { s.pc.driveBroken = false; s.pc.drive = 100; return UI.talk([['dad', '드라이브 하나 새로 사 왔다. 대신 엄마 말 잘 들어라.', 'smile'], '새 드라이브다!']); }
    return UI.talk([['dad', '요즘 성적도 그런데 CD만 굽고 있었냐? 네 용돈으로 고쳐.', 'angry']]);
  }
  return UI.talk(['다음에 용산에 가면 고쳐야겠다.']);
}

async function actScan(label) {
  const s = G.s;
  if (!s.pc.scanner) return UI.notice(label, ['스캐너가 없다.']);
  if (s.money < MANGA_PRICE) { await UI.notice(`${label} · 만화책 스캔`, ['서점에 갔지만 만화책 살 돈이 없었다.']); return; }
  const mangas = availableWorks(s).filter(w => w.type === 'manga');
  if (!mangas.length) return UI.notice(label, ['서점에 있는 만화는 이미 다 스캔했다.']);
  const ids = await UI.pickWorks(`${label} · 어떤 만화책을 살까?`, mangas, {
    max: 1, allowCancel: false, confirmLabel: '서점으로',
    info: w => `${owned(s, w.id)}/${w.units}권 · 권당 ${won(MANGA_PRICE)}`,
  });
  if (!ids || !ids.length) return;
  const w = WORK_BY_ID[ids[0]];
  const maxBuy = Math.min(6, remaining(s, w), Math.floor(s.money / MANGA_PRICE), Math.floor(hddFree(s) / w.size));
  if (maxBuy <= 0) return UI.notice(label, [hddFree(s) < w.size ? '하드에 스캔본을 저장할 공간이 없다.' : '살 수 있는 권이 없다.']);
  const opts = [1, 2, 3, 4, 6].filter(n => n <= maxBuy).map(n => ({ label: `${n}권 사기`, value: n, note: won(n * MANGA_PRICE) }));
  if (!opts.some(o => o.value === maxBuy)) opts.push({ label: `${maxBuy}권 사기`, value: maxBuy, note: won(maxBuy * MANGA_PRICE) });
  const n = await UI.choose(`「${w.title}」 몇 권을 살까? (한 번에 최대 6권 스캔)`, opts);
  s.money -= n * MANGA_PRICE;
  s.screen = 'desktop';
  Sound.sfx('scanner');
  await UI.progress('스캔 중…', `「${w.title}」 한 장, 한 장… (300dpi)`, 1800, '스캔 완료');
  addUnits(s, w, n, 'hdd', 'scan');
  s.pc.frag = clamp(s.pc.frag + 3);
  await UI.notice(`${label} · 스캔 결과`, [
    `「${w.title}」 ${n}권을 스캔해 JPG로 저장했다. (${mb(n * w.size)})`,
    pick(['책을 너무 세게 눌러서 책등이 갈라졌다. 그래도 스캔본은 완벽하다.', '삐뚤어진 페이지는 다시 스캔했다. 세 번이나.', '스캐너 불빛이 지나갈 때마다 방이 환해졌다.']),
    isComplete(s, w) ? '★ 완결까지 모았다!' : `${owned(s, w.id)}/${w.units}권`,
  ]);
}

async function actPC(label) {
  const s = G.s;
  const before = s.pc.frag;
  await UI.runDefrag(Math.max(before, 20));
  s.pc.frag = 0;
  s.pc.line = clamp(s.pc.line + 20);
  s.stats.defrags++;
  const lines = [`디스크 조각 모음 완료. 조각화 ${before}% → 0%`, '스캔디스크도 돌렸다. 하드디스크가 상쾌하게 돌아간다.', '모뎀과 전화선 연결도 점검했다. (회선 안정성 회복)'];
  if (s.pc.burner && !s.pc.driveBroken && s.pc.drive < 100) { s.pc.drive = clamp(s.pc.drive + 10); lines.push('CD 드라이브 먼지도 털었다. (+10%)'); }
  await UI.notice(`${label} · 컴퓨터 정비`, lines);
}

async function actYongsan(label) {
  const s = G.s;
  s.money -= PRICES.yongsan;
  await UI.talk([
    `지하철 1호선, 용산역. 육교를 건너면 전자상가의 냄새가 났다. (차비 ${won(PRICES.yongsan)})`,
    ['owner', pick(['어이 학생, 또 왔네? 오늘은 뭐 찾아?', '공CD 들어왔어. 케이크 통으로 사 가.', '학생, 요즘 뭐 굽길래 그렇게 자주 와?']), 'smile'],
  ]);
  for (;;) {
    const opts = SHOP.filter(it => it.avail(s)).map(it => ({ label: `${it.name}  ${won(shopPrice(it, s))}`, value: it.id, note: shopDesc(it, s), disabled: s.money < shopPrice(it, s) }));
    const usedPool = availableWorks(s).filter(w => w.type === 'movie' || w.type === 'game');
    if (usedPool.length) opts.push({ label: `중고 CD 뽑기  ${won(6000)}`, value: 'used', note: '영화나 게임이 구워진 중고 CD 한 장', disabled: s.money < 6000 });
    opts.push({ label: '돌아간다', value: 'back' });
    const c = await UI.choose(`가진 돈 ${won(s.money)}`, opts, 'owner');
    if (c === 'back') break;
    if (c === 'used') {
      s.money -= 6000;
      const w = pick(usedPool);
      const n = receiveDisc(s, w, 'trade');
      Sound.sfx('item');
      await UI.talk([['owner', '이건 덤이야. 잘 돌아갈 거야, 아마.', 'smile'], `중고 CD에는 「${w.title}」이 들어 있었다! (+${n}${TYPES[w.type].unit})`]);
      continue;
    }
    const it = SHOP.find(x => x.id === c);
    s.money -= shopPrice(it, s);
    Sound.sfx('coin');
    if (c === 'cd10') s.blank += 10;
    if (c === 'cd30') s.blank += 30;
    if (c === 'lens') s.pc.drive = clamp(s.pc.drive + 30);
    if (c === 'repair') { s.pc.driveBroken = false; s.pc.drive = 90; }
    if (c === 'scanner') s.pc.scanner = true;
    if (c === 'usb') s.pc.usb = true;
    if (c === 'burner24') { s.pc.fastBurner = true; s.pc.driveBroken = false; s.pc.drive = 100; }
    if (c === 'hdd20') { s.pc.hdd20 = true; s.hddCap += 12000; }
    if (c === 'hdd40') { s.pc.hdd40 = true; s.hddCap += 25000; }
    if (c === 'hdd20' || c === 'hdd40') { Sound.sfx('startup'); await UI.notice('하드디스크 추가', [`${it.name}를 직접 달았다. 점퍼 설정 때문에 두 번 다시 열었다.`, `저장 공간: ${mb(s.hddCap)}`]); continue; }
    await UI.say('owner', pick(['고마워, 학생. 또 와.', '좋은 선택이야.', '그건 내가 보장하지.']), 'smile');
  }
}

async function actClub(label) {
  const s = G.s;
  s.stats.club++;
  const greet = [
    '동호회 게시판에 접속했다. 새벽에도 사람들이 바글바글하다.',
    s.stats.club === 1 ? ['hyung', '어, 신입이네? 반가워. 여기 규칙은 하나야. 받았으면 나눠라.', 'smile'] : ['hyung', pick(['왔냐? 오늘 좋은 거 들어왔다.', '요즘 뭐 모으냐? 형한테 말해 봐.', '자막 새로 나온 거 봤냐?']), 'smile'],
  ];
  await UI.talk(greet);
  if (s.stats.club === 2 && !s.flags.resume) {
    s.flags.resume = true;
    await UI.talk([['hyung', '너 아직도 끊기면 처음부터 받냐? 이어받기 기능 있는 프로그램 써. 링크 줄게.', 'smile']]);
    await UI.notice('새 기술', ['"이어받기"를 배웠다.', '인터넷이 끊겨도 손실 없이 다시 받을 수 있다.']);
  }
  const pool = availableWorks(s);
  if (!pool.length) return UI.notice(label, ['이미 모든 걸 가지고 있었다. 형이 혀를 찼다.']);
  const offers = [];
  const shuffled = pool.slice().sort(() => Math.random() - 0.5);
  for (const w of shuffled) { if (offers.length >= 3) break; if (!offers.some(o => o.type === w.type) || offers.length >= 2) offers.push(w); }
  const ids = await UI.pickWorks(`${label} · 교환할 CD 고르기`, offers, {
    max: 1, allowCancel: false, confirmLabel: '교환하기',
    hint: 'CD 한 장 분량을 받아 바로 CD로 보관합니다',
    info: w => `${owned(s, w.id)}/${w.units}${TYPES[w.type].unit} · CD 1장에 최대 ${Math.floor(CD_MB / w.size)}${TYPES[w.type].unit}`,
  });
  if (!ids || !ids.length) return;
  const w = WORK_BY_ID[ids[0]];
  s.blank--; s.money -= PRICES.club;
  const n = receiveDisc(s, w, 'trade');
  Sound.sfx('item');
  const mine = ownedWorks(s).length;
  if (mine > 1) s.method.share += 3;
  await UI.notice(`${label} · 교환 결과`, [
    `내 CD를 구워 우편으로 보내고, 형의 CD를 받았다. (공CD 1장, 우표 ${won(PRICES.club)})`,
    `「${w.title}」 +${n}${TYPES[w.type].unit} (CD 보관)`,
    isComplete(s, w) ? '★ 완결!' : '',
  ].filter(Boolean));
  if (chance(0.06)) await evVirus();
}

async function actFriend(label) {
  const s = G.s;
  s.mom = clamp(s.mom - 2);
  if (s.money < PRICES.friend) {
    s.friend = clamp(s.friend + 5);
    return UI.talk(['주머니에 돈이 없어서 운동장에서 공만 찼다.', ['friend', '다음엔 PC방 가자. 너 돈 생기면.', 'smile']]);
  }
  s.money -= PRICES.friend;
  s.friend = clamp(s.friend + 12);
  await UI.talk([pick([
    '태식이와 PC방에 갔다. 스타크래프트 한 판에 한 시간이 순식간이었다.',
    '오락실에서 철권을 했다. 태식이는 늘 같은 캐릭터만 한다.',
    '학교 끝나고 떡볶이를 먹고, 문방구 앞 게임기에서 놀았다.',
  ])]);
  if (chance(0.5) && ownedWorks(s).length) {
    await UI.say('friend', pick(['야, 너 그거 있지? 하나만 구워 주라. 형이 보고 싶대.', '그거 나도 좀 보자. CD로 구워 줄 수 있냐?']), 'smile');
    const opts = [
      { label: '구워 준다', value: 'burn', note: s.blank > 0 ? '공CD 1장' : '공CD가 없다', disabled: s.blank <= 0 },
      { label: '내 CD를 빌려준다', value: 'lend', note: '돌아올지는 모른다' },
      { label: '거절한다', value: 'no' },
    ];
    const c = await UI.choose('', opts);
    if (c === 'burn') { s.blank--; s.method.share += 8; s.friend = clamp(s.friend + 8); s.stats.gifts++; return UI.say('friend', '역시 너밖에 없다! 다음에 떡볶이 쏜다.', 'smile'); }
    if (c === 'lend') {
      s.method.share += 6; s.friend = clamp(s.friend + 5);
      const cdWorks = ownedWorks(s).filter(w => s.col[w.id].cd > 0);
      if (cdWorks.length && chance(0.3)) {
        const w = pick(cdWorks); s.col[w.id].cd--; s.stats.lost++;
        return UI.talk([['friend', '야… 미안. 그 CD 동생이 긁어 놨어.', 'sad'], `「${w.title}」 CD 한 장이 돌아오지 않았다.`]);
      }
      return UI.say('friend', '다 보면 바로 돌려줄게!', 'smile');
    }
    s.friend = clamp(s.friend - 6);
    return UI.say('friend', '치사하다, 진짜.', 'angry');
  }
  if (chance(0.35)) {
    const pool = availableWorks(s).filter(w => w.type === 'game' || w.type === 'music');
    if (pool.length) {
      const w = pick(pool);
      const n = receiveDisc(s, w, 'trade');
      return UI.talk([['friend', '이거 우리 형이 구운 건데, 너 가져.', 'smile'], `「${w.title}」 CD를 받았다! (+${n}${TYPES[w.type].unit})`]);
    }
  }
}

// ---------- 월말 정리 (회복 이벤트 포함) ----------
async function monthEndUpkeep() {
  const s = G.s;
  s.mom = clamp(s.mom + 4);
  // 모뎀 전화요금 몫
  if (s.phoneNights > 0) {
    const bill = s.phoneNights * PRICES.phone;
    const paid = Math.min(bill, s.money);
    s.money -= paid;
    if (paid < bill) s.mom = clamp(s.mom - 8);
    await UI.notice('전화요금', [`이번 달 밤새 모뎀을 쓴 날: ${s.phoneNights}일`, `엄마가 전화요금 몫으로 ${won(paid)}을 가져갔다.${paid < bill ? ' (모자란 만큼 잔소리를 들었다)' : ''}`]);
    s.phoneNights = 0;
  }
  // 시험 성적 → 용돈
  const dm = dateOf(s.turn).m;
  if (EXAM_MONTHS.includes(dm)) await examResult();
  // 엄마 인내심 바닥 → 금지령
  if (s.mom < 15 && s.banTurn !== s.turn + 1) {
    s.banTurn = s.turn + 1;
    await UI.talk([['mom', '너 요즘 컴퓨터밖에 안 하지? 다음 달은 컴퓨터 금지야. 공부해!', 'angry']]);
    await UI.notice('컴퓨터 금지령', ['다음 달에는 다운로드, 굽기, 정비를 할 수 없다.', '엄마 인내심: 공부, 심부름, 일찍 자기로 회복할 수 있습니다.']);
    s.mom = clamp(s.mom + 20);
    return;
  }
  // 회복 이벤트 (월 1회까지)
  if (s.net === 'adsl' && s.pc.line < 45 && chance(0.5)) {
    s.pc.line = 100;
    return UI.talk(['인터넷이 자꾸 끊긴다고 신고했더니, 기사님이 와서 단자함을 고쳐 주었다.', '회선 안정성이 완전히 회복되었다.']);
  }
  if (s.pc.burner && !s.pc.driveBroken && s.pc.drive < 40 && chance(0.25)) {
    s.pc.drive = clamp(s.pc.drive + 40);
    return UI.talk([['dad', '컴퓨터 가게 지나다가 렌즈 클리너라는 게 있길래 사 왔다.', 'smile'], 'CD 드라이브 상태가 좋아졌다.']);
  }
  if (s.pc.frag >= 70 && chance(0.35)) {
    await UI.talk([['hyung', '너 요즘 다운 속도 느리다며? 조각 모음 한 번 돌려. 밤에 켜 두고 자.', 'smile']]);
    const c = await UI.choose('오늘 밤 조각 모음을 돌려 둘까?', [{ label: '돌려 두고 잔다', value: 'y' }, { label: '귀찮다', value: 'n' }]);
    if (c === 'y') { await UI.runDefrag(s.pc.frag); s.pc.frag = 0; s.stats.defrags++; return UI.notice('디스크 조각 모음', ['조각 모음이 끝났다. 하드디스크가 다시 빨라졌다.']); }
  }
}

async function examResult() {
  const s = G.s;
  const score = (s.monthStudy || 0) + (s.mom >= 60 ? 1 : 0);
  if (score >= 3) {
    s.allowanceMod = 2000; s.money += 5000; s.mom = clamp(s.mom + 10);
    Sound.sfx('coin');
    await UI.talk([['dad', '성적이 올랐네? 잘했다. 이건 보너스.', 'smile']]);
    return UI.notice('성적표', ['성적이 올랐다!', `보너스 ${won(5000)} · 다음 시험까지 용돈 +${won(2000)}`]);
  }
  if (score === 2) {
    s.allowanceMod = 0;
    return UI.notice('성적표', ['성적은 평소와 비슷했다.', '용돈은 그대로.']);
  }
  s.allowanceMod = -2000; s.mom = clamp(s.mom - 10);
  await UI.talk([['mom', '이 점수가 뭐니? 컴퓨터만 하더니. 다음 시험까지 용돈 깎는다.', 'angry']]);
  return UI.notice('성적표', ['성적이 떨어졌다.', `다음 시험까지 용돈 -${won(2000)}`, '시험 기간에 공부를 더 하거나, 엄마 인내심을 60 이상으로 유지하면 성적이 오릅니다.']);
}

// ---------- 컬렉션 ----------
function collectionView() {
  return new Promise(resolve => {
    const s = G.s;
    let tab = 'anime';
    const render = () => {
      const tabs = h('div', { class: 'tabs' }, ...Object.entries(TYPES).map(([k, v]) =>
        h('button', { class: 'tab' + (tab === k ? ' on' : ''), onclick: () => { tab = k; render(); } }, v.folder)));
      const list = h('div', { class: 'list' });
      const all = WORKS.filter(w => w.type === tab);
      let hidden = 0;
      for (const w of all) {
        const n = owned(s, w.id);
        const e = s.col[w.id];
        if (!n && !s.seen[w.id] && !Meta.data.seen[w.id]) { hidden++; continue; }
        const where = e ? [e.hdd ? `HDD ${e.hdd}` : '', e.cd ? `CD ${e.cd}` : ''].filter(Boolean).join(' · ') : '';
        list.append(UI.workRow(w, n ? `${n}/${w.units}${TYPES[w.type].unit} ${where}` : '없음 (본 적 있음)', { dim: !n, noNew: true }));
      }
      if (hidden) list.append(h('p', { class: 'empty' }, `??? 아직 만나지 못한 작품 ${hidden}개`));
      const tr = computeTraits(s);
      UI.setPanel(UI.win(`C:\\내 컬렉션\\${TYPES[tab].folder}\\`, h('div', { class: 'picker' },
        tabs, list, h('div', { class: 'sel-info' }, `${tr.ownedCount}개 작품 · 완결 ${tr.completed}개 · 구운 CD ${s.stats.discs}장`)),
      [h('button', { class: 'btn primary', onclick: resolve }, '닫기')]));
      document.getElementById('panel').className = 'tall';
    };
    render();
  });
}

// ---------- 내 컴퓨터 ----------
function bar(label, v, max = 100, color, text) {
  return h('div', { class: 'stat' },
    h('span', { class: 'lbl' }, label),
    h('div', { class: 'bar' }, h('i', { style: `width:${Math.min(100, v / max * 100)}%;${color ? 'background:' + color : ''}` })),
    h('span', { class: 'num' }, text != null ? text : Math.round(v)));
}

function computerView() {
  return new Promise(resolve => {
    const s = G.s;
    const tr = computeTraits(s);
    const gMax = Math.max(1, ...Object.values(tr.genre));
    const mMax = Math.max(1, ...Object.values(tr.method));
    const used = hddUsed(s);
    const body = h('div', { class: 'props' },
      h('h3', {}, '시스템'),
      bar('C: 드라이브', used, s.hddCap, used / s.hddCap > 0.9 ? '#c03030' : null, `${mb(used)}/${mb(s.hddCap)}`),
      bar('조각화', s.pc.frag, 100, '#c03030', s.pc.frag + '%'),
      bar('회선 안정성', s.pc.line, 100, null, s.pc.line + '%'),
      s.pc.burner ? bar('CD 드라이브', s.pc.driveBroken ? 0 : s.pc.drive, 100, null, s.pc.driveBroken ? '고장' : s.pc.drive + '%') : null,
      h('p', { class: 'dim' }, `인터넷: ${s.net === 'adsl' ? 'ADSL' : '56k 모뎀'} · 장비: ${['CD 버너', '스캐너', 'USB 메모리'].filter((_, i) => [s.pc.burner, s.pc.scanner, s.pc.usb][i]).join(', ') || '없음'} · 공CD ${s.blank}장`),
      bar('엄마 인내심', s.mom, 100, s.mom < 30 ? '#c03030' : '#3a8a3a', s.mom),
      h('h3', {}, `성향 — ${traitLabel(tr)}`),
      ...Object.entries(GENRES).map(([k, g]) => bar(g.trait, tr.genre[k], gMax, g.color, '')),
      h('h3', {}, '수집 매체 (모을 수 있는 것 중 몇 %)'),
      ...Object.entries(MEDIA_NAMES).map(([k, n]) => bar(n, tr.media[k] * 100, 100, '#3a7a9a', Math.round(tr.media[k] * 100) + '%')),
      h('h3', {}, '수집 방식'),
      ...Object.entries(METHOD_NAMES).map(([k, n]) => bar(n, tr.method[k], mMax, '#606080', '')),
    );
    UI.setPanel(UI.win('내 컴퓨터 - 속성', body, [h('button', { class: 'btn primary', onclick: resolve }, '닫기')]));
    document.getElementById('panel').className = 'tall';
  });
}

// ---------- 메뉴 / 기록 ----------
async function menu() {
  const c = await UI.choose('메뉴', [
    { label: '계속하기', value: 'back' },
    { label: '도움말', value: 'help' },
    { label: '저장하고 타이틀로', value: 'quit', note: '이번 달 처음부터 이어집니다' },
  ]);
  if (c === 'help') {
    await UI.notice('도움말', [
      '· 매달 일정 3칸을 고릅니다. 시험 기간(4·7·10·12월)엔 첫 칸이 공부로 고정돼요.',
      '· 다운로드는 인터넷 속도만큼 받고, 하드 공간이 차면 멈춥니다. 다 못 받은 파일은 다음에 이어서 받아요.',
      '· CD 굽기: 하드의 파일을 CD로 옮겨 공간을 비우고, 포맷에도 안전하게 지킵니다.',
      '· 무엇을(장르) 어떻게(다운로드·스캔·교환·완결) 모았는지가 엔딩을 정합니다.',
      '· 엄마 인내심이 낮으면 밤에 컴퓨터를 꺼 버립니다.',
      '· 돈: 공CD 값은 해마다 내려갑니다. 친구·동호회·용산에는 돈이 들고, 모뎀 시절엔 밤샌 날마다 전화요금 몫을 냅니다.',
      '· 시험 달에 공부를 2번 이상 하거나 엄마 인내심이 60 이상이면 성적이 올라 용돈이 늘어납니다.',
      '· 하드 증설, 고속 CD-RW, 스캐너, USB는 용산에서 돈을 모아 삽니다.',
      '· 조각화가 높으면 다운로드가 느려집니다. 컴퓨터 정비로 조각 모음을 하세요.',
    ]);
    return 'back';
  }
  if (c === 'quit') { Save.write(G.s); return 'quit'; }
  return 'back';
}

function records() {
  const m = Meta.load();
  const rows = Object.entries(Story.ENDINGS).map(([k, e]) => h('div', { class: 'rec' + (m.endings[k] ? '' : ' dim') }, m.endings[k] ? '★ ' + e.title : '??? 미발견'));
  const rrows = Object.entries(Story.ROMANCE).map(([k, e]) => h('div', { class: 'rec' + (m.romance[k] ? '' : ' dim') }, m.romance[k] ? '♥ ' + e.title : '??? 미발견'));
  const seen = Object.keys(m.seen).length;
  UI.setPanel(UI.win('기록', h('div', { class: 'props' },
    h('h3', {}, `진로 엔딩 ${Object.keys(m.endings).length}/${rows.length}`), ...rows,
    h('h3', {}, `에필로그 ${Object.keys(m.romance).length}/${rrows.length}`), ...rrows,
    h('h3', {}, '수집 도감'), h('p', {}, `만난 작품 ${seen}/${WORKS.length}`),
    h('p', { class: 'dim' }, `플레이 횟수 ${m.plays}`),
  ), [h('button', { class: 'btn primary', onclick: title }, '닫기')]));
  document.getElementById('panel').className = 'tall';
}

boot();
