// 수집왕 NDS — 두 화면 출력, 창, 글자, 입력
#include <stdarg.h>
#include "common.h"

static u16 bb_top[SCREEN_W * SCREEN_H] __attribute__((aligned(32)));
static u16 bb_bot[SCREEN_W * SCREEN_H] __attribute__((aligned(32)));
Screen TOP = { VRAM_TOP, bb_top }, BOT = { VRAM_BOT, bb_bot };
u32 frame_count;

// 두 엔진 모두 모드 5, BG3을 256x256 16비트 비트맵으로
static void setup_engine(volatile u32 *dispcnt, volatile u16 *bg3cnt, volatile u16 *pa) {
  *dispcnt = 0x00010805;          // 모드 5, BG3 켜기, 보통 표시
  *bg3cnt = (1 << 14) | (1 << 7) | (1 << 2);   // 256x256, 직접 색 비트맵
  pa[0] = 256; pa[1] = 0; pa[2] = 0; pa[3] = 256;   // 확대·회전 없음
  *(volatile u32 *)(pa + 4) = 0; *(volatile u32 *)(pa + 6) = 0;
}
void gfx_init(void) {
  REG_POWCNT1 = 0x8203;           // 화면, 2D 엔진 A·B 켜기. 엔진 A가 위 화면
  REG_EXMEMCNT &= ~0x8800;        // 카드·GBA 슬롯 접근은 ARM9 쪽 (메인 RAM 우선권은 그대로)
  VRAMCNT_A = 0x81;               // VRAM A → 엔진 A 배경 (0x06000000)
  VRAMCNT_C = 0x84;               // VRAM C → 엔진 B 배경 (0x06200000)
  setup_engine((volatile u32 *)0x04000000, (volatile u16 *)0x0400000E, (volatile u16 *)0x04000030);
  setup_engine((volatile u32 *)0x04001000, (volatile u16 *)0x0400100E, (volatile u16 *)0x04001030);
  for (int i = 0; i < SCREEN_W * SCREEN_H; i++) { VRAM_TOP[i] = 0x8000; VRAM_BOT[i] = 0x8000; }
}

// ---------- 입력 ----------
static u16 keys_now, keys_prev, keys_hit;
int touch_x, touch_y, touch_down, tap_x = -1, tap_y = -1;
u16 raw_tx, raw_ty;
void poll_input(void) {
  Shared *sh = SHARED;
  u16 k = (u16)(~REG_KEYINPUT & 0x03FF);
  k |= (sh->xy & 3) << 10;
  int down = sh->pen;
  if (down) { touch_x = sh->tx; touch_y = sh->ty; raw_tx = sh->rawx; raw_ty = sh->rawy; k |= KEY_TOUCH; }
  touch_down = down;
  keys_prev = keys_now;
  keys_now = k;
  keys_hit = keys_now & ~keys_prev;
  if (keys_hit & KEY_TOUCH) { tap_x = touch_x; tap_y = touch_y; }
}
int pressed(u16 k) { return keys_hit & k; }
int held(u16 k) { return keys_now & k; }
int tapped_in(int x, int y, int w, int h) {
  return (keys_hit & KEY_TOUCH) && tap_x >= x && tap_x < x + w && tap_y >= y && tap_y < y + h;
}

void vsync(void) {
  while (REG_VCOUNT >= 192) {}
  while (REG_VCOUNT < 192) {}
  frame_count++;
}
void frame(void) {
  vsync(); poll_input();
#ifdef AUTOTEST
  // 시험용: 지금 몇 번째 달인지 아래 화면 오른쪽 위 8픽셀에 (흰색 = 1)
  { extern State S; int v = S.magic ? S.turn : 255; for (int i = 0; i < 8; i++) VRAM_BOT[248 + i] = (v >> i) & 1 ? 0xFFFF : 0x8000; }
#endif
  if (pressed(KEY_SELECT)) sound_toggle();
}
void wait_frames(int n) { while (n--) frame(); }

// 두 화면 모두 천천히 어둡게/밝게 (밝기 조절 레지스터)
#define REG_MASTER_BRIGHT     (*(volatile u16 *)0x0400006C)
#define REG_MASTER_BRIGHT_SUB (*(volatile u16 *)0x0400106C)
void fade(int dir) {
  for (int i = 0; i <= 16; i++) {
    int v = dir > 0 ? i : 16 - i;
    u16 r = (u16)(v ? (2 << 14) | v : 0);
    REG_MASTER_BRIGHT = r; REG_MASTER_BRIGHT_SUB = r;
    vsync();
  }
}

// ---------- 그리기 ----------
static void copy32(void *dst, const void *src, int bytes) {
  u32 *d = dst; const u32 *s = src;
  for (int i = 0; i < bytes / 4; i++) d[i] = s[i];
}
int top_overlay;
void present(Screen *s) { copy32(s->vram, s->bb, SCREEN_W * SCREEN_H * 2); if (s == &TOP) top_overlay = 0; }
void copy_image(u16 *dst, const u16 *src) { copy32(dst, src, SCREEN_W * SCREEN_H * 2); }
void restore(Screen *s, int x, int y, int w, int h) {
  int x0 = MAX(0, x), x1 = MIN(SCREEN_W, x + w), y1 = MIN(SCREEN_H, y + h);
  for (int yy = MAX(0, y); yy < y1; yy++)
    for (int xx = x0; xx < x1; xx++) s->vram[yy * SCREEN_W + xx] = s->bb[yy * SCREEN_W + xx];
}
ITCM void fill(u16 *dst, int x, int y, int w, int h, u16 c) {
  int x0 = MAX(0, x), x1 = MIN(SCREEN_W, x + w), y1 = MIN(SCREEN_H, y + h);
  for (int yy = MAX(0, y); yy < y1; yy++) {
    u16 *p = dst + yy * SCREEN_W;
    for (int xx = x0; xx < x1; xx++) p[xx] = c;
  }
}
// 뒤판을 1/4 밝기로 줄이고 남색을 더해 반투명 창처럼
ITCM void blend_dark(Screen *s, int x, int y, int w, int h) {
  int x0 = MAX(0, x), x1 = MIN(SCREEN_W, x + w), y1 = MIN(SCREEN_H, y + h);
  for (int yy = MAX(0, y); yy < y1; yy++) {
    const u16 *src = s->bb + yy * SCREEN_W;
    u16 *d = s->vram + yy * SCREEN_W;
    for (int xx = x0; xx < x1; xx++) d[xx] = (u16)(0x8000 | (((src[xx] >> 2) & 0x1CE7) + ((6 << 10) | (1 << 5) | 1)));
  }
}
static void pset(u16 *dst, int x, int y, u16 c) { if ((unsigned)x < SCREEN_W && (unsigned)y < SCREEN_H) dst[y * SCREEN_W + x] = c; }
void draw_img(u16 *dst, int x, int y, int w, int h, const u16 *img) {
  for (int yy = 0; yy < h; yy++)
    for (int xx = 0; xx < w; xx++) { u16 c = img[yy * w + xx]; if (c & 0x8000) pset(dst, x + xx, y + yy, c); }
}
void draw_icon(u16 *dst, int x, int y, int type, int done) { draw_img(dst, x, y, 16, 16, ART_ICONS + (type * 2 + (done ? 1 : 0)) * 256); }
void win_bar(u16 *dst, int x, int y, int w, int h, int val, int max, u16 color) {
  fill(dst, x, y, w, h, C_WHITE);
  win_frame(dst, x, y, w, h, 1);
  int f = max > 0 ? (int)((long long)MIN(val, max) * (w - 4) / max) : 0;
  if (f > 0) fill(dst, x + 2, y + 2, f, h - 4, color);
}
static const char *const DIG[10] = {
  "111101101101111", "010110010010111", "111001111100111", "111001111001111", "101101111001001",
  "111100111001111", "111100111101111", "111001010010010", "111101111101111", "111101111001111",
};
void digits3x5(u16 *dst, int x, int y, int n, u16 c) {
  char buf[8]; int len = 0;
  do { buf[len++] = '0' + n % 10; n /= 10; } while (n && len < 7);
  for (int i = len - 1; i >= 0; i--, x += 4)
    for (int k = 0; k < 15; k++) if (DIG[buf[i] - '0'][k] == '1') pset(dst, x + k % 3, y + k / 3, c);
}
void draw_face(u16 *dst, int x, int y, int who, int expr) {
  draw_img(dst, x, y, 40, 40, ART_FACES + (who * 6 + expr) * 1600);
  for (int i = -1; i <= 40; i++) { pset(dst, x + i, y - 1, C_BORDER); pset(dst, x + i, y + 40, C_BORDER); pset(dst, x - 1, y + i, C_BORDER); pset(dst, x + 40, y + i, C_BORDER); }
}
void dark_box(Screen *s, int x0, int y0, int x1, int y1) {
  blend_dark(s, x0, y0, x1 - x0, y1 - y0);
  fill(s->vram, x0, y0, x1 - x0, 1, C_BORDER); fill(s->vram, x0, y1 - 1, x1 - x0, 1, C_BORDER);
  fill(s->vram, x0, y0, 1, y1 - y0, C_BORDER); fill(s->vram, x1 - 1, y0, 1, y1 - y0, C_BORDER);
}
// 왼쪽 남색 → 오른쪽 파랑 제목 막대
static void title_bar(u16 *dst, int x0, int y0, int x1, const char *title) {
  for (int x = x0; x < x1; x++) {
    int t = (x - x0) * 16 / MAX(1, x1 - x0);
    u16 c = (u16)(0x8000 | (t + 4) << 10 | (t / 2) << 5);
    for (int y = y0; y < y0 + 14; y++) dst[y * SCREEN_W + x] = c;
  }
  draw_text_flat(dst, x0 + 4, y0, title, C_WHITE);
}
void dark_title_box(Screen *s, int x0, int y0, int x1, int y1, const char *title) {
  dark_box(s, x0, y0, x1, y1);
  title_bar(s->vram, x0 + 1, y0 + 1, x1 - 1, title);
}
// 윈도우 98 입체 테두리
void win_frame(u16 *dst, int x, int y, int w, int h, int sunken) {
  u16 hi = sunken ? W_SHADE : W_LIGHT, lo = sunken ? W_LIGHT : W_DARK, lo2 = sunken ? W_FACE : W_SHADE;
  fill(dst, x, y, w, 1, hi); fill(dst, x, y, 1, h, hi);
  fill(dst, x, y + h - 1, w, 1, lo); fill(dst, x + w - 1, y, 1, h, lo);
  fill(dst, x + 1, y + h - 2, w - 2, 1, lo2); fill(dst, x + w - 2, y + 1, 1, h - 2, lo2);
}
void win_window(u16 *dst, int x, int y, int w, int h, const char *title) {
  fill(dst, x, y, w, h, W_FACE);
  win_frame(dst, x, y, w, h, 0);
  title_bar(dst, x + 3, y + 3, x + w - 3, title);
}
void win_button(u16 *dst, int x, int y, int w, int h, const char *label, int state) {
  fill(dst, x, y, w, h, state == 1 ? W_NAVY : W_FACE);
  win_frame(dst, x, y, w, h, state == 2);
  int tw = text_w(label), o = state == 2 ? 1 : 0;
  u16 c = state == 1 ? C_WHITE : state == 3 ? W_GRAYTX : W_TEXT;
  draw_text_flat(dst, x + (w - tw) / 2 + o, y + (h - 13) / 2 + o, label, c);
}
// 아래 화면: 청록색 바탕화면과 작업 표시줄
void desktop(const char *clock) {
  Screen *s = &BOT;
  u16 *d = s->bb;
  fill(d, 0, 0, SCREEN_W, SCREEN_H - 20, W_DESK);
  fill(d, 0, SCREEN_H - 20, SCREEN_W, 20, W_FACE);
  fill(d, 0, SCREEN_H - 20, SCREEN_W, 1, W_LIGHT);
  // 시작 단추
  fill(d, 2, SCREEN_H - 17, 46, 15, W_FACE); win_frame(d, 2, SCREEN_H - 17, 46, 15, 0);
  for (int i = 0; i < 4; i++) fill(d, 6 + (i % 2) * 4, SCREEN_H - 14 + (i / 2) * 4, 3, 3, i == 0 ? RGB(230, 60, 40) : i == 1 ? RGB(60, 180, 60) : i == 2 ? RGB(40, 90, 220) : RGB(240, 200, 40));
  draw_text_flat(d, 16, SCREEN_H - 17, "시작", W_TEXT);
  // 시계 칸
  int cw = text_w(clock) + 10;
  win_frame(d, SCREEN_W - cw - 2, SCREEN_H - 17, cw, 15, 1);
  draw_text_flat(d, SCREEN_W - cw + 3, SCREEN_H - 17, clock, W_TEXT);
  present(s);
}

// ---------- 글자 ----------
static int glyph_index(u32 cp) {
  int lo = 0, hi = FONT_COUNT - 1;
  while (lo <= hi) {
    int mid = (lo + hi) >> 1;
    if (FONT_CODES[mid] == cp) return mid;
    if (FONT_CODES[mid] < cp) lo = mid + 1; else hi = mid - 1;
  }
  return -1;
}
int glyph_width(u32 cp) { int i = glyph_index(cp); return i < 0 ? 6 : FONT_WIDTHS[i]; }
u32 utf8_next(const char **p) {
  const unsigned char *s = (const unsigned char *)*p;
  u32 cp;
  if (s[0] < 0x80) { cp = s[0]; *p += 1; }
  else if ((s[0] & 0xE0) == 0xC0) { cp = ((s[0] & 0x1F) << 6) | (s[1] & 0x3F); *p += 2; }
  else if ((s[0] & 0xF0) == 0xE0) { cp = ((s[0] & 0x0F) << 12) | ((s[1] & 0x3F) << 6) | (s[2] & 0x3F); *p += 3; }
  else { cp = '?'; *p += 4; }
  return cp;
}
int text_w(const char *s) { int w = 0; while (*s) w += glyph_width(utf8_next(&s)); return w; }
void put_utf8(char *o, int *at, u32 cp) {
  if (cp < 0x80) o[(*at)++] = (char)cp;
  else if (cp < 0x800) { o[(*at)++] = (char)(0xC0 | (cp >> 6)); o[(*at)++] = (char)(0x80 | (cp & 63)); }
  else { o[(*at)++] = (char)(0xE0 | (cp >> 12)); o[(*at)++] = (char)(0x80 | ((cp >> 6) & 63)); o[(*at)++] = (char)(0x80 | (cp & 63)); }
  o[*at] = 0;
}

ITCM static void glyph_bits(u16 *dst, int x, int y, const u16 *rows, int h, u16 color, int shadow) {
  int inside = x >= 0 && y >= 0 && x + 17 < SCREEN_W && y + h + 1 < SCREEN_H;
  for (int pass = shadow ? 0 : 1; pass < 2; pass++) {
    int o = pass ? 0 : 1;
    u16 c = pass ? color : C_SHADOW;
    for (int r = 0; r < h; r++) {
      u32 bits = rows[r];
      if (!bits) continue;
      for (int b = 0; bits; b++, bits = (bits << 1) & 0xFFFF)
        if (bits & 0x8000) {
          int px = x + b + o, py = y + r + o;
          if (inside || ((unsigned)px < SCREEN_W && (unsigned)py < SCREEN_H)) dst[py * SCREEN_W + px] = c;
        }
    }
  }
}
static int draw_glyph(u16 *dst, int x, int y, u32 cp, u16 color, int shadow) {
  int i = glyph_index(cp);
  if (i < 0) return 6;
  glyph_bits(dst, x, y, FONT_BITS + i * FONT_CELL_H, FONT_CELL_H, color, shadow);
  return FONT_WIDTHS[i];
}
int draw_text(u16 *dst, int x, int y, const char *s, u16 color) { while (*s) x += draw_glyph(dst, x, y, utf8_next(&s), color, 1); return x; }
int draw_text_flat(u16 *dst, int x, int y, const char *s, u16 color) { while (*s) x += draw_glyph(dst, x, y, utf8_next(&s), color, 0); return x; }

// 띄어쓰기 단위로 줄을 바꾼다. 줄 수를 넘으면 그 자리를 돌려준다
static const char *wrap_core(u16 *dst, int x0, int y, int w, int max_lines, const char *s, u16 color, int shadow, int *lines_out) {
  int x = x0, line = 0;
  const char *p = s;
  while (*p) {
    const char *q = p;
    u32 cp = utf8_next(&q);
    if (cp == '\n') { x = x0; line++; y += LINE_H; p = q; if (line >= max_lines) break; continue; }
    int gw = glyph_width(cp);
    if (cp != ' ' && (p == s || p[-1] == ' ')) {
      int ww = 0;
      for (const char *r = p; *r && *r != ' ' && *r != '\n';) ww += glyph_width(utf8_next(&r));
      if (x + ww > x0 + w && x > x0 && ww <= w) { x = x0; line++; y += LINE_H; if (line >= max_lines) break; }
    }
    if (x + gw > x0 + w) { x = x0; line++; y += LINE_H; if (line >= max_lines) break; }
    if (cp == ' ' && x == x0) { p = q; continue; }
    if (dst) draw_glyph(dst, x, y, cp, color, shadow);
    x += gw;
    p = q;
  }
  if (lines_out) *lines_out = line + (x > x0 ? 1 : 0);
  return *p ? p : 0;
}
const char *draw_wrapped(u16 *dst, int x, int y, int w, int max_lines, const char *s, u16 color, int shadow) {
  return wrap_core(dst, x, y, w, max_lines, s, color, shadow, 0);
}
int count_lines(int w, const char *s) { int n; wrap_core(0, 0, 0, w, 99, s, 0, 0, &n); return n ? n : 1; }

