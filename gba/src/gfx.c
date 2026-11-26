// 수집왕 GBA — 화면 출력, 글자, 입력
#include "common.h"

#define FAST __attribute__((section(".iwram"), target("arm"), noinline))

u16 bb[SCREEN_W * SCREEN_H] EWRAM_BSS __attribute__((aligned(4)));
u32 frame_count;

// ---------- 입력 ----------
u16 keys_now, keys_prev;
static u16 keys_hit, key_latch, key_last;
// 매 프레임 새로 눌린 버튼을 모아 둔다 (인터럽트 없이: 어떤 에뮬레이터에서도 같게 동작하도록)
__attribute__((long_call)) void latch_keys(void);
FAST void latch_keys(void) {
  u16 k = ~REG_KEYINPUT & 0x03FF;
  key_latch |= k & ~key_last;
  key_last = k;
}
void poll_keys(void) {
  latch_keys();
  keys_prev = keys_now;
  keys_now = key_last;
  keys_hit = key_latch;
  key_latch = 0;
}
int pressed(u16 k) { return keys_hit & k; }
int held(u16 k) { return keys_now & k; }

// 화면 갱신을 기다리며 소리를 움직이고, SELECT로 소리를 켜고 끈다
void vsync(void) {
  while (REG_VCOUNT >= 160) {}
  while (REG_VCOUNT < 160) {}
  frame_count++;
  latch_keys();
  sound_update();
}

static void dma_copy32(void *dst, const void *src, u32 words) {
  REG_DMA3CNT = 0;
  REG_DMA3SAD = (u32)src;
  REG_DMA3DAD = (u32)dst;
  REG_DMA3CNT = words | (1u << 26) | (1u << 31);
}

void gfx_present(void) { dma_copy32(VRAM, bb, SCREEN_W * SCREEN_H / 2); }
void gfx_restore(int y0, int y1) {
  if (y0 < 0) y0 = 0;
  if (y1 > SCREEN_H) y1 = SCREEN_H;
  if (y1 > y0) dma_copy32(VRAM + y0 * SCREEN_W, bb + y0 * SCREEN_W, (y1 - y0) * SCREEN_W / 2);
}
void gfx_restore_rect(int x0, int y0, int x1, int y1) {
  for (int y = MAX(0, y0); y < MIN(SCREEN_H, y1); y++)
    for (int x = MAX(0, x0); x < MIN(SCREEN_W, x1); x++) VRAM[y * SCREEN_W + x] = bb[y * SCREEN_W + x];
}

void pset(u16 *dst, int x, int y, u16 c) {
  if ((unsigned)x < SCREEN_W && (unsigned)y < SCREEN_H) dst[y * SCREEN_W + x] = c;
}
FAST void fill(u16 *dst, int x, int y, int w, int h, u16 c) {
  int x0 = MAX(0, x), x1 = MIN(SCREEN_W, x + w), y1 = MIN(SCREEN_H, y + h);
  for (int yy = MAX(0, y); yy < y1; yy++) {
    u16 *p = dst + yy * SCREEN_W;
    for (int xx = x0; xx < x1; xx++) p[xx] = c;
  }
}
// 뒤판을 1/4 밝기로 줄이고 남색을 더해 반투명 창처럼
FAST void blend_dark(u16 *dst, int x, int y, int w, int h) {
  int x0 = MAX(0, x), x1 = MIN(SCREEN_W, x + w), y1 = MIN(SCREEN_H, y + h);
  for (int yy = MAX(0, y); yy < y1; yy++) {
    const u16 *src = bb + yy * SCREEN_W;
    u16 *d = dst + yy * SCREEN_W;
    for (int xx = x0; xx < x1; xx++) d[xx] = (u16)(((src[xx] >> 2) & 0x1CE7) + ((6 << 10) | (1 << 5) | 1));
    latch_keys();
  }
}
void draw_box(int x0, int y0, int x1, int y1) {
  blend_dark(VRAM, x0, y0, x1 - x0, y1 - y0);
  for (int x = x0; x < x1; x++) { pset(VRAM, x, y0, C_BORDER); pset(VRAM, x, y1 - 1, C_BORDER); }
  for (int y = y0; y < y1; y++) { pset(VRAM, x0, y, C_BORDER); pset(VRAM, x1 - 1, y, C_BORDER); }
}
// 윈도우 98 느낌의 남색 제목 막대가 있는 창
void draw_title_box(int x0, int y0, int x1, int y1, const char *title) {
  draw_box(x0, y0, x1, y1);
  // 제목 막대: 왼쪽 남색 → 오른쪽 파랑 (열마다 색을 한 번만 계산)
  u16 col[SCREEN_W];
  for (int x = x0 + 1; x < x1 - 1; x++) { int t = (x - x0) * 16 / (x1 - x0); col[x] = (u16)((t + 4) << 10 | (t / 2) << 5); }
  for (int y = y0 + 1; y < y0 + 15; y++) { u16 *d = VRAM + y * SCREEN_W; for (int x = x0 + 1; x < x1 - 1; x++) d[x] = col[x]; }
  draw_text(VRAM, x0 + 5, y0 + 1, title, C_WHITE);
}

void draw_img(u16 *dst, int x, int y, int w, int h, const u16 *img) {
  for (int yy = 0; yy < h; yy++)
    for (int xx = 0; xx < w; xx++) {
      u16 c = img[yy * w + xx];
      if (!(c & 0x8000)) pset(dst, x + xx, y + yy, c);
    }
}
void draw_face(int x, int y, int who, int expr) {
  draw_img(VRAM, x, y, 40, 40, FACES + (who * 6 + expr) * 1600);
  for (int i = -1; i <= 40; i++) { pset(VRAM, x + i, y - 1, C_BORDER); pset(VRAM, x + i, y + 40, C_BORDER); pset(VRAM, x - 1, y + i, C_BORDER); pset(VRAM, x + 40, y + i, C_BORDER); }
}
void draw_icon(int x, int y, int type, int done) { draw_img(VRAM, x, y, 16, 16, ICONS + (type * 2 + (done ? 1 : 0)) * 256); }

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

__attribute__((long_call)) static void glyph_bits(u16 *dst, int x, int y, const u16 *rows, int h, u16 color);
FAST static void glyph_bits(u16 *dst, int x, int y, const u16 *rows, int h, u16 color) {
  // 그림자(오른쪽 아래 1칸) 다음 글자. 화면 안쪽이면 경계 검사 없이 바로 쓴다
  int inside = x >= 0 && y >= 0 && x + 17 < SCREEN_W && y + h + 1 < SCREEN_H;
  for (int pass = 0; pass < 2; pass++) {
    int o = pass ? 0 : 1;
    u16 c = pass ? color : C_SHADOW;
    for (int r = 0; r < h; r++) {
      u32 bits = rows[r];
      if (!bits) continue;
      u16 *line = dst + (y + r + o) * SCREEN_W + x + o;
      for (int b = 0; bits; b++, bits = (bits << 1) & 0xFFFF)
        if (bits & 0x8000) { if (inside) line[b] = c; else pset(dst, x + b + o, y + r + o, c); }
    }
  }
}
static int draw_glyph(u16 *dst, int x, int y, u32 cp, u16 color) {
  int i = glyph_index(cp);
  if (i < 0) return 6;
  glyph_bits(dst, x, y, FONT_BITS + i * FONT_CELL_H, FONT_CELL_H, color);
  latch_keys();
  return FONT_WIDTHS[i];
}
int draw_text(u16 *dst, int x, int y, const char *s, u16 color) {
  while (*s) x += draw_glyph(dst, x, y, utf8_next(&s), color);
  return x;
}

// 띄어쓰기 단위로 줄을 바꾼다. 줄 수를 넘으면 그 자리를 돌려준다
#define LINE_H 15
static const char *wrap_core(u16 *dst, int x0, int y, int w, int max_lines, const char *s, u16 color, int *lines_out) {
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
    if (dst) draw_glyph(dst, x, y, cp, color);
    x += gw;
    p = q;
  }
  if (lines_out) *lines_out = line + (x > x0 ? 1 : 0);
  return *p ? p : 0;
}
const char *draw_wrapped(u16 *dst, int x, int y, int w, int max_lines, const char *s, u16 color) {
  return wrap_core(dst, x, y, w, max_lines, s, color, 0);
}
int count_lines(int w, const char *s) { int n; wrap_core(0, 0, 0, w, 99, s, 0, &n); return n ? n : 1; }

void draw_bar(int x, int y, int w, int h, int val, int max, u16 color) {
  fill(VRAM, x, y, w, h, RGB(30, 30, 50));
  int f = max > 0 ? (int)((s32)MIN(val, max) * (w - 2) / max) : 0;
  if (f > 0) fill(VRAM, x + 1, y + 1, f, h - 2, color);
}

static const char *const DIG[10] = {
  "111101101101111", "010110010010111", "111001111100111", "111001111001111", "101101111001001",
  "111100111001111", "111100111101111", "111001010010010", "111101111101111", "111101111001111",
};
void digits3x5(u16 *dst, int x, int y, int n, u16 c) {
  char buf[8]; int len = 0;
  do { buf[len++] = '0' + n % 10; n /= 10; } while (n && len < 7);
  for (int i = len - 1; i >= 0; i--, x += 4) {
    const char *p = DIG[buf[i] - '0'];
    for (int k = 0; k < 15; k++) if (p[k] == '1') pset(dst, x + k % 3, y + k / 3, c);
  }
}

// 화면 전체를 천천히 어둡게/밝게 (밝기 조절 레지스터)
#define REG_BLDCNT (*(volatile u16 *)0x04000050)
#define REG_BLDY   (*(volatile u16 *)0x04000054)
void fade(int dir) {
  REG_BLDCNT = 0x00FF | (dir > 0 ? 0xC0 : 0xC0);   // 모든 층을 검게
  for (int i = 0; i <= 16; i++) {
    REG_BLDY = dir > 0 ? i : 16 - i;
    vsync();
  }
  if (dir < 0) REG_BLDCNT = 0;
}
