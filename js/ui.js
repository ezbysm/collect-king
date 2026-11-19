// 수집왕 — 화면 구성 요소 (대화창, 선택지, 창, 목록)
'use strict';

const UI = (() => {
  let panel, statusEl, scene, sctx;
  let sceneMode = 'room';    // room | title | defrag | future
  let defragState = null;
  let advanceKey = null;
  let lastLine = null;   // 선택지에 앞 대사를 함께 보여 주기 위해

  function h(tag, props = {}, ...kids) {
    const e = document.createElement(tag);
    for (const k in props) {
      const v = props[k];
      if (v == null || v === false) continue;
      if (k === 'class') e.className = v;
      else if (k.startsWith('on')) e.addEventListener(k.slice(2), v);
      else if (k === 'html') e.innerHTML = v;
      else e.setAttribute(k, v === true ? '' : v);
    }
    for (const c of kids.flat()) if (c != null && c !== false) e.append(c.nodeType ? c : document.createTextNode(String(c)));
    return e;
  }

  function init() {
    panel = document.getElementById('panel');
    statusEl = document.getElementById('status');
    scene = document.getElementById('scene');
    sctx = scene.getContext('2d');
    window.addEventListener('resize', fitScene);
    fitScene();
    // 모바일은 첫 터치 후에 소리를 낼 수 있다
    const unlock = () => Sound.unlock();
    document.addEventListener('pointerdown', unlock);
    document.addEventListener('keydown', unlock);
    document.addEventListener('click', e => { if (e.target.closest('button') && !e.target.closest('#mute')) Sound.sfx('click'); });
    const muteBtn = document.getElementById('mute');
    const paintMute = () => { muteBtn.textContent = Sound.isMuted() ? '♪ 꺼짐' : '♪ 켜짐'; muteBtn.classList.toggle('off', Sound.isMuted()); };
    muteBtn.addEventListener('click', () => { Sound.unlock(); Sound.setMuted(!Sound.isMuted()); paintMute(); });
    paintMute();
    document.addEventListener('keydown', e => {
      if ((e.key === 'Enter' || e.key === ' ') && advanceKey) { e.preventDefault(); advanceKey(); }
    });
    const loop = t => { renderScene(t); requestAnimationFrame(loop); };
    requestAnimationFrame(loop);
  }

  function fitScene() {
    const wrap = document.getElementById('sceneWrap');
    const avail = wrap.clientWidth;
    const maxH = window.innerHeight * 0.4;
    let scale = Math.floor(Math.min(avail / Art.W, maxH / Art.H));
    let w = scale >= 2 ? Art.W * scale : Math.min(avail, maxH * Art.W / Art.H);
    scene.style.width = w + 'px';
    scene.style.height = (w * Art.H / Art.W) + 'px';
  }

  let lastDraw = 0;
  // 화면 아래쪽 배경화면: 계절·장면에 따라 바뀐다
  let wallKind = '';
  function updateWallpaper() {
    const kind = sceneMode === 'title' || !G.s ? 'title' : sceneMode === 'future' ? 'future'
      : Art.seasonOf(dateOf(Math.min(G.s.turn, TOTAL_TURNS - 1)).m);
    if (kind === wallKind) return;
    wallKind = kind;
    document.getElementById('app').style.backgroundImage = `url(${Art.wallpaper(kind)})`;
  }

  function renderScene(t) {
    updateWallpaper();
    if (t - lastDraw < 66) return; // 15fps면 충분
    lastDraw = t;
    if (sceneMode === 'title') Art.drawTitle(sctx, t);
    else if (sceneMode === 'defrag' && defragState) Art.drawDefrag(sctx, defragState.df, defragState.p);
    else if (G.s) Art.drawRoom(sctx, G.s, t, { future: sceneMode === 'future' });
  }
  function setScene(mode) { sceneMode = mode; lastDraw = 0; }

  function clear() { panel.innerHTML = ''; panel.scrollTop = 0; panel.className = ''; }
  function setPanel(...nodes) { clear(); panel.append(...nodes); }

  // ---------- 상태 표시줄 ----------
  function updateStatus() {
    const s = G.s;
    if (!s || sceneMode === 'title') { statusEl.innerHTML = ''; statusEl.classList.add('hidden'); return; }
    statusEl.classList.remove('hidden');
    const d = dateOf(Math.min(s.turn, TOTAL_TURNS - 1));
    const used = hddUsed(s), pct = Math.min(100, used / s.hddCap * 100);
    statusEl.innerHTML = '';
    statusEl.append(
      h('div', { class: 'st-date' }, `${d.y}.${String(d.m).padStart(2, '0')}`, h('small', {}, ` 중${d.grade}`)),
      h('div', { class: 'st-money' }, won(s.money)),
      h('div', { class: 'st-hdd' },
        h('span', { class: 'lbl' }, 'C:'),
        h('div', { class: 'bar' + (pct > 90 ? ' warn' : '') }, h('i', { style: `width:${pct}%` })),
        h('span', { class: 'num' }, `${mb(used)}/${mb(s.hddCap)}`)),
      h('div', { class: 'st-cd' }, `CD ${s.blank}`),
    );
  }

  // ---------- 대화 ----------
  function nameOf(who) {
    if (who === 'her') return G.s.heroine.name;
    if (who === 'me') return G.s.name;
    return (CAST[who] && CAST[who].name) || '';
  }

  function say(who, text, expr = 'normal') {
    lastLine = { who, text, expr };
    if (who === 'her') Sound.bgm('her');
    return new Promise(resolve => {
      const c = CAST[who] || {};
      const box = h('div', { class: 'dialog' + (c.face ? ' has-face' : '') + (who === 'narr' ? ' narr' : '') });
      if (c.face) {
        const cv = h('canvas', { width: 40, height: 40, class: 'face' });
        Art.drawFace(cv, c.face, expr);
        box.append(cv);
      }
      const tEl = h('div', { class: 'dlg-text' });
      const name = nameOf(who);
      box.append(h('div', { class: 'dlg-body' }, name ? h('div', { class: 'dlg-name' }, name) : null, tEl));
      const wrap = h('div', { class: 'dlg-wrap' }, box, h('div', { class: 'dlg-next' }, '▼ 탭하여 계속'));
      setPanel(wrap);
      updateStatus();
      let i = 0, done = false;
      const timer = setInterval(() => {
        i += 1;
        tEl.textContent = text.slice(0, i);
        if (text[i - 1] && text[i - 1] !== ' ' && i % 2) Sound.sfx('blip');
        if (i >= text.length) { clearInterval(timer); done = true; }
      }, 22);
      const adv = () => {
        if (!done) { clearInterval(timer); tEl.textContent = text; done = true; return; }
        advanceKey = null;
        wrap.removeEventListener('click', adv);
        resolve();
      };
      advanceKey = adv;
      wrap.addEventListener('click', adv);
    });
  }

  async function talk(lines) {
    for (const l of lines) {
      if (!l) continue;
      if (typeof l === 'string') await say('narr', l);
      else await say(l[0], l[1], l[2]);
    }
  }

  // 선택지: opts = [{label, value, note, disabled}]
  function choose(prompt, opts, who = null, expr = 'normal') {
    return new Promise(resolve => {
      advanceKey = null;
      const nodes = [];
      if (!prompt && lastLine) { prompt = lastLine.text; who = lastLine.who === 'narr' ? null : lastLine.who; expr = lastLine.expr; }
      lastLine = null;
      if (prompt) {
        const c = who ? CAST[who] : null;
        const box = h('div', { class: 'dialog small' + (c && c.face ? ' has-face' : '') });
        if (c && c.face) { const cv = h('canvas', { width: 40, height: 40, class: 'face' }); Art.drawFace(cv, c.face, expr); box.append(cv); }
        box.append(h('div', { class: 'dlg-body' }, who ? h('div', { class: 'dlg-name' }, nameOf(who)) : null, h('div', { class: 'dlg-text' }, prompt)));
        nodes.push(box);
      }
      const list = h('div', { class: 'choices' });
      for (const o of opts) {
        list.append(h('button', {
          class: 'btn choice', disabled: o.disabled,
          onclick: () => resolve(o.value),
        }, h('span', {}, o.label), o.note ? h('small', {}, o.note) : null));
      }
      nodes.push(list);
      setPanel(...nodes);
      updateStatus();
    });
  }

  // ---------- 윈도우 98 창 ----------
  function win(title, body, buttons = []) {
    return h('div', { class: 'win' },
      h('div', { class: 'win-title' }, h('span', {}, title), h('i', { class: 'x' }, '×')),
      h('div', { class: 'win-body' }, body),
      buttons.length ? h('div', { class: 'win-btns' }, buttons) : null);
  }

  function notice(title, lines, btnLabel = '확인') {
    Sound.sfx('notice');
    return new Promise(resolve => {
      const body = h('div', { class: 'notice' }, ...lines.map(l => (typeof l === 'string' ? h('p', {}, l) : l)));
      const ok = h('button', { class: 'btn primary', onclick: () => { advanceKey = null; resolve(); } }, btnLabel);
      setPanel(win(title, body, [ok]));
      advanceKey = () => ok.click();
      updateStatus();
    });
  }

  // 진행 막대 연출
  function progress(title, label, ms, finalText) {
    return new Promise(resolve => {
      advanceKey = null;
      const fill = h('i', {});
      const pct = h('span', {}, '0%');
      setPanel(win(title, h('div', { class: 'prog' }, h('p', {}, label), h('div', { class: 'bar big' }, fill), pct)));
      const t0 = performance.now();
      const step = now => {
        const p = Math.min(1, (now - t0) / ms);
        fill.style.width = (p * 100) + '%';
        pct.textContent = p < 1 ? Math.floor(p * 100) + '%' : (finalText || '완료');
        if (p < 1) requestAnimationFrame(step); else setTimeout(resolve, 250);
      };
      requestAnimationFrame(step);
    });
  }

  function runDefrag(frag, ms = 2600) {
    Sound.sfx('hdd', ms / 1000);
    return new Promise(resolve => {
      defragState = { df: Art.makeDefrag(frag), p: 0 };
      const prev = sceneMode;
      setScene('defrag');
      setPanel(win('디스크 조각 모음', h('div', { class: 'prog' }, h('p', {}, '밤새 조각 모음을 돌려 두고 잠들었다…'), h('p', { class: 'dim' }, '빨간 칸이 조각난 파일, 파란 칸이 정리된 파일입니다.'))));
      const t0 = performance.now();
      const step = now => {
        defragState.p = Math.min(1, (now - t0) / ms);
        if (defragState.p < 1) requestAnimationFrame(step);
        else setTimeout(() => { setScene(prev); defragState = null; resolve(); }, 500);
      };
      requestAnimationFrame(step);
    });
  }

  // ---------- 작품 목록 ----------
  const TABS = [['all', '전체'], ...Object.entries(TYPES).map(([k, v]) => [k, v.folder])];

  function workRow(w, info, opts = {}) {
    const s = G.s;
    const ic = h('canvas', { width: 16, height: 16, class: 'icon' });
    Art.drawIcon(ic, w.type, isComplete(s, w));
    const tags = w.genres.map(g => h('em', { class: 'tag', style: `--c:${GENRES[g].color}` }, GENRES[g].name));
    return h('div', { class: 'row' + (opts.sel ? ' sel' : '') + (opts.dim ? ' dim' : ''), onclick: opts.onclick },
      ic,
      h('div', { class: 'row-main' },
        h('div', { class: 'row-title' }, w.title, isNew(s, w) && !opts.noNew ? h('b', { class: 'new' }, 'NEW') : null),
        h('div', { class: 'row-sub' }, ...tags, h('span', {}, info))),
      opts.badge ? h('div', { class: 'badge' }, opts.badge) : null);
  }

  // 여러 개(최대 max) 고르기. 반환: id 배열 (취소 시 null)
  function pickWorks(title, works, { max = 1, info, hint, allowCancel = true, confirmLabel = '확인' } = {}) {
    return new Promise(resolve => {
      advanceKey = null;
      let tab = 'all';
      const sel = [];
      const list = h('div', { class: 'list' });
      const tabsEl = h('div', { class: 'tabs' });
      const okBtn = h('button', { class: 'btn primary', onclick: () => resolve(sel.slice()) }, confirmLabel);
      const cancel = allowCancel ? h('button', { class: 'btn', onclick: () => resolve(null) }, '취소') : null;
      const selInfo = h('div', { class: 'sel-info' });
      function render() {
        tabsEl.innerHTML = '';
        for (const [k, label] of TABS) {
          const n = works.filter(w => k === 'all' || w.type === k).length;
          if (k !== 'all' && !n) continue;
          tabsEl.append(h('button', { class: 'tab' + (tab === k ? ' on' : ''), onclick: () => { tab = k; render(); } }, label));
        }
        list.innerHTML = '';
        const shown = works.filter(w => tab === 'all' || w.type === tab)
          .sort((a, b) => (isNew(G.s, b) - isNew(G.s, a)) || (b.appear - a.appear));
        for (const w of shown) {
          const i = sel.indexOf(w.id);
          list.append(workRow(w, info ? info(w) : '', {
            sel: i >= 0, badge: i >= 0 ? (max > 1 ? String(i + 1) : '✓') : null,
            onclick: () => {
              const j = sel.indexOf(w.id);
              if (j >= 0) sel.splice(j, 1);
              else { if (sel.length >= max) sel.shift(); sel.push(w.id); }
              render();
            },
          }));
        }
        if (!shown.length) list.append(h('p', { class: 'empty' }, '고를 수 있는 작품이 없다.'));
        okBtn.disabled = sel.length === 0;
        selInfo.textContent = max > 1 ? `선택 ${sel.length}/${max}` + (hint ? ` · ${hint}` : '') : (hint || '');
      }
      render();
      setPanel(win(title, h('div', { class: 'picker' }, tabsEl, list, selInfo), [cancel, okBtn].filter(Boolean)));
      panel.className = 'tall';
      updateStatus();
    });
  }

  return { h, init, setScene, setPanel, clear, updateStatus, say, talk, choose, win, notice, progress, runDefrag, pickWorks, workRow, TABS, fitScene };
})();
