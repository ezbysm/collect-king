// 수집왕 — 도트 그래픽 (모든 그림을 코드로 그린다)
'use strict';

const Art = (() => {
  const W = 180, H = 135;
  const FONT = '11px Galmuri11';

  function R(ctx, x, y, w, h, c) { ctx.fillStyle = c; ctx.fillRect(x, y, w, h); }
  function P(ctx, pts, c) { ctx.fillStyle = c; for (const [x, y] of pts) ctx.fillRect(x, y, 1, 1); }

  // 4x4 베이어 디더링: density(0~1)만큼 점을 찍어 중간 색을 만든다
  const BAYER = [0, 8, 2, 10, 12, 4, 14, 6, 3, 11, 1, 9, 15, 7, 13, 5];
  function dither(ctx, x, y, w, h, c, d) {
    ctx.fillStyle = c;
    for (let py = y; py < y + h; py++)
      for (let px = x; px < x + w; px++)
        if ((BAYER[(py & 3) * 4 + (px & 3)] + 0.5) / 16 < d) ctx.fillRect(px, py, 1, 1);
  }
  function hex2rgb(h) { const n = parseInt(h.slice(1, 7), 16); return [n >> 16, (n >> 8) & 255, n & 255]; }
  function rgb2hex(c) { return '#' + c.map(v => Math.round(v).toString(16).padStart(2, '0')).join(''); }
  // 색 사이를 steps단계로 나눠 더 부드러운 팔레트를 만든다
  function smooth(colors, steps = 4) {
    const out = [];
    for (let i = 0; i < colors.length - 1; i++) {
      const a = hex2rgb(colors[i]), b = hex2rgb(colors[i + 1]);
      for (let k = 0; k < steps; k++) out.push(rgb2hex(a.map((v, j) => v + (b[j] - v) * k / steps)));
    }
    out.push(colors[colors.length - 1]);
    return out;
  }
  // 세로 그라데이션을 디더링으로 (colors: 위→아래)
  function gradV(ctx, x, y, w, h, colors0) {
    const colors = colors0.length < h / 4 ? smooth(colors0, Math.max(1, Math.floor(h / 4 / Math.max(1, colors0.length - 1)))) : colors0;
    const n = colors.length - 1;
    for (let py = 0; py < h; py++) {
      const t = (py / Math.max(1, h - 1)) * n;
      const i = Math.min(n - 1, Math.floor(t)), f = t - i;
      R(ctx, x, y + py, w, 1, colors[i]);
      ctx.fillStyle = colors[i + 1];
      for (let px = x; px < x + w; px++)
        if ((BAYER[((y + py) & 3) * 4 + (px & 3)] + 0.5) / 16 < f) ctx.fillRect(px, y + py, 1, 1);
    }
  }
  function disc(ctx, cx, cy, r, c) {
    ctx.fillStyle = c;
    for (let y = -r; y <= r; y++) for (let x = -r; x <= r; x++) if (x * x + y * y <= r * r + r * 0.6) ctx.fillRect(cx + x, cy + y, 1, 1);
  }

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

  // 계절별 저녁 하늘 (위→아래)
  const SKY = {
    spring: ['#6f8fc8', '#9fb2dc', '#e4bfc8', '#f6c9a8', '#f8dcb0'],
    summer: ['#3f7fd0', '#62a0e4', '#9cc8f0', '#d4ecf6', '#f2f0d0'],
    autumn: ['#3d4a8a', '#7a6aa6', '#d48a7a', '#f0a868', '#f8cc84'],
    winter: ['#141c3c', '#23305a', '#3a4878', '#5a6a96', '#8a90b0'],
    future: ['#0c1028', '#141a3a', '#1e2a52', '#2a3a6a', '#3a4472'],
  };
  const CD_COLORS = ['#e8e8f0', '#c9d6e8', '#f2d48a', '#9fc7e8', '#e8a0a0', '#b0e0b8', '#d0b8e8', '#f0f0f0', '#a8b0c0'];
  const BEIGE = { hi: '#f2ecdc', base: '#ddd4bc', mid: '#cbc1a6', dark: '#aca286', deep: '#857c64' };
  const WOOD = { hi: '#c4925e', base: '#a8744a', mid: '#8e5e38', dark: '#6e4528', deep: '#4e2f1a' };

  // ---------- 창밖 ----------
  function drawSkyline(ctx, x, y, w, h, base, lit, seed, dark) {
    const r = rng(seed);
    let bx = x;
    while (bx < x + w) {
      const bw = 7 + ((r() * 6) | 0), bh = 8 + ((r() * (h - 6)) | 0);
      R(ctx, bx, y + h - bh, Math.min(bw, x + w - bx), bh, base);
      R(ctx, bx, y + h - bh, Math.min(bw, x + w - bx), 1, dark);
      for (let wy = y + h - bh + 2; wy < y + h - 1; wy += 3)
        for (let wx = bx + 1; wx < Math.min(bx + bw - 1, x + w); wx += 2) if (r() > 0.55) R(ctx, wx, wy, 1, 1, lit);
      bx += bw + 1;
    }
  }

  function drawWindowStatic(ctx, season, future) {
    const x = 8, y = 10, w = 50, h = 44;
    // 커튼 봉
    R(ctx, x - 9, y - 7, w + 18, 2, WOOD.dark);
    R(ctx, x - 10, y - 8, 3, 4, WOOD.deep); R(ctx, x + w + 7, y - 8, 3, 4, WOOD.deep);
    // 창틀
    R(ctx, x - 3, y - 3, w + 6, h + 6, '#efe6d2');
    R(ctx, x - 3, y + h + 2, w + 6, 1, '#d4c6a8');
    R(ctx, x - 1, y - 1, w + 2, 1, '#c8b896'); R(ctx, x - 1, y - 1, 1, h + 2, '#c8b896');
    // 하늘
    const sky = future ? SKY.future : SKY[season];
    gradV(ctx, x, y, w, h, sky);
    if (season === 'winter' || future) {
      const r = rng(9);
      for (let i = 0; i < 16; i++) R(ctx, x + ((r() * w) | 0), y + ((r() * h * 0.6) | 0), 1, 1, r() > 0.5 ? '#ffffff' : '#a8b4e0');
      // 초승달
      disc(ctx, x + 38, y + 9, 4, '#f4f0d8'); disc(ctx, x + 40, y + 8, 4, sky[0]);
    } else {
      const sunY = season === 'summer' ? y + 10 : y + h - 14;
      disc(ctx, x + 36, sunY, 6, season === 'summer' ? '#fff8d8' : '#ffd890');
      ctx.fillStyle = season === 'summer' ? 'rgba(255,250,230,0.25)' : 'rgba(255,230,170,0.3)';
      ctx.beginPath(); ctx.arc(x + 36, sunY, 9, 0, Math.PI * 2); ctx.fill();
      disc(ctx, x + 36, sunY, 4, season === 'summer' ? '#ffffff' : '#fff0c0');
      // 구름
      const cc = season === 'autumn' ? '#f4c0a0' : season === 'spring' ? '#fde4e4' : '#ffffff';
      for (const [cx, cy, rr] of [[x + 10, y + 12, 3], [x + 14, y + 11, 4], [x + 18, y + 12, 3]]) disc(ctx, cx, cy, rr, cc);
      R(ctx, x + 8, y + 14, 14, 2, cc);
      dither(ctx, x + 8, y + 14, 14, 2, '#00000022', 0.5);
    }
    // 먼 아파트 단지
    drawSkyline(ctx, x, y + h - 18, w, 18, future || season === 'winter' ? '#1a2040' : season === 'summer' ? '#6a86a8' : '#6a5a7a',
      '#ffd27a', 77, future || season === 'winter' ? '#2a3058' : '#7a6a8a');
    // 계절 앞쪽 풍경
    if (!future) {
      if (season === 'spring') {
        R(ctx, x, y + 3, 16, 2, '#5a3a2a'); R(ctx, x + 14, y + 5, 8, 1, '#5a3a2a'); R(ctx, x + 6, y + 5, 1, 6, '#5a3a2a');
        const r = rng(3);
        for (let i = 0; i < 70; i++) {
          const px = x + ((r() * 26) | 0), py = y + 1 + ((r() * 14) | 0);
          R(ctx, px, py, 1, 1, r() > 0.6 ? '#ffe4ee' : r() > 0.3 ? '#f7b8cc' : '#e890ac');
        }
      } else if (season === 'summer') {
        const r = rng(5);
        R(ctx, x + w - 6, y + 18, 2, h - 18, '#4a3022');
        for (let i = 0; i < 140; i++) {
          const px = x + w - 22 + ((r() * 22) | 0), py = y + ((r() * 26) | 0);
          R(ctx, px, py, 1, 1, r() > 0.66 ? '#7cc06a' : r() > 0.33 ? '#4f9a4a' : '#357a38');
        }
      } else if (season === 'autumn') {
        const r = rng(7);
        R(ctx, x, y + 6, 18, 1, '#4a3022'); R(ctx, x + 10, y + 7, 1, 5, '#4a3022');
        for (let i = 0; i < 70; i++) {
          const px = x + ((r() * 24) | 0), py = y + 1 + ((r() * 16) | 0);
          R(ctx, px, py, 1, 1, r() > 0.66 ? '#f0a030' : r() > 0.33 ? '#e06a2a' : '#b8402a');
        }
      } else {
        R(ctx, x, y + 4, 20, 1, '#3a2a22'); R(ctx, x + 12, y + 5, 1, 6, '#3a2a22');
        R(ctx, x, y + 3, 20, 1, '#f4f6fa'); R(ctx, x + 11, y + 4, 1, 1, '#f4f6fa');
        R(ctx, x, y + h - 3, w, 3, '#e8ecf4'); dither(ctx, x, y + h - 4, w, 1, '#e8ecf4', 0.5);
      }
    }
    // 창살
    drawWindowBars(ctx);
    // 창턱과 화분
    R(ctx, x - 5, y + h + 3, w + 10, 3, '#f4ecdc');
    R(ctx, x - 5, y + h + 6, w + 10, 1, '#c8b896');
    dither(ctx, x - 5, y + h + 7, w + 10, 2, '#b8a07a', 0.35);
    R(ctx, x + 41, y + h - 3, 8, 6, '#c0603a'); R(ctx, x + 40, y + h - 4, 10, 2, '#d47a50');
    R(ctx, x + 47, y + h - 3, 2, 6, '#9a4a2a');
    P(ctx, [[x + 43, y + h - 8], [x + 44, y + h - 9], [x + 45, y + h - 7], [x + 46, y + h - 10], [x + 42, y + h - 6], [x + 47, y + h - 7], [x + 44, y + h - 6], [x + 45, y + h - 5]], '#4f9a4a');
    P(ctx, [[x + 44, y + h - 10], [x + 46, y + h - 8], [x + 43, y + h - 6]], '#7cc06a');
    // 커튼 (주름)
    for (const cx of [x - 9, x + w + 2]) {
      R(ctx, cx, y - 5, 7, h + 14, '#c99a70');
      for (let i = 0; i < 7; i++) {
        const c = i % 3 === 0 ? '#a87a54' : i % 3 === 1 ? '#dcb48a' : '#c99a70';
        R(ctx, cx + i, y - 5, 1, h + 14, c);
      }
      R(ctx, cx, y + h + 8, 7, 1, '#a87a54');
      R(ctx, cx + 1, y + 30, 5, 2, '#8a6040'); // 커튼 끈
    }
  }
  function drawWindowBars(ctx) {
    const x = 8, y = 10, w = 50, h = 44;
    R(ctx, x + w / 2 - 1, y, 2, h, '#efe6d2'); R(ctx, x + w / 2 + 1, y, 1, h, '#cbbd9f');
    R(ctx, x, y + h / 2 - 1, w, 2, '#efe6d2'); R(ctx, x, y + h / 2 + 1, w, 1, '#cbbd9f');
  }
  function drawWindowAnim(ctx, season, t, future) {
    const x = 8, y = 10, w = 50, h = 44;
    if (future) {
      if ((t / 900) % 2 < 1) R(ctx, x + 30, y + h - 9, 1, 1, '#ff6060'); // 탑 꼭대기 불빛
    } else if (season === 'spring') {
      for (let i = 0; i < 6; i++) {
        const fx = x + ((i * 13 + t * 0.012 + Math.sin(t / 700 + i) * 4) % w + w) % w;
        const fy = y + ((i * 9 + t * 0.018) % h);
        R(ctx, fx | 0, fy | 0, 1, 1, i % 2 ? '#ffe4ee' : '#f7b8cc');
      }
    } else if (season === 'autumn') {
      for (let i = 0; i < 4; i++) {
        const fx = x + ((i * 17 + t * 0.01 + Math.sin(t / 800 + i) * 5) % w + w) % w;
        const fy = y + ((i * 11 + t * 0.02) % h);
        R(ctx, fx | 0, fy | 0, 2, 1, i % 2 ? '#e06a2a' : '#f0a030');
      }
    } else if (season === 'winter') {
      for (let i = 0; i < 18; i++) {
        const fx = x + ((i * 7 + Math.sin(t / 900 + i) * 3 + 50) % w);
        const fy = y + ((i * 13 + t * 0.012 * (1 + (i % 3) * 0.3)) % h);
        R(ctx, fx | 0, fy | 0, 1, 1, '#ffffff');
      }
    } else {
      // 여름: 잠자리 한 마리
      const dx = x + 10 + ((t / 60) % 30), dy = y + 20 + Math.sin(t / 300) * 3;
      R(ctx, dx | 0, dy | 0, 3, 1, '#c84a2a'); if ((t / 80) % 2 < 1) R(ctx, (dx | 0) + 1, (dy | 0) - 1, 1, 1, '#ffffffaa');
    }
    drawWindowBars(ctx);
  }

  // ---------- 벽 ----------
  function drawWall(ctx, future) {
    R(ctx, 0, 0, W, 98, '#e6d6b8');
    for (let x = 0; x < W; x += 12) { R(ctx, x, 0, 2, 98, '#dfceac'); }
    for (let y = 8; y < 94; y += 12) for (let x = 6; x < W; x += 12) P(ctx, [[x, y], [x - 1, y + 1], [x + 1, y + 1], [x, y + 2]], '#d6c39c');
    // 몰딩
    R(ctx, 0, 0, W, 3, WOOD.mid); R(ctx, 0, 3, W, 1, WOOD.dark);
    // 오른쪽 구석과 천장 쪽은 어둡게
    ctx.fillStyle = 'rgba(90,60,30,0.10)'; ctx.fillRect(150, 4, 30, 90);
    ctx.fillStyle = 'rgba(90,60,30,0.08)'; ctx.fillRect(0, 4, W, 6);
    // 걸레받이
    R(ctx, 0, 94, W, 5, WOOD.mid); R(ctx, 0, 94, W, 1, WOOD.hi); R(ctx, 0, 98, W, 1, WOOD.deep);
    if (future) { ctx.fillStyle = 'rgba(30,30,70,0.18)'; ctx.fillRect(0, 4, W, 90); }
  }

  function drawFloor(ctx) {
    R(ctx, 0, 99, W, 36, WOOD.base);
    const r = rng(11);
    for (let row = 0; row < 6; row++) {
      const y = 99 + row * 6;
      R(ctx, 0, y, W, 1, WOOD.dark);
      if (row % 2) dither(ctx, 0, y + 1, W, 5, WOOD.mid, 0.3);
      const off = (row * 37) % 50;
      for (let x = off; x < W; x += 50) R(ctx, x, y, 1, 6, WOOD.dark);
      for (let k = 0; k < 6; k++) R(ctx, (r() * W) | 0, y + 2 + ((r() * 3) | 0), 4 + ((r() * 6) | 0), 1, WOOD.hi);
    }
    // 러그
    const rx = 18, ry = 109, rw = 94, rh = 21;
    R(ctx, rx, ry, rw, rh, '#7a3434');
    R(ctx, rx + 2, ry + 2, rw - 4, rh - 4, '#a84a46');
    R(ctx, rx + 4, ry + 4, rw - 8, rh - 8, '#8e3c3a');
    for (let x = rx + 6; x < rx + rw - 6; x += 6) { P(ctx, [[x, ry + 10], [x + 1, ry + 9], [x + 2, ry + 10], [x + 1, ry + 11]], '#e8c070'); }
    for (let x = rx + 3; x < rx + rw - 3; x += 4) { R(ctx, x, ry + 3, 2, 1, '#d8a860'); R(ctx, x, ry + rh - 4, 2, 1, '#d8a860'); }
    for (let y = ry + 1; y < ry + rh; y += 2) { R(ctx, rx - 2, y, 2, 1, '#e8dcc0'); R(ctx, rx + rw, y, 2, 1, '#e8dcc0'); }
    dither(ctx, rx, ry + rh, rw, 2, '#4a2a1a', 0.3);
  }

  // ---------- 벽 장식 ----------
  function drawPoster(ctx, s, tr) {
    if (!tr.top) return;
    const c = GENRES[tr.top].color;
    R(ctx, 67, 13, 22, 32, '#00000030');
    R(ctx, 66, 12, 22, 32, '#fbf6ea');
    gradV(ctx, 67, 13, 20, 24, [c, c, '#2a2a3a']);
    // 가운데 인물 실루엣
    disc(ctx, 77, 21, 3, '#ffffffcc');
    R(ctx, 73, 25, 9, 8, '#ffffffcc'); R(ctx, 71, 27, 13, 6, '#ffffff99');
    R(ctx, 68, 38, 18, 2, '#3a3a4a'); R(ctx, 70, 41, 14, 1, '#9a9aa8');
    R(ctx, 65, 11, 4, 3, '#f0e8c8cc'); R(ctx, 85, 11, 4, 3, '#f0e8c8cc');
  }

  function drawCalendar(ctx, m, future) {
    R(ctx, 95, 15, 16, 19, '#00000025');
    R(ctx, 94, 14, 16, 19, '#fdfdf8');
    R(ctx, 94, 14, 16, 5, '#d84a3a'); R(ctx, 94, 18, 16, 1, '#a8342a');
    P(ctx, [[97, 13], [102, 13], [107, 13]], '#5a5a5a');
    const str = future ? '26' : String(m);
    digits(ctx, str, 102 - str.length * 2, 20, '#333333');
    for (let yy = 27; yy < 32; yy += 2) for (let xx = 96; xx < 109; xx += 2) R(ctx, xx, yy, 1, 1, '#c8c8c8');
  }

  function drawClock(ctx) {
    const cx = 131, cy = 23;
    disc(ctx, cx + 1, cy + 1, 7, '#00000025');
    disc(ctx, cx, cy, 7, WOOD.dark);
    disc(ctx, cx, cy, 6, '#f8f4e8');
    for (const [dx, dy] of [[0, -5], [5, 0], [0, 5], [-5, 0]]) R(ctx, cx + dx, cy + dy, 1, 1, '#5a5a5a');
    // 밤 10시 10분
    P(ctx, [[cx, cy], [cx - 1, cy - 1], [cx - 2, cy - 1], [cx - 3, cy - 2]], '#2a2a2a');
    P(ctx, [[cx + 1, cy - 1], [cx + 2, cy - 2], [cx + 3, cy - 3], [cx + 3, cy - 4]], '#2a2a2a');
  }

  function drawCorkBoard(ctx, s, tr) {
    const x = 150, y = 12, w = 27, h = 26;
    R(ctx, x + 1, y + 1, w, h, '#00000025');
    R(ctx, x, y, w, h, WOOD.mid);
    R(ctx, x + 1, y + 1, w - 2, h - 2, '#c8a070');
    dither(ctx, x + 1, y + 1, w - 2, h - 2, '#a8804e', 0.3);
    // 사진들: 친구와 그 아이의 추억이 늘어난다
    const photos = 1 + Math.min(3, Math.floor((s.friend || 0) / 30) + (s.love >= 50 ? 1 : 0));
    const spots = [[x + 3, y + 3, '#7ab0e0'], [x + 14, y + 4, '#90c890'], [x + 4, y + 14, '#f0b0c8'], [x + 15, y + 15, '#e8c870']];
    for (let i = 0; i < photos; i++) {
      const [px, py, c] = spots[i];
      R(ctx, px, py, 9, 8, '#fbfbf4'); R(ctx, px + 1, py + 1, 7, 5, c);
      R(ctx, px + 2, py + 3, 2, 3, '#4a3a3a88'); R(ctx, px + 5, py + 3, 2, 3, '#4a3a3a88');
      R(ctx, px + 4, py - 1, 1, 1, '#d83030');
    }
    if (tr.total > 120) { R(ctx, x + 18, y + 13, 7, 9, '#fff38a'); R(ctx, x + 19, y + 15, 5, 1, '#c8a830'); R(ctx, x + 19, y + 17, 4, 1, '#c8a830'); }
  }

  // ---------- 가구 ----------
  function drawRack(ctx, s) {
    const x = 4, y = 59, w = 56, h = 39;
    R(ctx, x, y, w, h, WOOD.mid);
    R(ctx, x, y, w, 2, WOOD.hi); R(ctx, x + w - 2, y, 2, h, WOOD.dark);
    R(ctx, x + 2, y + 2, w - 5, h - 3, WOOD.deep);
    const rows = [y + 3, y + 15, y + 27];
    for (const ry of rows) { R(ctx, x + 2, ry + 10, w - 4, 2, WOOD.base); R(ctx, x + 2, ry + 10, w - 4, 1, WOOD.hi); dither(ctx, x + 2, ry + 12, w - 5, 1, '#000000', 0.4); }
    let discs = s.stats.discs;
    const r = rng(42);
    const books = [['#3a6ab0', '#2a4a80'], ['#b03a3a', '#802a2a'], ['#3a9a5a', '#2a7040'], ['#d8b040', '#a88420']];
    for (let i = 0; i < 4; i++) { R(ctx, x + 4 + i * 4, rows[2], 3, 10, books[i][0]); R(ctx, x + 6 + i * 4, rows[2], 1, 10, books[i][1]); R(ctx, x + 4 + i * 4, rows[2] + 3, 3, 1, '#f0e8d0'); }
    let idx = 0;
    for (let ri = 0; ri < 3 && discs > 0; ri++) {
      const start = ri === 2 ? x + 22 : x + 3;
      for (let cx = start; cx < x + w - 4 && discs > 0; cx++, discs--, idx++) {
        const col = CD_COLORS[(r() * CD_COLORS.length) | 0];
        R(ctx, cx, rows[ri] + 1, 1, 9, col);
        R(ctx, cx, rows[ri] + 1, 1, 1, idx % 2 ? '#ffffff' : '#d8dce8');
        if (idx % 3 === 0) R(ctx, cx, rows[ri] + 5, 1, 2, '#5a5a6a');
      }
    }
    // 서점에서 산 실물 책은 방바닥에 쌓인다
    let nb = 0;
    for (const id in s.col) { const w2 = WORK_BY_ID[id]; if (w2 && (w2.type === 'manga' || w2.type === 'novel')) nb += s.col[id].phys || 0; }
    const BOOK = [['#c8503a', '#9a3a28'], ['#3a6ab0', '#2a4a80'], ['#e0b040', '#b08820'], ['#4a9a5a', '#2e7040'], ['#8a5aa0', '#6a3a80'], ['#e8e0d0', '#c0b8a8']];
    for (let i = 0, bx = 1; i < Math.min(nb, 40) && bx < 16; bx += 8) {
      for (let k = 0; k < 20 && i < Math.min(nb, 40); k++, i++) {
        const [c1, c2] = BOOK[(i * 7) % BOOK.length];
        const sh = (k * 3 + bx) % 3 - 1;
        R(ctx, bx + sh, 132 - k * 2, 7, 2, c1); R(ctx, bx + sh, 133 - k * 2, 7, 1, c2); R(ctx, bx + sh + 6, 132 - k * 2, 1, 1, '#f4f0e0');
      }
    }
    // 넘치는 CD는 바닥에 탑으로
    let sx = 62;
    while (discs > 0 && sx < 92) {
      const n = Math.min(discs, 26);
      for (let i = 0; i < n; i++) { R(ctx, sx, 127 - i, 8, 1, i % 2 ? '#d8dce8' : '#b8c0d0'); }
      R(ctx, sx, 127 - n, 8, 1, '#ffffff'); R(ctx, sx + 3, 127 - n, 2, 1, '#8a8a9a');
      dither(ctx, sx + 8, 128 - n, 1, n, '#000000', 0.4);
      discs -= n; sx += 10;
    }
  }

  function drawDesk(ctx) {
    ctx.fillStyle = 'rgba(40,20,10,0.28)'; ctx.fillRect(92, 99, 88, 16); // 책상 아래 그림자
    R(ctx, 90, 77, 90, 4, WOOD.base); R(ctx, 90, 77, 90, 1, WOOD.hi);
    R(ctx, 90, 81, 90, 3, WOOD.dark); R(ctx, 90, 84, 90, 1, WOOD.deep);
    R(ctx, 92, 85, 4, 30, WOOD.mid); R(ctx, 95, 85, 1, 30, WOOD.dark);
    R(ctx, 174, 85, 4, 30, WOOD.mid); R(ctx, 177, 85, 1, 30, WOOD.dark);
    // 서랍장
    R(ctx, 150, 85, 24, 20, WOOD.base); R(ctx, 150, 85, 1, 20, WOOD.hi);
    R(ctx, 151, 94, 22, 1, WOOD.dark); R(ctx, 151, 104, 22, 1, WOOD.dark);
    R(ctx, 159, 89, 6, 2, '#d8c090'); R(ctx, 159, 98, 6, 2, '#d8c090');
  }

  function drawLamp(ctx) {
    R(ctx, 95, 74, 10, 3, '#2e4a78'); R(ctx, 95, 74, 10, 1, '#4a6aa0');
    R(ctx, 99, 60, 1, 14, '#3a3a4a'); R(ctx, 99, 60, 6, 1, '#3a3a4a');
    R(ctx, 102, 54, 8, 2, '#2e4a78'); R(ctx, 101, 56, 10, 3, '#2e4a78'); R(ctx, 101, 56, 10, 1, '#4a6aa0');
    R(ctx, 103, 59, 6, 1, '#fff2c0');
    // 따뜻한 불빛
    ctx.fillStyle = 'rgba(255,220,140,0.16)';
    ctx.beginPath(); ctx.moveTo(101, 59); ctx.lineTo(111, 59); ctx.lineTo(120, 77); ctx.lineTo(92, 77); ctx.closePath(); ctx.fill();
  }

  function drawPC(ctx, s) {
    // CRT 몸체
    R(ctx, 109, 46, 38, 31, BEIGE.dark); // 뒤쪽 깊이
    R(ctx, 107, 44, 38, 32, BEIGE.base);
    R(ctx, 107, 44, 38, 1, BEIGE.hi); R(ctx, 107, 44, 1, 32, BEIGE.hi);
    R(ctx, 144, 45, 1, 31, BEIGE.mid); R(ctx, 108, 75, 37, 1, BEIGE.mid);
    R(ctx, 110, 47, 32, 25, '#2a2a2a'); R(ctx, 110, 47, 32, 1, '#1a1a1a');
    R(ctx, 113, 73, 10, 1, BEIGE.dark); R(ctx, 138, 73, 2, 1, '#40c060');
    // 받침
    R(ctx, 117, 76, 20, 2, BEIGE.mid);
    // 키보드
    R(ctx, 111, 78, 31, 4, BEIGE.base); R(ctx, 111, 78, 31, 1, BEIGE.hi); R(ctx, 111, 81, 31, 1, BEIGE.dark);
    for (let xx = 112; xx < 141; xx += 2) { R(ctx, xx, 79, 1, 1, BEIGE.dark); R(ctx, xx + 1, 80, 1, 1, BEIGE.dark); }
    // 마우스와 패드
    R(ctx, 143, 79, 6, 3, '#3a5a8a'); R(ctx, 145, 79, 2, 2, BEIGE.base); R(ctx, 145, 79, 2, 1, BEIGE.hi);
    // 본체
    R(ctx, 152, 47, 19, 30, BEIGE.dark);
    R(ctx, 150, 45, 19, 31, BEIGE.base);
    R(ctx, 150, 45, 19, 1, BEIGE.hi); R(ctx, 150, 45, 1, 31, BEIGE.hi); R(ctx, 168, 46, 1, 30, BEIGE.mid);
    R(ctx, 152, 49, 15, 4, BEIGE.mid); R(ctx, 152, 51, 15, 1, BEIGE.dark);
    if (s.pc.burner) { R(ctx, 152, 55, 15, 4, BEIGE.mid); R(ctx, 152, 57, 15, 1, BEIGE.dark); R(ctx, 164, 56, 2, 1, s.pc.driveBroken ? '#c03030' : '#e09030'); }
    R(ctx, 154, 62, 8, 1, '#3a3a3a'); // 플로피
    disc(ctx, 159, 69, 1, BEIGE.dark);
    for (let yy = 72; yy < 76; yy += 2) R(ctx, 152, yy, 15, 1, BEIGE.mid);
    // 머그컵
    R(ctx, 172, 71, 5, 6, '#e8e8ec'); R(ctx, 172, 71, 5, 1, '#5a3a22'); R(ctx, 177, 72, 1, 3, '#c8c8d0'); R(ctx, 176, 76, 1, 1, '#b8b8c0');
  }

  function drawScreen(ctx, s, t) {
    const sx = 112, sy = 49, sw = 28, sh = 21;
    const mode = s.screen || 'desktop';
    if (mode === 'off') {
      R(ctx, sx, sy, sw, sh, '#1a2222');
      R(ctx, sx + 2, sy + 2, 8, 1, '#3a4848'); R(ctx, sx + 2, sy + 3, 4, 1, '#2e3c3c');
      return;
    }
    // 바탕화면: 노을 그라데이션 + 언덕 + 구름
    gradV(ctx, sx, sy, sw, sh - 3, ['#3a6ab8', '#6a9ad8', '#e8b8a0']);
    R(ctx, sx, sy + sh - 7, sw, 4, '#4a8a5a'); R(ctx, sx + 4, sy + sh - 8, 12, 1, '#4a8a5a'); R(ctx, sx + 8, sy + sh - 9, 6, 1, '#4a8a5a');
    R(ctx, sx + 16, sy + 4, 6, 2, '#ffffff'); R(ctx, sx + 17, sy + 3, 3, 1, '#ffffff');
    // 아이콘
    for (const [iy, c] of [[2, '#f0f0f0'], [7, '#f2d24a'], [12, '#a0c8f0']]) { R(ctx, sx + 2, sy + iy, 3, 3, c); R(ctx, sx + 2, sy + iy + 3, 3, 1, '#ffffff66'); }
    // 작업 표시줄
    R(ctx, sx, sy + sh - 3, sw, 3, '#c0c0c0'); R(ctx, sx, sy + sh - 3, sw, 1, '#ffffff');
    R(ctx, sx + 1, sy + sh - 2, 5, 1, '#3a8a3a'); R(ctx, sx + sw - 6, sy + sh - 2, 5, 1, '#808080');
    if (mode === 'download' || mode === 'burn') {
      R(ctx, sx + 7, sy + 4, 19, 11, '#c0c0c0'); R(ctx, sx + 7, sy + 4, 19, 1, '#ffffff');
      R(ctx, sx + 7, sy + 4, 19, 2, '#000080');
      R(ctx, sx + 9, sy + 9, 15, 3, '#ffffff');
      const p = (t / 40) % 15;
      R(ctx, sx + 9, sy + 9, p | 0, 3, mode === 'burn' ? '#c03030' : '#000080');
    }
    // 화면 반사광과 주사선
    P(ctx, [[sx + 1, sy + 1], [sx + 2, sy + 1], [sx + 1, sy + 2]], '#ffffff88');
    for (let yy = sy; yy < sy + sh; yy += 2) { ctx.fillStyle = 'rgba(0,0,0,0.07)'; ctx.fillRect(sx, yy, sw, 1); }
  }

  function drawBoy(ctx, s, t, future) {
    const d = dateOf(Math.min(s.turn, 35));
    const season = seasonOf(d.m);
    const shirt = future ? ['#5a6a7a', '#4a5866', '#6e7e8e'] : season === 'winter' ? ['#2a3a6a', '#1e2a50', '#3a4c80'] : season === 'summer' ? ['#f4f4f4', '#cfd4dc', '#ffffff'] : ['#e8ecf4', '#c4cad8', '#f8faff'];
    // 의자
    R(ctx, 128, 117, 3, 9, '#2a2a3a');
    R(ctx, 118, 126, 23, 2, '#2a2a3a'); P(ctx, [[118, 128], [140, 128], [129, 128]], '#1a1a22');
    R(ctx, 114, 92, 30, 26, '#33334a');
    R(ctx, 115, 91, 28, 1, '#44445e'); R(ctx, 113, 93, 1, 23, '#2a2a3c'); R(ctx, 144, 93, 1, 23, '#2a2a3c');
    R(ctx, 116, 94, 26, 22, '#4a4a66');
    dither(ctx, 116, 94, 26, 22, '#5a5a7a', 0.25);
    R(ctx, 116, 94, 1, 22, '#6a6a88');
    // 몸 (숨쉬기)
    const bob = (t / 1400) % 2 < 1 ? 0 : 1;
    const by = 87 + bob;
    R(ctx, 117, by, 24, 9, shirt[0]);
    R(ctx, 117, by, 24, 1, shirt[2]); R(ctx, 129, by + 1, 1, 8, shirt[1]); R(ctx, 117, by + 6, 24, 3, shirt[1]);
    R(ctx, 113, by + 2, 5, 8, shirt[0]); R(ctx, 140, by + 2, 5, 8, shirt[0]);
    R(ctx, 113, by + 2, 1, 8, shirt[1]); R(ctx, 144, by + 2, 1, 8, shirt[1]);
    // 목, 머리
    R(ctx, 126, by - 3, 6, 4, '#e2b08c'); R(ctx, 126, by - 1, 6, 1, '#c8946e');
    const hair = future ? '#2a2422' : '#1d1a1a', hairHi = '#3a3440';
    const hy = 72 + bob;
    R(ctx, 124, hy, 10, 1, hair); R(ctx, 122, hy + 1, 14, 1, hair);
    R(ctx, 121, hy + 2, 16, 8, hair); R(ctx, 122, hy + 10, 14, 2, hair); R(ctx, 124, hy + 12, 10, 1, hair);
    P(ctx, [[126, hy + 1], [127, hy + 1], [125, hy + 2], [131, hy + 2], [132, hy + 3], [124, hy + 4]], hairHi);
    if (s.screen !== 'off') P(ctx, [[129, hy], [130, hy], [128, hy + 1]], '#5a7aa8'); // 모니터 빛이 머리에
    R(ctx, 120, hy + 5, 1, 4, '#e2b08c'); R(ctx, 137, hy + 5, 1, 4, '#e2b08c');
    P(ctx, [[129, hy - 1], [130, hy - 2]], hair); // 삐친 머리
    if (future) P(ctx, [[124, hy + 3], [133, hy + 4], [129, hy + 2]], '#8a8a8a');
    // 헤드폰
    if (!future) {
      R(ctx, 122, hy - 1, 14, 1, '#2a2a2a'); R(ctx, 121, hy, 1, 2, '#2a2a2a'); R(ctx, 136, hy, 1, 2, '#2a2a2a');
      R(ctx, 119, hy + 3, 3, 6, '#2a2a2a'); R(ctx, 136, hy + 3, 3, 6, '#2a2a2a');
      R(ctx, 119, hy + 3, 1, 6, '#4a4a4a'); R(ctx, 138, hy + 3, 1, 6, '#4a4a4a');
      R(ctx, 139, hy + 8, 1, 3, '#2a2a2a'); R(ctx, 140, hy + 10, 1, 6, '#2a2a2a');
    }
  }

  // 정적인 부분은 상태가 바뀔 때만 다시 그린다
  let cache = null, cacheKey = '';
  function staticKey(s, future, season, tr) {
    let nb = 0;
    for (const id in s.col) nb += s.col[id].phys || 0;
    return [future, season, dateOf(Math.min(s.turn, 35)).m, s.stats.discs, nb, tr.top, tr.total > 120, s.pc.burner, s.pc.driveBroken,
      Math.floor((s.friend || 0) / 30), s.love >= 50].join('|');
  }

  function drawRoom(ctx, s, t = 0, opt = {}) {
    const future = !!opt.future;
    const d = dateOf(Math.min(s.turn, 35));
    const season = seasonOf(d.m);
    const tr = computeTraits(s);
    const key = staticKey(s, future, season, tr);
    if (!cache) { cache = document.createElement('canvas'); cache.width = W; cache.height = H; }
    if (key !== cacheKey) {
      const c = cache.getContext('2d');
      c.clearRect(0, 0, W, H);
      drawWall(c, future);
      drawWindowStatic(c, season, future);
      // 창으로 들어오는 빛
      c.fillStyle = future || season === 'winter' ? 'rgba(150,170,255,0.07)' : 'rgba(255,214,150,0.13)';
      c.beginPath(); c.moveTo(8, 58); c.lineTo(58, 58); c.lineTo(112, 135); c.lineTo(36, 135); c.closePath(); c.fill();
      drawPoster(c, s, tr);
      drawCalendar(c, d.m, future);
      drawClock(c);
      drawCorkBoard(c, s, tr);
      drawFloor(c);
      drawRack(c, s);
      drawDesk(c);
      if (future) drawFutureDesk(c); else { drawLamp(c); drawPC(c, s); }
      // 저녁 분위기
      c.fillStyle = future ? 'rgba(10,14,40,0.30)' : season === 'winter' ? 'rgba(30,30,80,0.16)' : 'rgba(255,160,80,0.05)';
      c.fillRect(0, 0, W, H);
      // 가장자리 비네팅
      dither(c, 0, 0, W, 3, '#00000044', 0.5); dither(c, 0, H - 3, W, 3, '#00000044', 0.5);
      dither(c, 0, 0, 2, H, '#00000044', 0.5); dither(c, W - 2, 0, 2, H, '#00000044', 0.5);
      cacheKey = key;
    }
    ctx.imageSmoothingEnabled = false;
    ctx.drawImage(cache, 0, 0);
    drawWindowAnim(ctx, future ? 'future' : season, t, future);
    if (future) drawFutureScreen(ctx, t);
    else {
      drawScreen(ctx, s, t);
      const mode = s.screen || 'desktop';
      R(ctx, 158, 70, 2, 1, (mode !== 'off' && (t / 250) % 2 < 1) ? '#40e070' : '#206030');
      if (mode !== 'off') { ctx.fillStyle = 'rgba(120,200,255,0.07)'; ctx.fillRect(104, 42, 46, 60); }
    }
    drawBoy(ctx, s, t, future);
  }

  // 2026년 책상: 얇은 모니터, NAS, 스마트폰
  function drawFutureDesk(ctx) {
    R(ctx, 102, 40, 48, 30, '#1a1a1e'); R(ctx, 102, 40, 48, 1, '#3a3a40');
    R(ctx, 122, 70, 8, 6, '#2a2a2e'); R(ctx, 114, 76, 24, 1, '#3a3a40');
    R(ctx, 108, 78, 34, 2, '#3a3a40'); R(ctx, 108, 78, 34, 1, '#5a5a62');
    R(ctx, 154, 57, 15, 20, '#2a2a30'); R(ctx, 154, 57, 15, 1, '#4a4a52');
    for (let i = 0; i < 4; i++) R(ctx, 156, 60 + i * 4, 10, 2, '#40404a');
    R(ctx, 144, 76, 5, 1, '#1a1a1a'); R(ctx, 144, 75, 5, 1, '#4a6aa0');
    R(ctx, 172, 71, 5, 6, '#e8e8ec'); R(ctx, 177, 72, 1, 3, '#c8c8d0');
  }
  function drawFutureScreen(ctx, t) {
    gradV(ctx, 104, 42, 44, 26, ['#1e3a7a', '#3a6ab8', '#8ab0e0']);
    R(ctx, 108, 47, 16, 12, '#d8e4f4'); R(ctx, 109, 48, 14, 7, '#7a9ac8');
    R(ctx, 127, 47, 18, 3, '#d8e4f4'); R(ctx, 127, 52, 14, 2, '#d8e4f4'); R(ctx, 127, 56, 16, 2, '#d8e4f4');
    for (let i = 0; i < 4; i++) R(ctx, 165, 61 + i * 4, 1, 1, (t / 300 + i) % 3 < 1.5 ? '#40e070' : '#208040');
  }

  // 캔버스 글자의 흐린 가장자리를 없애 도트처럼 또렷하게
  function crispText(ctx, text, x, y, color, align = 'left') {
    const tw = W, th = 14;
    const off = document.createElement('canvas'); off.width = tw; off.height = th;
    const o = off.getContext('2d');
    o.font = FONT; o.textBaseline = 'top'; o.fillStyle = '#000';
    const mw = Math.ceil(o.measureText(text).width);
    o.fillText(text, 0, 1);
    const d = o.getImageData(0, 0, tw, th);
    const [r, g, b] = hex2rgb(color);
    for (let i = 0; i < d.data.length; i += 4) { const a = d.data[i + 3] >= 110 ? 255 : 0; d.data[i] = r; d.data[i + 1] = g; d.data[i + 2] = b; d.data[i + 3] = a; }
    o.putImageData(d, 0, 0);
    const dx = align === 'center' ? Math.round(x - mw / 2) : x;
    ctx.drawImage(off, dx, y - 1);
    return { x: dx, w: mw };
  }

  // ---------- 타이틀 ----------
  // 모니터 옆으로 그동안 모은 것들: CD 장, 만화책 책장, 게임 박스, 바닥의 스핀들과 책 더미
  let titleCache = null, titleText = null, titleCursorX = 0;
  function drawTitle(ctx, t) {
    ctx.imageSmoothingEnabled = false;
    if (!titleCache) {
      titleCache = document.createElement('canvas'); titleCache.width = W; titleCache.height = H;
      const c = titleCache.getContext('2d');
      const r = rng(5);
      gradV(c, 0, 0, W, 100, ['#10101e', '#161628', '#1c1a30']);
      // 작은 창과 달
      R(c, 4, 5, 28, 30, '#2a2a40'); gradV(c, 6, 7, 24, 26, SKY.winter);
      disc(c, 22, 13, 3, '#f4f0d8'); disc(c, 24, 12, 3, SKY.winter[0]);
      for (let i = 0; i < 8; i++) R(c, 6 + ((r() * 24) | 0), 7 + ((r() * 14) | 0), 1, 1, '#ffffff');
      drawSkyline(c, 6, 23, 24, 10, '#0e1226', '#ffd27a', 12, '#1a1e38');
      R(c, 17, 5, 2, 30, '#2a2a40'); R(c, 4, 19, 28, 2, '#2a2a40');

      // 왼쪽: CD 장 (꽉 찬 케이스들)
      R(c, 3, 40, 32, 58, WOOD.mid); R(c, 3, 40, 32, 1, WOOD.hi); R(c, 33, 40, 2, 58, WOOD.dark);
      R(c, 5, 42, 28, 55, WOOD.deep);
      for (let row = 0; row < 4; row++) {
        const ry = 43 + row * 14;
        for (let x = 6; x < 32; x++) {
          const col = CD_COLORS[(r() * CD_COLORS.length) | 0];
          R(c, x, ry, 1, 11, col); R(c, x, ry, 1, 1, x % 2 ? '#ffffff' : '#d8dce8');
          if (x % 3 === 0) R(c, x, ry + 5, 1, 3, '#4a4a5a');
        }
        R(c, 5, ry + 11, 28, 2, WOOD.base); R(c, 5, ry + 11, 28, 1, WOOD.hi);
      }

      // 오른쪽: 만화책 책장
      R(c, 138, 22, 40, 76, WOOD.mid); R(c, 138, 22, 40, 1, WOOD.hi); R(c, 176, 22, 2, 76, WOOD.dark);
      R(c, 140, 24, 36, 73, WOOD.deep);
      const MANGA = [['#e04a3a', '#a83020'], ['#3a7ad0', '#2a5aa0'], ['#f0c040', '#c09020'], ['#4ab060', '#2e8040'], ['#f4f0e8', '#c8c0b0'], ['#a060c0', '#7a40a0'], ['#f08ab0', '#c06088']];
      for (let row = 0; row < 4; row++) {
        const ry = 25 + row * 18;
        let x = 141;
        let series = (r() * MANGA.length) | 0;
        while (x < 174) {
          if (r() < 0.25) series = (r() * MANGA.length) | 0; // 같은 시리즈가 몇 권씩 이어진다
          const [c1, c2] = MANGA[series];
          const hgt = 13 + (series % 3);
          R(c, x, ry + 16 - hgt, 2, hgt, c1); R(c, x + 1, ry + 16 - hgt, 1, hgt, c2);
          R(c, x, ry + 18 - hgt, 2, 1, '#ffffffaa'); // 권수 띠
          x += 2;
          if (r() < 0.06 && x < 170) { R(c, x, ry + 6, 1, 10, '#00000000'); x += 1; }
        }
        R(c, 140, ry + 16, 36, 2, WOOD.base); R(c, 140, ry + 16, 36, 1, WOOD.hi);
      }
      // 책장 위 게임 박스들
      const boxes = [[139, 8, 11, 14, '#2a4a8a', '#f0c040'], [151, 11, 10, 11, '#8a2a2a', '#f4f0e8'], [162, 6, 13, 16, '#1a1a2a', '#40c0e0']];
      for (const [bx, by, bw, bh, c1, c2] of boxes) {
        R(c, bx, by, bw, bh, c1); R(c, bx, by, bw, 1, '#ffffff55'); R(c, bx + bw - 1, by, 1, bh, '#00000055');
        R(c, bx + 2, by + 2, bw - 4, 4, c2); R(c, bx + 2, by + bh - 4, bw - 4, 1, c2);
        disc(c, bx + (bw >> 1), by + (bh >> 1) + 2, 2, '#ffffff44');
      }

      // 책상
      R(c, 0, 98, W, 6, '#3a2a22'); R(c, 0, 98, W, 1, '#5a4030');
      gradV(c, 0, 104, W, 31, ['#24182a', '#1a1220']);
      // CRT (가운데)
      R(c, 52, 22, 82, 76, '#8a826c');
      R(c, 48, 20, 82, 76, '#cfc6ae'); R(c, 48, 20, 82, 1, '#ece4d0'); R(c, 48, 20, 1, 76, '#ece4d0');
      R(c, 129, 21, 1, 75, '#aea38a'); R(c, 49, 95, 81, 1, '#aea38a');
      R(c, 53, 25, 72, 60, '#222222');
      R(c, 58, 89, 18, 2, '#aea38a');
      // 모니터 앞 책상 위: 투명 케이스 더미, 키보드 대신 CD 한 장
      for (let i = 0; i < 4; i++) { R(c, 36, 94 - i * 2, 11, 2, i % 2 ? '#c8d8e8' : '#a8c0d8'); R(c, 36, 94 - i * 2, 1, 2, '#ffffff'); }
      R(c, 132, 92, 6, 6, '#f0c040'); R(c, 132, 92, 6, 1, '#fff0a0'); R(c, 133, 94, 4, 2, '#c08020'); // 만화책 한 권

      // 바닥: CD 스핀들, 흩어진 CD, 만화책 더미, 게임 박스
      R(c, 146, 112, 20, 18, '#d8dce8'); R(c, 146, 112, 20, 1, '#ffffff');
      for (let i = 0; i < 9; i++) R(c, 148, 128 - i * 2, 16, 1, '#a8b0c8');
      R(c, 145, 110, 22, 3, '#4a4a5a'); R(c, 154, 107, 4, 3, '#4a4a5a');
      for (const [cx, cy] of [[20, 122], [34, 128]]) { disc(c, cx, cy, 6, '#d8dce8'); disc(c, cx, cy, 2, '#24182a'); P(c, [[cx - 3, cy - 4], [cx - 2, cy - 5]], '#ffffff'); disc(c, cx + 3, cy + 2, 1, '#e8a0c0'); }
      for (let k = 0; k < 7; k++) { const [c1, c2] = MANGA[(k * 3) % MANGA.length]; R(c, 64 + (k % 2), 130 - k * 3, 14, 3, c1); R(c, 64 + (k % 2), 132 - k * 3, 14, 1, c2); R(c, 77 + (k % 2), 130 - k * 3, 1, 2, '#f4f0e0'); }
      R(c, 100, 116, 16, 18, '#2a4a8a'); R(c, 100, 116, 16, 1, '#5a7ac0'); R(c, 102, 118, 12, 5, '#f0c040'); R(c, 102, 126, 12, 1, '#f0c040'); R(c, 115, 116, 1, 18, '#1a2a5a');
    }
    ctx.drawImage(titleCache, 0, 0);
    // 화면
    gradV(ctx, 56, 28, 66, 54, ['#08204a', '#0e2e66', '#183a7a']);
    if (!titleText) {
      titleText = document.createElement('canvas'); titleText.width = W; titleText.height = H;
      const tc = titleText.getContext('2d');
      crispText(tc, '수집왕', 90, 40, '#000a20', 'center');
      crispText(tc, '수집왕', 89, 39, '#ffe680', 'center');
      const sub = crispText(tc, '2000-2003', 87, 57, '#9ad0ff', 'center');
      titleCursorX = sub.x + sub.w + 1;
    }
    ctx.drawImage(titleText, 0, 0);
    if ((t / 500) % 2 < 1) R(ctx, titleCursorX, 57, 4, 9, '#9ad0ff');
    for (let y = 28; y < 82; y += 2) { ctx.fillStyle = 'rgba(0,0,0,0.18)'; ctx.fillRect(56, y, 66, 1); }
    P(ctx, [[57, 29], [58, 29], [59, 29], [57, 30], [57, 31]], '#ffffff66');
    R(ctx, 118, 89, 3, 2, (t / 700) % 2 < 1 ? '#40e070' : '#30a050');
    // 모니터 불빛이 책상에 번진다
    ctx.fillStyle = 'rgba(80,140,255,0.10)'; ctx.fillRect(40, 98, 98, 3);
  }

  // ---------- 화면 아래쪽 배경화면 ----------
  // 계절마다 바뀌는 노을 진 아파트 단지와 전봇대 풍경
  const wallCache = {};
  function wallpaper(kind) {
    if (wallCache[kind]) return wallCache[kind];
    const cw = 110, ch = 200;
    const cv = document.createElement('canvas'); cv.width = cw; cv.height = ch;
    const c = cv.getContext('2d');
    const night = kind === 'winter' || kind === 'title' || kind === 'future';
    const pal = {
      spring: ['#6a84c0', '#9aa8d8', '#d8b4c8', '#f2c4b0', '#f8d8b8'],
      summer: ['#2e6cc0', '#4a8ad8', '#7ab0e8', '#b8d8f0', '#e8f0e0'],
      autumn: ['#33407a', '#6a5a98', '#c07a80', '#e89a68', '#f4c080'],
      winter: ['#0c1430', '#16224a', '#26346a', '#3a4a80', '#5a6496'],
      title: ['#06081a', '#0c1028', '#141a3a', '#1e2650', '#2a3060'],
      future: ['#06081a', '#0c1028', '#141a3a', '#1e2650', '#2a3060'],
    }[kind];
    gradV(c, 0, 0, cw, ch, pal);
    const r = rng(kind.length * 31 + 7);
    if (night) {
      for (let i = 0; i < 60; i++) { const x = (r() * cw) | 0, y = (r() * ch * 0.6) | 0; R(c, x, y, 1, 1, r() > 0.7 ? '#ffffff' : '#8a96c8'); }
      disc(c, 80, 30, 6, '#f4f0d8'); disc(c, 83, 28, 6, pal[0]);
    } else {
      const sunY = kind === 'summer' ? 40 : 120;
      c.fillStyle = kind === 'summer' ? 'rgba(255,250,220,0.18)' : 'rgba(255,224,160,0.22)';
      for (const rr of [22, 16, 12]) { c.beginPath(); c.arc(75, sunY, rr, 0, Math.PI * 2); c.fill(); }
      disc(c, 75, sunY, 9, kind === 'summer' ? '#fffbe8' : '#ffd890'); disc(c, 75, sunY, 6, '#fff4d0');
    }
    // 구름
    const cloudC = night ? '#2a3460' : kind === 'autumn' ? '#e8a890' : kind === 'spring' ? '#f8e4e8' : '#ffffff';
    const cloudS = night ? '#1e2650' : kind === 'autumn' ? '#c07a78' : kind === 'spring' ? '#d8b8c8' : '#c8dcf0';
    for (const [cx, cy, n] of [[20, 50, 4], [70, 85, 5], [30, 115, 3]]) {
      for (let i = 0; i < n; i++) disc(c, cx + i * 6, cy - (i % 2) * 2, 4 + (i % 2), cloudC);
      R(c, cx - 3, cy + 3, n * 6 + 4, 2, cloudS);
    }
    // 새
    if (!night) for (const [bx, by] of [[30, 70], [38, 66], [44, 72]]) P(c, [[bx, by], [bx + 1, by + 1], [bx + 2, by], [bx + 3, by - 1], [bx - 1, by - 1]], '#3a3a5a');
    // 먼 산
    c.fillStyle = night ? '#141a38' : kind === 'summer' ? '#5a86a8' : '#7a6a90';
    for (let x = 0; x < cw; x++) { const hh = 14 + Math.sin(x / 14) * 6 + Math.sin(x / 5) * 2; c.fillRect(x, 150 - hh, 1, hh + 10); }
    // 아파트 단지 (두 겹)
    drawSkyline(c, 0, 140, cw, 30, night ? '#1a2040' : kind === 'summer' ? '#6a7a98' : '#5e5070', '#ffd27a', 21, night ? '#262c50' : '#7a6a88');
    drawSkyline(c, -4, 158, cw + 8, 42, night ? '#0e1228' : kind === 'summer' ? '#46566e' : '#3e3450', '#ffe08a', 99, night ? '#1a1e3a' : '#54486a');
    // 전봇대와 전깃줄
    R(c, 18, 120, 2, 80, '#2a2028'); R(c, 13, 126, 12, 1, '#2a2028'); R(c, 15, 130, 8, 1, '#2a2028');
    c.fillStyle = '#2a202888';
    for (let x = 0; x < cw; x++) { const sag = Math.sin((x / cw) * Math.PI) * 6; c.fillRect(x, 126 + sag - (x < 18 ? (18 - x) * 0.1 : 0), 1, 1); c.fillRect(x, 131 + sag * 0.8, 1, 1); }
    // 계절 장식
    if (kind === 'spring') for (let i = 0; i < 40; i++) R(c, (r() * cw) | 0, (r() * 170) | 0, 1, 1, r() > 0.5 ? '#ffe0ea' : '#f4a8c0');
    if (kind === 'autumn') for (let i = 0; i < 4; i++) { const x = 20 + i * 22, y = 60 + (i % 2) * 20; R(c, x, y, 4, 1, '#b83a2a'); R(c, x + 1, y - 1, 1, 1, '#ffffffaa'); R(c, x + 3, y - 1, 1, 1, '#ffffffaa'); }
    if (kind === 'winter') for (let i = 0; i < 70; i++) R(c, (r() * cw) | 0, (r() * ch) | 0, 1, 1, '#ffffff');
    wallCache[kind] = cv.toDataURL();
    return wallCache[kind];
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
    crispText(ctx, '디스크 조각 모음 - C:', 9, 9, '#ffffff');
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
    crispText(ctx, `완료: ${Math.floor(p * 100)}%`, 12, 114, '#000000');
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
    her(ctx, expr, bg) {
      bg('#f6d6dc');
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
    mom(ctx, expr, bg) {
      bg('#f2e2c4');
      R(ctx, 2, 33, 36, 7, '#c86a7a'); R(ctx, 12, 34, 16, 6, '#f6e8ea');
      const hc = '#5a3a2a';
      faceBase(ctx);
      for (const [x, y] of [[9, 8], [13, 6], [18, 5], [23, 6], [27, 8], [8, 13], [29, 13], [8, 18], [29, 18]]) R(ctx, x, y, 5, 5, hc);
      R(ctx, 11, 9, 18, 3, hc);
      P(ctx, [[10, 7], [16, 5], [24, 5], [30, 11]], '#7a5240');
      expression(ctx, expr);
    },
    dad(ctx, expr, bg) {
      bg('#d8e2ea');
      R(ctx, 2, 33, 36, 7, '#f4f4f4'); R(ctx, 19, 33, 3, 7, '#3a4a8a');
      faceBase(ctx);
      R(ctx, 11, 7, 18, 5, '#222222'); R(ctx, 11, 10, 2, 6, '#222222'); R(ctx, 27, 10, 2, 6, '#222222');
      P(ctx, [[14, 8], [20, 7]], '#6a6a6a');
      expression(ctx, expr);
      glasses(ctx, '#4a3a2a');
    },
    friend(ctx, expr, bg) {
      bg('#cfe8c8');
      R(ctx, 2, 33, 36, 7, '#2a5ab0'); R(ctx, 19, 33, 2, 7, '#f4f4f4');
      faceBase(ctx, '#d9a072');
      R(ctx, 12, 8, 16, 4, '#2a2a2a'); R(ctx, 12, 11, 1, 3, '#2a2a2a'); R(ctx, 27, 11, 1, 3, '#2a2a2a');
      P(ctx, [[14, 9], [18, 9], [22, 9], [25, 10]], '#4a4a4a');
      expression(ctx, expr === 'normal' ? 'smile' : expr);
      R(ctx, 26, 24, 2, 1, '#c07050'); // 반창고
    },
    hyung(ctx, expr, bg) {
      bg('#2c2c3c');
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
    owner(ctx, expr, bg) {
      bg('#e8dcc0');
      R(ctx, 2, 33, 36, 7, '#5a6a4a'); R(ctx, 12, 33, 16, 7, '#3a4a2a');
      faceBase(ctx, '#e0b08a');
      R(ctx, 10, 6, 20, 6, '#c83a2a'); R(ctx, 8, 11, 16, 2, '#a02a1a');
      R(ctx, 18, 7, 4, 3, '#f4f4f4');
      expression(ctx, expr === 'normal' ? 'smile' : expr);
      P(ctx, [[15, 27], [17, 28], [20, 28], [23, 28], [25, 27], [16, 26]], '#7a5a4a');
    },
  };

  function shade(hex, f) {
    const n = parseInt(hex.slice(1), 16);
    const ch = v => Math.max(0, Math.min(255, Math.round(v * f)));
    return '#' + [ch(n >> 16), ch((n >> 8) & 255), ch(n & 255)].map(v => v.toString(16).padStart(2, '0')).join('');
  }
  // 인물을 따로 그린 뒤 외곽선과 그림자를 입히고, 그라데이션 배경 위에 얹는다
  function drawFace(cv, who, expr = 'normal') {
    const ctx = cv.getContext('2d');
    ctx.imageSmoothingEnabled = false;
    const off = document.createElement('canvas'); off.width = 40; off.height = 40;
    const o = off.getContext('2d');
    let bgc = '#c8c8c8';
    (FACES[who] || FACES.friend)(o, expr, c => { bgc = c; });
    const src = o.getImageData(0, 0, 40, 40).data;
    const out = o.createImageData(40, 40), q = out.data;
    const A = (x, y) => (x < 0 || y < 0 || x > 39 || y > 39 ? 0 : src[(y * 40 + x) * 4 + 3]);
    for (let y = 0; y < 40; y++) for (let x = 0; x < 40; x++) {
      const i = (y * 40 + x) * 4;
      if (src[i + 3]) {
        // 오른쪽 아래는 살짝 어둡게 (빛은 왼쪽 위에서)
        const f = (!A(x + 1, y) || !A(x, y + 1)) ? 0.8 : x > 25 ? 0.9 : 1;
        q[i] = src[i] * f; q[i + 1] = src[i + 1] * f; q[i + 2] = src[i + 2] * f; q[i + 3] = 255;
      } else if (A(x - 1, y) || A(x + 1, y) || A(x, y - 1) || A(x, y + 1)) {
        q[i] = 30; q[i + 1] = 20; q[i + 2] = 26; q[i + 3] = 255;
      }
    }
    o.putImageData(out, 0, 0);
    gradV(ctx, 0, 0, 40, 40, [shade(bgc, 1.08), bgc, shade(bgc, 0.82)]);
    for (let y = 3; y < 40; y += 6) for (let x = (y % 12 === 3 ? 3 : 6); x < 40; x += 6) R(ctx, x, y, 1, 1, shade(bgc, 0.92));
    ctx.drawImage(off, 0, 0);
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

  return { W, H, drawRoom, drawTitle, drawFace, drawIcon, makeDefrag, drawDefrag, seasonOf, wallpaper };
})();
