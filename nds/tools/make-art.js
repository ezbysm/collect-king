// 웹판 도트 그림(js/art.js)으로 NDS용 그림을 만들어 C 배열로 저장한다.
//  - 위 화면(256x192): 계절별 방 4장 + 2026년 방, 타이틀
//  - 인물 초상화 6명 x 표정 6개 (40x40), 파일 종류 아이콘 (16x16)
// 색: 0x8000 | RGB555 (NDS 비트맵은 맨 윗비트가 1이어야 보임). 0 = 투명
// 사용법: (저장소 루트에서 정적 서버 실행 중) node nds/tools/make-art.js [http://localhost:8765/]
'use strict';
const fs = require('fs');
const path = require('path');
const { chromium } = require('playwright');

const URL = process.argv[2] || 'http://localhost:8765/';
const OUT = path.join(__dirname, '..', 'src', 'arm9');
const W = 256, H = 192, OX = 38, OY = 14;   // 방(180x135)을 놓는 자리. 위쪽 14줄은 상태 막대

(async () => {
  const browser = await chromium.launch();
  const page = await browser.newPage();
  await page.goto(URL);
  await page.waitForTimeout(900);
  const out = await page.evaluate(async ({ W, H, OX, OY }) => {
    const conv = (d, key) => {
      const a = [];
      for (let i = 0; i < d.length; i += 4) {
        if (key && d[i + 3] < 128) { a.push(0); continue; }
        a.push(0x8000 | (d[i] >> 3) | ((d[i + 1] >> 3) << 5) | ((d[i + 2] >> 3) << 10));
      }
      return a;
    };
    const canvas = (w, h) => { const c = document.createElement('canvas'); c.width = w; c.height = h; const x = c.getContext('2d'); x.imageSmoothingEnabled = false; return [c, x]; };
    const wallpaperInto = async (ctx, kind) => {
      const img = new Image(); img.src = Art.wallpaper(kind); await img.decode();
      const sh = Math.round(H * img.width / W);
      ctx.drawImage(img, 0, Math.max(0, img.height - sh), img.width, Math.min(sh, img.height), 0, 0, W, H);
    };
    const rooms = [];
    for (const [turn, kind, future] of [[0, 'spring', false], [4, 'summer', false], [7, 'autumn', false], [9, 'winter', false], [35, 'future', true]]) {
      G.s = newState('성민', '한빛중학교');
      G.s.turn = turn; G.s.friend = 0; G.s.love = 0; G.s.pc.burner = true; G.s.stats.discs = 0;
      const [c, x] = canvas(W, H);
      await wallpaperInto(x, kind);
      const [rc, rx] = canvas(180, 135);
      Art.drawRoom(rx, G.s, 1000, { future });
      if (!future) { rx.fillStyle = '#fdfdf8'; rx.fillRect(96, 20, 13, 6); }   // 달력 숫자는 롬이 그린다
      x.drawImage(rc, OX, OY);
      rooms.push(conv(x.getImageData(0, 0, W, H).data));
    }
    // 타이틀 (커서가 꺼진 순간과 켜진 순간을 비교해 커서 자리를 찾는다)
    const TY = 28;
    const [tc, tx] = canvas(W, H);
    await wallpaperInto(tx, 'title');
    const [a, ax] = canvas(180, 135); Art.drawTitle(ax, 600);
    const [b, bx] = canvas(180, 135); Art.drawTitle(bx, 0);
    const da = ax.getImageData(0, 0, 180, 135).data, db = bx.getImageData(0, 0, 180, 135).data;
    let cx0 = 999, cy0 = 999, cx1 = -1, cy1 = -1;
    for (let i = 0; i < da.length; i += 4) if (da[i] !== db[i] || da[i + 1] !== db[i + 1]) {
      const p = i / 4, X = p % 180, Y = (p / 180) | 0;
      if (Y > 40 && Y < 80) { cx0 = Math.min(cx0, X); cy0 = Math.min(cy0, Y); cx1 = Math.max(cx1, X); cy1 = Math.max(cy1, Y); }
    }
    tx.drawImage(a, OX, TY);
    const title = conv(tx.getImageData(0, 0, W, H).data);
    const cursor = [cx0 + OX, cy0 + TY, cx1 - cx0 + 1, cy1 - cy0 + 1];
    const who = ['her', 'mom', 'dad', 'friend', 'hyung', 'owner'];
    const exprs = ['normal', 'smile', 'blush', 'sad', 'surprise', 'angry'];
    const faces = [];
    for (const w of who) for (const e of exprs) { const [fc, fx] = canvas(40, 40); Art.drawFace(fc, w, e); faces.push(conv(fx.getImageData(0, 0, 40, 40).data)); }
    const icons = [];
    for (const t of ['anime', 'manga', 'novel', 'movie', 'music', 'game']) for (const done of [false, true]) {
      const [ic, ix] = canvas(16, 16); Art.drawIcon(ic, t, done); icons.push(conv(ix.getImageData(0, 0, 16, 16).data, true));
    }
    return { rooms, title, cursor, faces, icons };
  }, { W, H, OX, OY });
  await browser.close();

  const arr = (name, a) => {
    let t = `const unsigned short ${name}[${a.length}] __attribute__((aligned(4))) = {\n`;
    for (let i = 0; i < a.length; i += 24) t += '  ' + a.slice(i, i + 24).map(v => '0x' + v.toString(16)).join(',') + ',\n';
    return t + '};\n\n';
  };
  let c = '// 자동 생성: nds/tools/make-art.js — 웹판 js/art.js의 도트 그림 (0x8000|RGB555, 0 = 투명)\n#include "art_data.h"\n\n';
  out.rooms.forEach((r, i) => { c += arr(`ART_ROOM_${i}`, r); });
  c += 'const unsigned short *const ART_ROOMS[5] = { ART_ROOM_0, ART_ROOM_1, ART_ROOM_2, ART_ROOM_3, ART_ROOM_4 };\n\n';
  c += arr('ART_TITLE', out.title);
  c += arr('ART_FACES', [].concat(...out.faces));
  c += arr('ART_ICONS', [].concat(...out.icons));
  let h = '// 자동 생성: nds/tools/make-art.js\n#pragma once\n';
  h += 'extern const unsigned short *const ART_ROOMS[5];   // 봄, 여름, 가을, 겨울, 2026년 (256x192)\n';
  h += 'extern const unsigned short ART_TITLE[256 * 192];\n';
  h += 'extern const unsigned short ART_FACES[6 * 6 * 40 * 40];   // [인물 6][표정 6][40x40]\n';
  h += 'extern const unsigned short ART_ICONS[12 * 16 * 16];      // [종류 6][완결 0/1][16x16]\n';
  h += `#define ROOM_X ${OX}\n#define ROOM_Y ${OY}\n`;
  h += `#define TITLE_CURSOR_X ${out.cursor[0]}\n#define TITLE_CURSOR_Y ${out.cursor[1]}\n#define TITLE_CURSOR_W ${out.cursor[2]}\n#define TITLE_CURSOR_H ${out.cursor[3]}\n`;
  fs.writeFileSync(path.join(OUT, 'art_data.c'), c);
  fs.writeFileSync(path.join(OUT, 'art_data.h'), h);
  console.log(`방 5장, 타이틀(커서 ${out.cursor.join(',')}), 초상화 36장, 아이콘 12개 → nds/src/arm9/art_data.c`);
})();
