// 웹페이지·홈 화면 아이콘을 도트로 그려 assets/icons/에 PNG로 저장한다.
// 36x36 도트 그림을 정수배로 키운다 (180 = 5배: 아이폰 홈 화면 규격).
// 사용법: node tools/make-icons.js   (Playwright 크로미움 사용)
'use strict';
const fs = require('fs');
const path = require('path');
const { chromium } = require('playwright');

const OUT = path.join(__dirname, '..', 'assets', 'icons');

// 브라우저 안에서 실행되는 그리기 코드
function drawIcon() {
  const N = 36;
  const R = (c, x, y, w, h, col) => { c.fillStyle = col; c.fillRect(x, y, w, h); };
  const BAYER = [0, 8, 2, 10, 12, 4, 14, 6, 3, 11, 1, 9, 15, 7, 13, 5];
  const disc = (c, cx, cy, r, col) => { c.fillStyle = col; for (let y = -r; y <= r; y++) for (let x = -r; x <= r; x++) if (x * x + y * y <= r * r + r * 0.6) c.fillRect(cx + x, cy + y, 1, 1); };

  // 배경: 저녁 하늘 그라데이션 (디더링)
  const bg = document.createElement('canvas'); bg.width = N; bg.height = N;
  const b = bg.getContext('2d');
  const sky = ['#1c2650', '#2e3a74', '#5a4a8a', '#a8607e', '#e48a62', '#f4b070'];
  const hex = h => [1, 3, 5].map(i => parseInt(h.slice(i, i + 2), 16));
  for (let y = 0; y < N; y++) {
    const t = y / (N - 1) * (sky.length - 1), i = Math.min(sky.length - 2, Math.floor(t)), f = t - i;
    for (let x = 0; x < N; x++) {
      const use = (BAYER[(y & 3) * 4 + (x & 3)] + 0.5) / 16 < f ? sky[i + 1] : sky[i];
      R(b, x, y, 1, 1, use);
    }
  }
  for (const [x, y] of [[4, 4], [30, 3], [26, 8], [8, 11], [33, 13], [2, 16]]) R(b, x, y, 1, 1, '#ffffff');
  // 지평선 아파트 실루엣
  for (const [x, w, h] of [[0, 5, 6], [5, 4, 9], [10, 6, 5], [27, 4, 8], [31, 5, 6]]) {
    R(b, x, N - h, w, h, '#2a2244');
    for (let wy = N - h + 2; wy < N - 1; wy += 2) for (let wx = x + 1; wx < x + w - 1; wx += 2) if ((wx * 7 + wy * 3) % 5 < 2) R(b, wx, wy, 1, 1, '#ffd27a');
  }

  // 그림 (투명 바탕 위) → 외곽선을 입힌다
  const fg = document.createElement('canvas'); fg.width = N; fg.height = N;
  const c = fg.getContext('2d');
  // 플로피 디스크 (왼쪽 아래)
  R(c, 2, 22, 10, 11, '#3a6ab0'); R(c, 2, 22, 10, 1, '#5a8ad0'); R(c, 11, 23, 1, 10, '#2a4a80');
  R(c, 4, 22, 6, 4, '#c8ccd8'); R(c, 7, 23, 2, 2, '#3a6ab0');
  R(c, 4, 28, 6, 4, '#f4f0e8'); R(c, 5, 29, 4, 1, '#c8c0b0');
  // CD (가운데)
  const cx = 17, cy = 20;
  disc(c, cx, cy, 11, '#c8ccd8');
  disc(c, cx, cy, 10, '#e4e8f0');
  // 무지개 반사: 오른쪽 위와 왼쪽 아래에 고리 모양으로
  const band = ['#c0a8f0', '#90c8f0', '#a0e0b0', '#f0d080', '#f4a0c0'];
  for (let y = -10; y <= 10; y++) for (let x = -10; x <= 10; x++) {
    const d = Math.sqrt(x * x + y * y), a = Math.atan2(y, x) * 180 / Math.PI;
    if (d < 5 || d > 9.6) continue;
    const upRight = a > -80 && a < -15, downLeft = a > 100 && a < 165;
    if (upRight || downLeft) R(c, cx + x, cy + y, 1, 1, band[Math.min(4, Math.floor(d - 5))]);
  }
  disc(c, cx, cy, 4, '#a8b0c4'); disc(c, cx, cy, 3, '#d8dce8'); disc(c, cx, cy, 1, '#00000000');
  c.clearRect(cx - 1, cy - 1, 3, 3); R(c, cx - 1, cy, 3, 1, '#00000000');
  // 왕관 (CD 위)
  const ky = 3;
  R(c, 10, ky + 5, 15, 5, '#f0c040');
  R(c, 10, ky + 2, 3, 3, '#f0c040'); R(c, 16, ky, 3, 5, '#f0c040'); R(c, 22, ky + 2, 3, 3, '#f0c040');
  R(c, 11, ky + 1, 1, 1, '#f0c040'); R(c, 17, ky - 1, 1, 1, '#f0c040'); R(c, 23, ky + 1, 1, 1, '#f0c040');
  R(c, 10, ky + 5, 15, 1, '#fff0a0'); R(c, 17, ky, 1, 4, '#fff0a0');
  R(c, 10, ky + 9, 15, 1, '#b08020');
  R(c, 12, ky + 7, 2, 1, '#e04a5a'); R(c, 17, ky + 6, 2, 2, '#4a8ae0'); R(c, 21, ky + 7, 2, 1, '#e04a5a');
  // 만화책 (오른쪽 아래, 살짝 기울어진 느낌으로 계단)
  R(c, 24, 19, 10, 15, '#e04a3a'); R(c, 24, 19, 10, 1, '#f07a5a');
  R(c, 24, 19, 2, 15, '#a83020');
  R(c, 27, 21, 6, 3, '#f8f4e8'); R(c, 28, 22, 4, 1, '#3a3a4a');
  R(c, 27, 26, 6, 5, '#f4c0a0'); R(c, 29, 27, 2, 2, '#2a1e1e'); R(c, 28, 30, 4, 1, '#ffffff');
  R(c, 33, 20, 1, 14, '#f4f0e0');

  // 외곽선
  const src = c.getImageData(0, 0, N, N).data;
  const A = (x, y) => (x < 0 || y < 0 || x >= N || y >= N ? 0 : src[(y * N + x) * 4 + 3]);
  const out = c.createImageData(N, N);
  for (let y = 0; y < N; y++) for (let x = 0; x < N; x++) {
    const i = (y * N + x) * 4;
    if (src[i + 3]) { for (let k = 0; k < 4; k++) out.data[i + k] = src[i + k]; }
    else if (A(x - 1, y) || A(x + 1, y) || A(x, y - 1) || A(x, y + 1)) { out.data[i] = 26; out.data[i + 1] = 18; out.data[i + 2] = 30; out.data[i + 3] = 255; }
  }
  c.putImageData(out, 0, 0);
  b.drawImage(fg, 0, 0);

  const scaled = scale => {
    const cv = document.createElement('canvas'); cv.width = N * scale; cv.height = N * scale;
    const x = cv.getContext('2d'); x.imageSmoothingEnabled = false;
    x.drawImage(bg, 0, 0, N * scale, N * scale);
    return cv.toDataURL('image/png');
  };
  return { 1: scaled(1), 5: scaled(5), 10: scaled(10), 15: scaled(15) };
}

(async () => {
  fs.mkdirSync(OUT, { recursive: true });
  const browser = await chromium.launch();
  const page = await browser.newPage();
  const urls = await page.evaluate(drawIcon);
  const save = (name, url) => fs.writeFileSync(path.join(OUT, name), Buffer.from(url.split(',')[1], 'base64'));
  save('icon-36.png', urls[1]);
  save('apple-touch-icon.png', urls[5]);  // 180x180
  save('icon-360.png', urls[10]);
  save('icon-540.png', urls[15]);
  await browser.close();
  console.log('assets/icons/에 아이콘을 저장했습니다.');
})();
