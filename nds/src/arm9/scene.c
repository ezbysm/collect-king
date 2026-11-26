// 수집왕 NDS — 위 화면 장면: 계절별 방 배경 위에 그때그때 바뀌는 것(CD, 책, 포스터, 달력, 사진)을 그린다
// 그리는 법은 GBA판(gba/src/scene.c)과 같고, 방의 위치만 다르다 (ROOM_X, ROOM_Y)
#include "common.h"

static void R(int x, int y, int w, int h, u16 c) { fill(TOP.bb, x + ROOM_X, y + ROOM_Y, w, h, c); }
static u16 hex(u32 rgb) { return RGB((rgb >> 16) & 255, (rgb >> 8) & 255, rgb & 255); }

static const u32 CD_COLORS[9] = { 0xe8e8f0, 0xc9d6e8, 0xf2d48a, 0x9fc7e8, 0xe8a0a0, 0xb0e0b8, 0xd0b8e8, 0xf0f0f0, 0xa8b0c0 };
static u32 lcg;
static u32 lcg_next9(void) { lcg = lcg * 1664525u + 1013904223u; return (u32)(((unsigned long long)lcg * 9) >> 32); }

static void draw_rack(void) {
  int discs = S.st_discs;
  const int rows[3] = { 62, 74, 86 };
  lcg = 42;
  int idx = 0;
  for (int ri = 0; ri < 3 && discs > 0; ri++) {
    int start = ri == 2 ? 26 : 7;
    for (int cx = start; cx < 56 && discs > 0; cx++, discs--, idx++) {
      R(cx, rows[ri] + 1, 1, 9, hex(CD_COLORS[lcg_next9()]));
      R(cx, rows[ri] + 1, 1, 1, idx % 2 ? hex(0xffffff) : hex(0xd8dce8));
      if (idx % 3 == 0) R(cx, rows[ri] + 5, 1, 2, hex(0x5a5a6a));
    }
  }
  int sx = 62;
  while (discs > 0 && sx < 92) {
    int n = MIN(discs, 26);
    for (int i = 0; i < n; i++) R(sx, 127 - i, 8, 1, i % 2 ? hex(0xd8dce8) : hex(0xb8c0d0));
    R(sx, 127 - n, 8, 1, hex(0xffffff)); R(sx + 3, 127 - n, 2, 1, hex(0x8a8a9a));
    discs -= n; sx += 10;
  }
}

static void draw_books(void) {
  int nb = 0;
  for (int w = 0; w < NWORKS; w++) if (WORKS[w].type == T_MANGA || WORKS[w].type == T_NOVEL) nb += S.col[w].phys;
  static const u32 BOOK[6][2] = { { 0xc8503a, 0x9a3a28 }, { 0x3a6ab0, 0x2a4a80 }, { 0xe0b040, 0xb08820 }, { 0x4a9a5a, 0x2e7040 }, { 0x8a5aa0, 0x6a3a80 }, { 0xe8e0d0, 0xc0b8a8 } };
  int lim = MIN(nb, 40), i = 0;
  for (int bx = 1; i < lim && bx < 16; bx += 8)
    for (int k = 0; k < 20 && i < lim; k++, i++) {
      const u32 *c = BOOK[(i * 7) % 6];
      int sh = (k * 3 + bx) % 3 - 1;
      R(bx + sh, 132 - k * 2, 7, 2, hex(c[0])); R(bx + sh, 133 - k * 2, 7, 1, hex(c[1])); R(bx + sh + 6, 132 - k * 2, 1, 1, hex(0xf4f0e0));
    }
}

static void draw_poster(const Traits *t) {
  if (t->top < 0) return;
  u16 c = GENRES[t->top].color;
  R(66, 12, 22, 32, hex(0xfbf6ea));
  for (int y = 0; y < 24; y++) {
    int k = y < 12 ? 0 : (y - 12) * 2;
    int r = (c & 31) * (24 - k) / 24, g = ((c >> 5) & 31) * (24 - k) / 24, b = ((c >> 10) & 31) * (24 - k) / 24 + k / 6;
    R(67, 13 + y, 20, 1, (u16)(0x8000 | r | g << 5 | (b > 31 ? 31 : b) << 10));
  }
  R(75, 18, 5, 6, hex(0xf0f0f0)); R(73, 25, 9, 8, hex(0xf0f0f0)); R(71, 27, 13, 6, hex(0xd8d8e0));
  R(68, 38, 18, 2, hex(0x3a3a4a)); R(70, 41, 14, 1, hex(0x9a9aa8));
  R(65, 11, 4, 3, hex(0xf0e8c8)); R(85, 11, 4, 3, hex(0xf0e8c8));
}

static void draw_photos(void) {
  int photos = 1 + MIN(3, S.friend / 30 + (S.love >= 50 ? 1 : 0));
  static const int SP[4][2] = { { 153, 15 }, { 164, 16 }, { 154, 26 }, { 165, 27 } };
  static const u32 PC[4] = { 0x7ab0e0, 0x90c890, 0xf0b0c8, 0xe8c870 };
  for (int i = 1; i < photos; i++) {
    int px = SP[i][0], py = SP[i][1];
    R(px, py, 9, 8, hex(0xfbfbf4)); R(px + 1, py + 1, 7, 5, hex(PC[i]));
    R(px + 2, py + 3, 2, 3, hex(0x6a5a5a)); R(px + 5, py + 3, 2, 3, hex(0x6a5a5a));
    R(px + 4, py - 1, 1, 1, hex(0xd83030));
  }
}

// 상태 막대용 짧은 용량 (10GB 넘으면 소수점 없이)
static const char *short_mb(s32 mb) { return mb >= 10000 ? fmt("%dGB", mb / 1000) : mbstr(mb); }

// 위쪽 상태 막대: 날짜, 돈, 하드, 공CD
void scene_status(void) {
  int y, m; date_of(MIN(S.turn, 35), &y, &m);
  for (int i = 0; i < SCREEN_W * 14; i++) TOP.bb[i] = (u16)(0x8000 | ((TOP.bb[i] >> 2) & 0x1CE7));
  int x = draw_text(TOP.bb, 3, 0, fmt("%d.%s%d", y, m < 10 ? "0" : "", m), C_WHITE);
  x = draw_text(TOP.bb, x + 7, 0, wonstr(S.money), C_GREEN);
  s32 used = hdd_used();
  x = draw_text(TOP.bb, x + 7, 0, fmt("%s/%s", short_mb(used), short_mb(S.hdd_cap)), used * 10 > S.hdd_cap * 9 ? C_RED : C_WHITE);
  const char *cd = fmt("공CD %d", S.blank);
  draw_text(TOP.bb, MAX(x + 7, SCREEN_W - 3 - text_w(cd)), 0, cd, C_WHITE);
}

int top_scene;            // 1 = 방, 2 = 2026년 방, 3 = 타이틀
u32 scene_key;            // 방을 그릴 때의 상태 (돈·공CD·하드)
u32 status_key(void) { return (u32)S.money * 31 + S.blank * 7 + (u32)hdd_used() + S.turn * 1000003u; }

void scene_room(void) {
  top_scene = 1; scene_key = status_key();
  copy_image(TOP.bb, ART_ROOMS[season_of(MIN(S.turn, 35))]);
  int y, m; date_of(MIN(S.turn, 35), &y, &m);
  digits3x5(TOP.bb, ROOM_X + 102 - (m >= 10 ? 4 : 2), ROOM_Y + 20, m, hex(0x333333));
  Traits t; compute_traits(&t);
  draw_poster(&t);
  draw_photos();
  draw_rack();
  draw_books();
  if (S.screen == SCR_OFF) { R(112, 49, 28, 21, hex(0x1a2222)); R(114, 51, 8, 1, hex(0x3a4848)); }
  scene_status();
  present(&TOP);
}

void scene_future(void) {
  top_scene = 2;
  copy_image(TOP.bb, ART_ROOMS[4]);
  draw_rack();
  draw_books();
  present(&TOP);
}

void scene_title(void) {
  top_scene = 3;
  copy_image(TOP.bb, ART_TITLE);
  present(&TOP);
}
