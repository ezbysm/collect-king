// 웹판의 도트 그래픽(js/art.js)으로 GBA 화면(240x160) 배경을 그려 RGB555 C 배열로 만든다.
// 사용법: (저장소 루트에서 정적 서버 실행 중) node gba/tools/make-bg.js [http://localhost:8765/]
'use strict';
const fs = require('fs');
const path = require('path');
const { chromium } = require('playwright');

const URL = process.argv[2] || 'http://localhost:8765/';
const OUT = path.join(__dirname, '..', 'src', 'bg_data.c');

(async () => {
  const browser = await chromium.launch();
  const page = await browser.newPage();
  await page.goto(URL);
  await page.waitForTimeout(800);
  const px = await page.evaluate(async () => {
    // 가을 저녁, 컬렉션이 어느 정도 쌓인 방
    G.s = newState('성민', '한빛중학교');
    G.s.turn = 19; G.s.friend = 70; G.s.love = 55; G.s.pc.burner = true; G.s.stats.discs = 70;
    for (const w of WORKS.slice(0, 40)) addUnits(G.s, w, Math.ceil(w.units / 2), 'cd', w.type === 'manga' ? 'book' : 'trade');
    const cv = document.createElement('canvas'); cv.width = 240; cv.height = 160;
    const c = cv.getContext('2d'); c.imageSmoothingEnabled = false;
    // 바탕: 가을 배경화면
    const img = new Image(); img.src = Art.wallpaper('autumn');
    await img.decode();
    const sh = Math.round(160 * img.width / 240); // 가로를 240에 맞추고 아래쪽(아파트 단지)만 잘라 쓴다
    c.drawImage(img, 0, img.height - sh, img.width, sh, 0, 0, 240, 160);
    // 방 장면(180x135)을 가운데 위에
    const room = document.createElement('canvas'); room.width = 180; room.height = 135;
    Art.drawRoom(room.getContext('2d'), G.s, 1000, {});
    c.drawImage(room, 30, 0);
    const conv = d => { const out = []; for (let i = 0; i < d.length; i += 4) out.push((d[i] >> 3) | ((d[i + 1] >> 3) << 5) | ((d[i + 2] >> 3) << 10)); return out; };
    // 그 아이 초상화 (40x40)
    const fc = document.createElement('canvas'); fc.width = 40; fc.height = 40;
    Art.drawFace(fc, 'her', 'smile');
    return { bg: conv(c.getImageData(0, 0, 240, 160).data), face: conv(fc.getContext('2d').getImageData(0, 0, 40, 40).data) };
  });
  await browser.close();
  const arr = (name, a) => {
    let t = `const unsigned short ${name}[${a.length}] __attribute__((aligned(4))) = {\n`;
    for (let i = 0; i < a.length; i += 16) t += '  ' + a.slice(i, i + 16).map(v => '0x' + v.toString(16)).join(', ') + ',\n';
    return t + '};\n\n';
  };
  let c = '// 자동 생성: gba/tools/make-bg.js — 웹판 js/art.js의 도트 그림 (RGB555)\n\n';
  c += arr('BG_ROOM', px.bg) + arr('FACE_HER', px.face);
  fs.writeFileSync(OUT, c);
  console.log('배경 240x160, 초상화 40x40 → gba/src/bg_data.c');
})();
