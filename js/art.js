// 수집왕 — 도트 그래픽 (모든 그림을 코드로 그린다)
'use strict';

const Art = (() => {
  const W = 180, H = 135;
  const FONT = '11px Galmuri11';

  function R(ctx, x, y, w, h, c) { ctx.fillStyle = c; ctx.fillRect(x, y, w, h); }
  function P(ctx, pts, c) { ctx.fillStyle = c; for (const [x, y] of pts) ctx.fillRect(x, y, 1, 1); }

  // 3x5 숫자 폰트
  const DIG = {
    '0': '111101101101111', '1': '010110010010111', '2': '111001111100111', '3': '111001111001111',
    '4': '101101111001001', '5': '111100111001111', '6': '111100111101111', '7': '111001010010010',
    '8': '111101111101111', '9': '111101111001111',
  };
  function digits(ctx, str, x, y, c) {
    for (const ch of String(str)) {
      const p = DIG[ch];
      if (p) for (let i = 0; i < 15; i++) if (p[i] === '1') R(ctx, x + (i % 3), y + ((i / 3) | 0), 1, 1, c);
      x += 4;
    }
  }
  function seasonOf(m) {
    if (m >= 3 && m <= 5) return 'spring';
    if (m >= 6 && m <= 8) return 'summer';
    if (m >= 9 && m <= 11) return 'autumn';
    return 'winter';
  }
  function rng(seed) {
    let v = seed >>> 0 || 1;
    return () => { v = (v * 1664525 + 1013904223) >>> 0; return v / 4294967296; };
  }

  const SKY = {
    spring: ['#8fb4e0', '#b9cfe8', '#f2d1c9', '#f6b9a6'],
    summer: ['#4f8fd8', '#7fb2ea', '#b8daf5', '#e3f0d0'],
    autumn: ['#5b6aa8', '#a184b0', '#e9a07a', '#f4c27a'],
    winter: ['#3c4a78', '#5c6c9a', '#8c9cc0', '#c8d0e0'],
  };
  const CD_COLORS = ['#e8e8f0', '#c9d6e8', '#f2d48a', '#9fc7e8', '#e8a0a0', '#b0e0b8', '#d0b8e8', '#f0f0f0', '#a8b0c0'];

  // ---------- 방 ----------
  function drawWindow(ctx, season, t, future) {
    const x = 8, y = 10, w = 50, h = 44;
    R(ctx, x - 2, y - 2, w + 4, h + 4, '#f4efe2');
    const sky = future ? ['#141a3a', '#1e2a52', '#2a3a6a', '#3a4a7a'] : SKY[season];
    for (let i = 0; i < 4; i++) R(ctx, x, y + i * (h / 4), w, h / 4 + 1, sky[i]);
    if (future) {
      // 2026년 도시의 밤 — 아파트 불빛
      const r = rng(26);
      for (let i = 0; i < 5; i++) {
        const bx = x + i * 10 + 1, bh = 14 + ((r() * 18) | 0);
        R(ctx, bx, y + h - bh, 9, bh, '#10142a');
        for (let wy = y + h - bh + 2; wy < y + h - 2; wy += 3)
          for (let wx = bx + 1; wx < bx + 8; wx += 3) if (r() > 0.45) R(ctx, wx, wy, 1, 1, '#f6d77a');
      }
    } else if (season === 'spring') {
      R(ctx, x, y + 34, 22, 2, '#6b4a3a');
      const r = rng(3);
      for (let i = 0; i < 40; i++) P(ctx, [[x + ((r() * 26) | 0), y + 26 + ((r() * 10) | 0)]], r() > 0.5 ? '#f7c6d4' : '#f2a8bf');
      for (let i = 0; i < 4; i++) { const fx = x + ((i * 13 + t * 0.02) % w); P(ctx, [[fx | 0, y + ((i * 9 + t * 0.03) % h) | 0]], '#f7c6d4'); }
    } else if (season === 'summer') {
      R(ctx, x + 30, y + 20, 2, 24, '#5a3a2a');
      const r = rng(5);
      for (let i = 0; i < 70; i++) P(ctx, [[x + 20 + ((r() * 30) | 0), y + 6 + ((r() * 22) | 0)]], r() > 0.5 ? '#4f9a4a' : '#3d7d3a');
    } else if (season === 'autumn') {
      R(ctx, x + 6, y + 22, 2, 22, '#5a3a2a');
      const r = rng(7);
      for (let i = 0; i < 50; i++) P(ctx, [[x + ((r() * 20) | 0), y + 10 + ((r() * 18) | 0)]], r() > 0.5 ? '#e0822a' : '#c8502a');
      for (let i = 0; i < 3; i++) P(ctx, [[x + ((i * 17 + t * 0.015) % w) | 0, y + ((i * 11 + t * 0.025) % h) | 0]], '#e0822a');
    } else {
      R(ctx, x, y + h - 4, w, 4, '#f4f6fa');
      for (let i = 0; i < 14; i++) {
        const fx = x + ((i * 7 + Math.sin(t / 900 + i) * 3 + 50) % w);
        const fy = y + ((i * 13 + t * 0.012 * (1 + (i % 3) * 0.3)) % h);
        P(ctx, [[fx | 0, fy | 0]], '#ffffff');
      }
    }
    // 창살
    R(ctx, x + w / 2 - 1, y, 2, h, '#f4efe2');
    R(ctx, x, y + h / 2 - 1, w, 2, '#f4efe2');
    // 커튼
    R(ctx, x - 5, y - 4, 5, h + 10, '#c9a07a');
    R(ctx, x + w, y - 4, 5, h + 10, '#c9a07a');
    for (let i = 0; i < h + 10; i += 3) { R(ctx, x - 4, y - 4 + i, 1, 2, '#b08a64'); R(ctx, x + w + 2, y - 4 + i, 1, 2, '#b08a64'); }
  }

  function drawPoster(ctx, s) {
    const tr = computeTraits(s);
    if (!tr.top) return;
    const c = GENRES[tr.top].color;
    R(ctx, 66, 12, 22, 32, '#fdfaf0');
    R(ctx, 67, 13, 20, 30, c);
    R(ctx, 70, 17, 14, 14, '#ffffff33');
    P(ctx, [[76, 19], [75, 20], [77, 20], [74, 21], [78, 21], [76, 22], [76, 23]], '#ffffff');
    R(ctx, 69, 36, 16, 2, '#ffffffaa');
    R(ctx, 71, 39, 12, 1, '#ffffff88');
    R(ctx, 76, 11, 2, 2, '#c03030'); // 압정
    if (tr.total > 120) { // 두 번째 포스터
      const c2 = GENRES[Object.entries(tr.genre).sort((a, b) => b[1] - a[1])[1][0]].color;
      R(ctx, 160, 14, 16, 22, '#fdfaf0');
      R(ctx, 161, 15, 14, 20, c2);
      R(ctx, 164, 19, 8, 8, '#ffffff44');
      R(ctx, 167, 13, 2, 2, '#3060c0');
    }
  }

  function drawCalendar(ctx, s, m) {
    R(ctx, 94, 14, 16, 18, '#fdfdf8');
    R(ctx, 94, 14, 16, 4, '#d84a3a');
    const str = String(m);
    digits(ctx, str, 102 - str.length * 2, 21, '#333333');
    R(ctx, 96, 28, 12, 1, '#cccccc');
  }

  function drawRack(ctx, s) {
    const x = 4, y = 58, w = 56;
    R(ctx, x, y, w, 40, '#6b4a30');
    R(ctx, x + 2, y + 2, w - 4, 36, '#4a3220');
    const rows = [y + 3, y + 15, y + 27];
    for (const ry of rows) R(ctx, x + 2, ry + 10, w - 4, 2, '#7d5838');
    let discs = s.stats.discs;
    const r = rng(42);
    // 처음엔 교과서 몇 권
    const books = ['#3a6ab0', '#b03a3a', '#3a9a5a', '#d8b040'];
    for (let i = 0; i < 4; i++) R(ctx, x + 4 + i * 4, rows[2], 3, 10, books[i]);
    let idx = 0;
    for (let ri = 0; ri < 3 && discs > 0; ri++) {
      const start = ri === 2 ? x + 22 : x + 3;
      for (let cx = start; cx < x + w - 3 && discs > 0; cx++, discs--, idx++) {
        const col = CD_COLORS[(r() * CD_COLORS.length) | 0];
        R(ctx, cx, rows[ri] + 1, 1, 9, col);
        if (idx % 2) R(ctx, cx, rows[ri] + 1, 1, 1, '#ffffff');
      }
    }
    // 서점에서 산 실물 책은 방바닥에 쌓인다
    let books2 = 0;
    for (const id in s.col) { const w = WORK_BY_ID[id]; if (w && (w.type === 'manga' || w.type === 'novel')) books2 += s.col[id].phys || 0; }
    const BOOK_COLORS = ['#c8503a', '#3a6ab0', '#e0b040', '#4a9a5a', '#8a5aa0', '#e8e0d0'];
    for (let i = 0, bx = 2; i < Math.min(books2, 60) && bx < 16; bx += 7) {
      for (let k = 0; k < 20 && i < Math.min(books2, 60); k++, i++) {
        R(ctx, bx, 132 - k * 2, 6, 2, BOOK_COLORS[(i * 7) % BOOK_COLORS.length]);
        R(ctx, bx, 133 - k * 2, 6, 1, '#00000033');
      }
    }
    // 넘치는 CD는 바닥에 탑으로
    let sx = 62;
    while (discs > 0 && sx < 92) {
      const n = Math.min(discs, 26);
      for (let i = 0; i < n; i++) R(ctx, sx, 126 - i, 7, 1, i % 2 ? '#d8dce8' : '#b8c0d0');
      R(ctx, sx + 3, 126 - n, 1, 1, '#ffffff');
      discs -= n; sx += 9;
    }
  }

  function drawDesk(ctx) {
    R(ctx, 90, 77, 90, 4, '#8a5c38');
    R(ctx, 90, 81, 90, 2, '#6a4428');
    R(ctx, 92, 83, 4, 32, '#6a4428');
    R(ctx, 174, 83, 4, 32, '#6a4428');
    R(ctx, 150, 83, 24, 18, '#7a5030'); // 서랍
    R(ctx, 160, 90, 4, 1, '#d8c090');
  }

  function drawMonitor(ctx, s, t, future) {
    if (future) {
      // 얇은 LCD 모니터 + NAS
      R(ctx, 102, 40, 48, 30, '#1a1a1e');
      R(ctx, 104, 42, 44, 25, '#2a5aa0');
      R(ctx, 104, 42, 44, 3, '#3a7ad0');
      R(ctx, 108, 48, 14, 10, '#d8e4f4');
      R(ctx, 126, 48, 18, 4, '#d8e4f4');
      R(ctx, 126, 54, 12, 4, '#d8e4f4');
      R(ctx, 122, 70, 8, 5, '#2a2a2e');
      R(ctx, 114, 75, 24, 2, '#2a2a2e');
      R(ctx, 108, 78, 34, 2, '#3a3a40');
      R(ctx, 154, 58, 14, 19, '#2a2a30');
      for (let i = 0; i < 4; i++) R(ctx, 156, 61 + i * 4, 10, 2, '#40404a');
      for (let i = 0; i < 4; i++) R(ctx, 165, 61 + i * 4, 1, 1, (t / 300 + i) % 3 < 1.5 ? '#40e070' : '#208040');
      return;
    }
    // 스피커
    R(ctx, 96, 62, 9, 15, '#d8cfb8'); R(ctx, 98, 65, 5, 5, '#8a826e'); R(ctx, 99, 72, 3, 3, '#8a826e');
    // CRT
    R(ctx, 107, 44, 40, 33, '#d8cfb8');
    R(ctx, 107, 74, 40, 3, '#b8ad94');
    R(ctx, 145, 44, 2, 33, '#b8ad94');
    R(ctx, 110, 47, 34, 25, '#3a3a3a');
    const sx = 112, sy = 49, sw = 30, sh = 21;
    const mode = s.screen || 'desktop';
    if (mode === 'off') {
      R(ctx, sx, sy, sw, sh, '#1e2626');
      R(ctx, sx + 2, sy + 2, 6, 1, '#3a4848');
    } else {
      R(ctx, sx, sy, sw, sh, '#008080');
      // 아이콘
      R(ctx, sx + 2, sy + 2, 3, 3, '#f0f0f0'); R(ctx, sx + 2, sy + 7, 3, 3, '#f2d24a'); R(ctx, sx + 2, sy + 12, 3, 3, '#a0c8f0');
      // 작업 표시줄
      R(ctx, sx, sy + sh - 3, sw, 3, '#c0c0c0');
      R(ctx, sx + 1, sy + sh - 2, 5, 1, '#3a8a3a');
      if (mode === 'download' || mode === 'burn') {
        R(ctx, sx + 8, sy + 4, 20, 11, '#c0c0c0');
        R(ctx, sx + 8, sy + 4, 20, 2, '#000080');
        R(ctx, sx + 10, sy + 9, 16, 3, '#ffffff');
        const p = (t / 40) % 16;
        R(ctx, sx + 10, sy + 9, p | 0, 3, mode === 'burn' ? '#c03030' : '#000080');
      }
    }
    // 키보드
    R(ctx, 112, 79, 30, 3, '#d8cfb8'); R(ctx, 112, 81, 30, 1, '#b8ad94');
    // 본체
    R(ctx, 150, 46, 20, 31, '#d8cfb8');
    R(ctx, 168, 46, 2, 31, '#b8ad94');
    R(ctx, 152, 50, 15, 3, '#b8ad94');
    if (s.pc.burner) { R(ctx, 152, 55, 15, 3, '#b8ad94'); R(ctx, 164, 56, 2, 1, s.pc.driveBroken ? '#c03030' : '#e09030'); }
    R(ctx, 158, 70, 3, 1, (mode !== 'off' && (t / 250) % 2 < 1) ? '#40e070' : '#206030');
    // 모니터 빛
    if (mode !== 'off') { ctx.fillStyle = 'rgba(120,220,220,0.08)'; ctx.fillRect(100, 40, 56, 50); }
  }

  function drawBoy(ctx, s, future) {
    const d = dateOf(s.turn);
    const season = seasonOf(d.m);
    const shirt = future ? '#5a6a7a' : season === 'winter' ? '#2a3a6a' : season === 'summer' ? '#f4f4f4' : '#e8ecf4';
    const shade = future ? '#4a5866' : season === 'winter' ? '#1e2a50' : '#c8ccd8';
    // 의자
    R(ctx, 114, 92, 30, 26, '#33334a');
    R(ctx, 116, 94, 26, 22, '#44445e');
    R(ctx, 127, 118, 4, 8, '#2a2a3a');
    R(ctx, 119, 126, 20, 2, '#2a2a3a');
    // 몸
    R(ctx, 117, 88, 24, 8, shirt);
    R(ctx, 117, 94, 24, 2, shade);
    R(ctx, 114, 89, 4, 8, shirt); R(ctx, 140, 89, 4, 8, shirt);
    // 목, 머리
    R(ctx, 126, 85, 6, 4, '#e2b08c');
    const hair = future ? '#2a2422' : '#1d1a1a';
    R(ctx, 124, 73, 10, 1, hair); R(ctx, 122, 74, 14, 1, hair);
    R(ctx, 121, 75, 16, 8, hair); R(ctx, 122, 83, 14, 2, hair);
    R(ctx, 120, 78, 1, 4, '#e2b08c'); R(ctx, 137, 78, 1, 4, '#e2b08c');
    P(ctx, [[126, 75], [127, 75], [131, 76]], '#3a3434');
    if (future) P(ctx, [[124, 76], [133, 77], [129, 75]], '#8a8a8a'); // 흰머리 몇 가닥
  }

  function drawFloor(ctx, s, season) {
    R(ctx, 0, 0, W, 98, '#e9dcc0');
    for (let x = 0; x < W; x += 8) R(ctx, x, 0, 1, 98, '#e2d4b6');
    R(ctx, 0, 96, W, 3, '#8a6a4a');
    R(ctx, 0, 99, W, 36, '#b07d52');
    for (let y = 104; y < H; y += 8) R(ctx, 0, y, W, 1, '#9a6a44');
    // 러그
    R(ctx, 20, 110, 90, 20, '#8a3e3e');
    R(ctx, 22, 112, 86, 16, '#a85050');
    for (let x = 24; x < 106; x += 6) R(ctx, x, 119, 3, 2, '#e8c070');
  }

  function drawRoom(ctx, s, t = 0, opt = {}) {
    const future = !!opt.future;
    const d = dateOf(s.turn);
    const season = future ? 'winter' : seasonOf(d.m);
    ctx.imageSmoothingEnabled = false;
    drawFloor(ctx, s, season);
    drawWindow(ctx, season, t, future);
    drawPoster(ctx, s);
    if (!future) drawCalendar(ctx, s, d.m);
    else { R(ctx, 94, 14, 16, 18, '#fdfdf8'); digits(ctx, '26', 98, 21, '#333'); }
    drawRack(ctx, s);
    drawDesk(ctx);
    drawMonitor(ctx, s, t, future);
    drawBoy(ctx, s, future);
    // 저녁 분위기
    ctx.fillStyle = future ? 'rgba(10,14,40,0.35)' : season === 'winter' ? 'rgba(30,30,70,0.18)' : 'rgba(255,170,90,0.07)';
    ctx.fillRect(0, 0, W, H);
  }

  // ---------- 타이틀 ----------
  function drawTitle(ctx, t) {
    ctx.imageSmoothingEnabled = false;
    R(ctx, 0, 0, W, H, '#14141f');
    for (let y = 0; y < 98; y += 6) R(ctx, 0, y, W, 1, '#18182a');
    R(ctx, 0, 100, W, 35, '#22182a');
    R(ctx, 20, 98, 140, 5, '#3a2a22');
    // 큰 CRT
    R(ctx, 36, 16, 108, 82, '#cfc6ae');
    R(ctx, 36, 92, 108, 6, '#aea38a');
    R(ctx, 140, 16, 4, 82, '#aea38a');
    R(ctx, 42, 22, 94, 66, '#2a2a2a');
    R(ctx, 45, 25, 88, 60, '#0a2a5a');
    // 스캔라인
    for (let y = 25; y < 85; y += 2) { ctx.fillStyle = 'rgba(0,0,0,0.18)'; ctx.fillRect(45, y, 88, 1); }
    ctx.font = FONT; ctx.textBaseline = 'top'; ctx.textAlign = 'center';
    ctx.fillStyle = '#000a20'; ctx.fillText('수집왕', 90, 41);
    ctx.fillStyle = '#ffe680'; ctx.fillText('수집왕', 89, 40);
    ctx.fillStyle = '#9ad0ff';
    ctx.fillText('2000 - 2003', 89, 56);
    if ((t / 500) % 2 < 1) R(ctx, 113, 57, 4, 9, '#9ad0ff');
    // 전원 LED
    R(ctx, 128, 93, 3, 2, '#40e070');
    // 화면 빛
    ctx.fillStyle = 'rgba(80,140,255,0.08)'; ctx.fillRect(10, 10, 160, 110);
    // 바닥 CD 몇 장
    for (let i = 0; i < 6; i++) R(ctx, 150, 126 - i * 2, 14, 2, i % 2 ? '#c8d0e0' : '#a8b0c8');
    R(ctx, 18, 118, 12, 12, '#d8dce8'); R(ctx, 22, 122, 4, 4, '#14141f');
  }

  // ---------- 조각모음 ----------
  function makeDefrag(frag) {
    const N = 26 * 11;
    const cells = [];
    for (let i = 0; i < N; i++) {
      const r = Math.random();
      cells.push(r < 0.5 ? 1 : r < 0.5 + frag / 250 ? 2 : 0);
    }
    const used = cells.filter(c => c).length;
    const final = cells.map((c, i) => (i < used ? 1 : 0));
    return { cells, final, N };
  }
  function drawDefrag(ctx, df, p) {
    ctx.imageSmoothingEnabled = false;
    R(ctx, 0, 0, W, H, '#008080');
    R(ctx, 4, 6, 172, 122, '#c0c0c0');
    R(ctx, 4, 6, 172, 1, '#ffffff'); R(ctx, 4, 6, 1, 122, '#ffffff');
    R(ctx, 175, 6, 1, 122, '#404040'); R(ctx, 4, 127, 172, 1, '#404040');
    R(ctx, 6, 8, 168, 12, '#000080');
    ctx.font = FONT; ctx.textBaseline = 'top'; ctx.textAlign = 'left';
    ctx.fillStyle = '#ffffff'; ctx.fillText('디스크 조각 모음 - C:', 9, 9);
    R(ctx, 10, 24, 160, 74, '#ffffff');
    const k = Math.floor(df.N * p);
    for (let i = 0; i < df.N; i++) {
      const c = i < k ? df.final[i] : df.cells[i];
      const x = 12 + (i % 26) * 6, y = 26 + Math.floor(i / 26) * 6.5;
      const col = i === k ? '#20c040' : c === 1 ? '#2040c0' : c === 2 ? '#d03030' : '#e8e8e8';
      R(ctx, x, y | 0, 5, 5, col);
    }
    R(ctx, 10, 104, 160, 8, '#ffffff');
    R(ctx, 11, 105, Math.floor(158 * p), 6, '#000080');
    ctx.fillStyle = '#000000';
    ctx.fillText(`완료: ${Math.floor(p * 100)}%`, 12, 114);
  }

  // ---------- 초상화 (40x40) ----------
  const SKIN = '#f0c8a0', SKIN_D = '#d8a880', EYE = '#2a1e1e';
  function faceBase(ctx, skin = SKIN) {
    R(ctx, 13, 10, 14, 19, skin);
    R(ctx, 12, 12, 16, 15, skin);
    R(ctx, 14, 29, 12, 1, skin);
    R(ctx, 11, 18, 1, 4, skin); R(ctx, 28, 18, 1, 4, skin);
    R(ctx, 17, 29, 6, 4, skin);
    R(ctx, 17, 29, 6, 1, SKIN_D);
  }
  function expression(ctx, expr, opt = {}) {
    const ey = opt.eyeY || 19;
    if (expr === 'smile') {
      P(ctx, [[14, ey + 1], [15, ey], [16, ey], [17, ey + 1], [22, ey + 1], [23, ey], [24, ey], [25, ey + 1]], EYE);
      P(ctx, [[17, 24], [18, 25], [19, 25], [20, 25], [21, 25], [22, 24]], '#9a4a3a');
    } else if (expr === 'sad') {
      R(ctx, 15, ey + 1, 2, 2, EYE); R(ctx, 23, ey + 1, 2, 2, EYE);
      P(ctx, [[14, ey - 1], [15, ey - 2], [16, ey - 2], [23, ey - 2], [24, ey - 2], [25, ey - 1]], '#5a3a2a');
      P(ctx, [[18, 26], [19, 25], [20, 25], [21, 25], [22, 26]], '#9a4a3a');
    } else if (expr === 'surprise') {
      R(ctx, 15, ey - 1, 2, 4, EYE); R(ctx, 23, ey - 1, 2, 4, EYE);
      R(ctx, 19, 24, 2, 3, '#9a4a3a');
    } else if (expr === 'angry') {
      R(ctx, 15, ey, 2, 3, EYE); R(ctx, 23, ey, 2, 3, EYE);
      P(ctx, [[14, ey - 2], [15, ey - 2], [16, ey - 1], [23, ey - 1], [24, ey - 2], [25, ey - 2]], '#3a2a2a');
      R(ctx, 18, 25, 4, 1, '#9a4a3a');
    } else {
      R(ctx, 15, ey, 2, 3, EYE); R(ctx, 23, ey, 2, 3, EYE);
      P(ctx, [[15, ey], [23, ey]], '#ffffff');
      R(ctx, 19, 25, 3, 1, '#9a4a3a');
    }
    if (expr === 'blush' || opt.blush) {
      R(ctx, 13, 23, 3, 1, '#f08a9a'); R(ctx, 24, 23, 3, 1, '#f08a9a');
      if (expr === 'blush') { R(ctx, 15, ey, 2, 3, EYE); R(ctx, 23, ey, 2, 3, EYE); R(ctx, 19, 25, 2, 1, '#9a4a3a'); }
    }
  }
  function glasses(ctx, c = '#2a2a2a') {
    for (const gx of [13, 21]) {
      R(ctx, gx, 17, 6, 1, c); R(ctx, gx, 22, 6, 1, c); R(ctx, gx, 17, 1, 6, c); R(ctx, gx + 5, 17, 1, 6, c);
    }
    R(ctx, 19, 18, 2, 1, c);
  }

  const FACES = {
    her(ctx, expr) {
      R(ctx, 0, 0, 40, 40, '#f6d6dc');
      const hc = '#2b1d1a';
      R(ctx, 9, 11, 4, 27, hc); R(ctx, 27, 11, 4, 27, hc);
      R(ctx, 30, 30, 2, 8, hc);
      R(ctx, 2, 33, 36, 7, '#2a3466');
      R(ctx, 16, 33, 8, 4, '#f4f4f4'); R(ctx, 18, 35, 4, 3, '#d03040');
      faceBase(ctx);
      R(ctx, 11, 7, 18, 4, hc); R(ctx, 10, 9, 20, 3, hc);
      R(ctx, 12, 11, 16, 3, hc); R(ctx, 12, 14, 2, 3, hc); R(ctx, 26, 14, 2, 3, hc);
      P(ctx, [[16, 13], [22, 13]], SKIN);
      R(ctx, 25, 9, 3, 2, '#f06a8a');
      expression(ctx, expr);
    },
    mom(ctx, expr) {
      R(ctx, 0, 0, 40, 40, '#f2e2c4');
      R(ctx, 2, 33, 36, 7, '#c86a7a'); R(ctx, 12, 34, 16, 6, '#f6e8ea');
      const hc = '#5a3a2a';
      faceBase(ctx);
      for (const [x, y] of [[9, 8], [13, 6], [18, 5], [23, 6], [27, 8], [8, 13], [29, 13], [8, 18], [29, 18]]) R(ctx, x, y, 5, 5, hc);
      R(ctx, 11, 9, 18, 3, hc);
      P(ctx, [[10, 7], [16, 5], [24, 5], [30, 11]], '#7a5240');
      expression(ctx, expr);
    },
    dad(ctx, expr) {
      R(ctx, 0, 0, 40, 40, '#d8e2ea');
      R(ctx, 2, 33, 36, 7, '#f4f4f4'); R(ctx, 19, 33, 3, 7, '#3a4a8a');
      faceBase(ctx);
      R(ctx, 11, 7, 18, 5, '#222222'); R(ctx, 11, 10, 2, 6, '#222222'); R(ctx, 27, 10, 2, 6, '#222222');
      P(ctx, [[14, 8], [20, 7]], '#6a6a6a');
      expression(ctx, expr);
      glasses(ctx, '#4a3a2a');
    },
    friend(ctx, expr) {
      R(ctx, 0, 0, 40, 40, '#cfe8c8');
      R(ctx, 2, 33, 36, 7, '#2a5ab0'); R(ctx, 19, 33, 2, 7, '#f4f4f4');
      faceBase(ctx, '#d9a072');
      R(ctx, 12, 8, 16, 4, '#2a2a2a'); R(ctx, 12, 11, 1, 3, '#2a2a2a'); R(ctx, 27, 11, 1, 3, '#2a2a2a');
      P(ctx, [[14, 9], [18, 9], [22, 9], [25, 10]], '#4a4a4a');
      expression(ctx, expr === 'normal' ? 'smile' : expr);
      R(ctx, 26, 24, 2, 1, '#c07050'); // 반창고
    },
    hyung(ctx, expr) {
      R(ctx, 0, 0, 40, 40, '#2c2c3c');
      R(ctx, 0, 0, 40, 40, 'rgba(80,140,255,0.12)');
      R(ctx, 2, 33, 36, 7, '#1a1a1a');
      faceBase(ctx);
      const hc = '#3a2c22';
      R(ctx, 10, 6, 20, 7, hc); R(ctx, 9, 9, 3, 12, hc); R(ctx, 28, 9, 3, 12, hc);
      P(ctx, [[11, 5], [15, 4], [20, 4], [25, 5], [29, 7], [8, 12]], hc);
      R(ctx, 13, 12, 4, 3, hc); R(ctx, 22, 12, 5, 2, hc);
      expression(ctx, expr);
      glasses(ctx, '#111111');
      P(ctx, [[15, 27], [18, 28], [22, 28], [24, 27]], '#8a6a5a');
    },
    owner(ctx, expr) {
      R(ctx, 0, 0, 40, 40, '#e8dcc0');
      R(ctx, 2, 33, 36, 7, '#5a6a4a'); R(ctx, 12, 33, 16, 7, '#3a4a2a');
      faceBase(ctx, '#e0b08a');
      R(ctx, 10, 6, 20, 6, '#c83a2a'); R(ctx, 8, 11, 16, 2, '#a02a1a');
      R(ctx, 18, 7, 4, 3, '#f4f4f4');
      expression(ctx, expr === 'normal' ? 'smile' : expr);
      P(ctx, [[15, 27], [17, 28], [20, 28], [23, 28], [25, 27], [16, 26]], '#7a5a4a');
    },
  };

  function drawFace(cv, who, expr = 'normal') {
    const ctx = cv.getContext('2d');
    ctx.imageSmoothingEnabled = false;
    ctx.clearRect(0, 0, 40, 40);
    (FACES[who] || FACES.friend)(ctx, expr);
  }

  // ---------- 파일 아이콘 (16x16) ----------
  function drawIcon(cv, type, done) {
    const ctx = cv.getContext('2d');
    ctx.clearRect(0, 0, 16, 16);
    const ty = TYPES[type];
    if (type === 'novel') {
      R(ctx, 3, 1, 10, 14, '#ffffff'); R(ctx, 3, 1, 10, 1, '#808080'); R(ctx, 12, 1, 1, 14, '#808080');
      for (let y = 4; y < 13; y += 2) R(ctx, 5, y, 6, 1, '#4060a0');
    } else if (type === 'manga') {
      R(ctx, 1, 4, 14, 11, '#f2d24a'); R(ctx, 1, 3, 6, 2, '#f2d24a'); R(ctx, 1, 14, 14, 1, '#a08020');
      R(ctx, 4, 7, 8, 6, '#ffffff'); R(ctx, 5, 8, 3, 2, '#3aa0e0'); R(ctx, 8, 10, 3, 2, '#40a040');
    } else if (type === 'music') {
      R(ctx, 2, 2, 12, 12, '#d0d0e8'); R(ctx, 6, 4, 1, 7, '#2a2a6a'); R(ctx, 6, 4, 5, 1, '#2a2a6a');
      R(ctx, 10, 4, 1, 6, '#2a2a6a'); R(ctx, 4, 10, 3, 2, '#2a2a6a'); R(ctx, 8, 9, 3, 2, '#2a2a6a');
    } else { // anime, movie, game: 디스크/필름
      const c = type === 'game' ? '#a0e0a0' : type === 'movie' ? '#f0b0b0' : '#a8c8f0';
      R(ctx, 3, 1, 10, 14, '#e8e8e8'); R(ctx, 2, 3, 12, 10, '#e8e8e8');
      R(ctx, 4, 3, 8, 10, c); R(ctx, 3, 4, 10, 8, c);
      R(ctx, 7, 7, 2, 2, '#ffffff'); R(ctx, 5, 4, 2, 1, '#ffffff');
    }
    if (done) { R(ctx, 10, 0, 6, 6, '#e8c020'); P(ctx, [[12, 1], [13, 1], [11, 2], [14, 2], [12, 3], [13, 3], [12, 4], [13, 4]], '#fff4a0'); }
  }

  return { W, H, drawRoom, drawTitle, drawFace, drawIcon, makeDefrag, drawDefrag, seasonOf };
})();
