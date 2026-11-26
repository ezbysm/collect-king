// 수집왕 GBA — 1단계 시험 롬
// 확인할 것: 롬 빌드, 웹판 도트 그래픽을 옮긴 배경, 한글 글꼴 출력, 대화 넘기기, 십자키 선택지
#include "font.h"

typedef unsigned short u16;
typedef unsigned int u32;

#define REG_DISPCNT  (*(volatile u16 *)0x04000000)
#define REG_VCOUNT   (*(volatile u16 *)0x04000006)
#define REG_KEYINPUT (*(volatile u16 *)0x04000130)
#define REG_WAITCNT  (*(volatile u16 *)0x04000204)
#define VRAM         ((volatile u16 *)0x06000000)
#define SCREEN_W 240
#define SCREEN_H 160

#define KEY_A     0x0001
#define KEY_B     0x0002
#define KEY_START 0x0008
#define KEY_UP    0x0040
#define KEY_DOWN  0x0080

#define RGB(r, g, b) ((u16)(((r) >> 3) | (((g) >> 3) << 5) | (((b) >> 3) << 10)))

extern const u16 BG_ROOM[SCREEN_W * SCREEN_H];
extern const u16 FACE_HER[40 * 40];

static const u16 C_WHITE = RGB(255, 255, 255);
static const u16 C_SHADOW = RGB(10, 10, 30);
static const u16 C_NAME = RGB(255, 230, 128);
static const u16 C_BORDER = RGB(232, 232, 240);
static const u16 C_DIM = RGB(160, 200, 220);

// ---------- 화면 ----------
static void vsync(void) {
  while (REG_VCOUNT >= 160) {}
  while (REG_VCOUNT < 160) {}
}

static void restore_bg(int y0, int y1) {
  volatile u32 *dst = (volatile u32 *)(VRAM + y0 * SCREEN_W);
  const u32 *src = (const u32 *)(BG_ROOM + y0 * SCREEN_W);
  for (int i = 0; i < (y1 - y0) * SCREEN_W / 2; i++) dst[i] = src[i];
}

static inline void pset(int x, int y, u16 c) {
  if ((unsigned)x < SCREEN_W && (unsigned)y < SCREEN_H) VRAM[y * SCREEN_W + x] = c;
}

// 대화창: 배경을 어둡게 비치게(반투명 남색) + 테두리
static int last_box_top = SCREEN_H;
static void clear_boxes(int new_top) {
  int top = new_top < last_box_top ? new_top : last_box_top;
  restore_bg(top > 2 ? top - 2 : 0, SCREEN_H);
  last_box_top = new_top;
}

static void draw_box(int x0, int y0, int x1, int y1) {
  for (int y = y0; y < y1; y++)
    for (int x = x0; x < x1; x++) {
      // 각 색을 1/4로 줄이고 남색을 더한다 (비트 연산으로 한 번에)
      u16 c = BG_ROOM[y * SCREEN_W + x];
      VRAM[y * SCREEN_W + x] = (u16)(((c >> 2) & 0x1CE7) + ((6 << 10) | (1 << 5) | 1));
    }
  for (int x = x0; x < x1; x++) { pset(x, y0, C_BORDER); pset(x, y1 - 1, C_BORDER); }
  for (int y = y0; y < y1; y++) { pset(x0, y, C_BORDER); pset(x1 - 1, y, C_BORDER); }
}

static void draw_face(int x0, int y0) {
  for (int y = 0; y < 40; y++)
    for (int x = 0; x < 40; x++) VRAM[(y0 + y) * SCREEN_W + x0 + x] = FACE_HER[y * 40 + x];
  for (int i = -1; i <= 40; i++) { pset(x0 + i, y0 - 1, C_BORDER); pset(x0 + i, y0 + 40, C_BORDER); pset(x0 - 1, y0 + i, C_BORDER); pset(x0 + 40, y0 + i, C_BORDER); }
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

static int glyph_width(u32 cp) {
  int i = glyph_index(cp);
  return i < 0 ? 6 : FONT_WIDTHS[i];
}

// 그림자를 깔고 글자를 찍는다. 반환: 글자 폭
static int draw_glyph(int x, int y, u32 cp, u16 color) {
  int i = glyph_index(cp);
  if (i < 0) return 6;
  const u16 *rows = FONT_BITS + i * FONT_CELL_H;
  for (int pass = 0; pass < 2; pass++) {
    int ox = pass ? 0 : 1, oy = pass ? 0 : 1;
    u16 c = pass ? color : C_SHADOW;
    for (int r = 0; r < FONT_CELL_H; r++) {
      u16 bits = rows[r];
      for (int b = 0; bits; b++, bits <<= 1)
        if (bits & 0x8000) pset(x + b + ox, y + r + oy, c);
    }
  }
  return FONT_WIDTHS[i];
}

// UTF-8 한 글자 읽기
static u32 utf8_next(const char **p) {
  const unsigned char *s = (const unsigned char *)*p;
  u32 cp;
  if (s[0] < 0x80) { cp = s[0]; *p += 1; }
  else if ((s[0] & 0xE0) == 0xC0) { cp = ((s[0] & 0x1F) << 6) | (s[1] & 0x3F); *p += 2; }
  else if ((s[0] & 0xF0) == 0xE0) { cp = ((s[0] & 0x0F) << 12) | ((s[1] & 0x3F) << 6) | (s[2] & 0x3F); *p += 3; }
  else { cp = '?'; *p += 4; }
  return cp;
}

static void draw_text(int x, int y, const char *s, u16 color) {
  while (*s) x += draw_glyph(x, y, utf8_next(&s), color);
}

// ---------- 입력 ----------
static u16 keys_now, keys_prev;
static void poll_keys(void) { keys_prev = keys_now; keys_now = ~REG_KEYINPUT & 0x03FF; }
static int pressed(u16 k) { return (keys_now & k) && !(keys_prev & k); }

// ---------- 대화 ----------
enum { NARR, HER };
typedef struct { int who; const char *text; } Line;

#define BOX_Y0 100
#define LINE_H 15

// 한 줄씩 타자 치듯 출력. A로 빨리 넘기고, 다 나오면 A로 다음.
static void say(int who, const char *text) {
  clear_boxes(BOX_Y0);
  draw_box(3, BOX_Y0, SCREEN_W - 3, SCREEN_H - 3);
  int tx0 = 10, tx1 = SCREEN_W - 10, ty = BOX_Y0 + 6;
  if (who == HER) {
    draw_face(9, BOX_Y0 + 8);
    draw_text(56, BOX_Y0 + 4, "그 아이", C_NAME);
    tx0 = 56; ty = BOX_Y0 + 4 + LINE_H + 2;
  }
  int x = tx0, y = ty, done = 0;
  const char *p = text;
  while (1) {
    vsync();
    poll_keys();
    if (!done) {
      int steps = pressed(KEY_A) ? 999 : 1;   // A를 누르면 남은 글자를 한 번에
      while (steps-- && *p) {
        const char *q = p;
        u32 cp = utf8_next(&q);
        int w = glyph_width(cp);
        // 단어 첫 글자에서: 단어 전체가 이 줄에 안 들어가면 미리 줄을 바꾼다 (한국어는 띄어쓰기 단위)
        if (cp != ' ' && (p == text || p[-1] == ' ')) {
          int ww = 0;
          for (const char *r = p; *r && *r != ' ';) ww += glyph_width(utf8_next(&r));
          if (x + ww > tx1 && x > tx0 && ww <= tx1 - tx0) { x = tx0; y += LINE_H; }
        }
        if (x + w > tx1) { x = tx0; y += LINE_H; }
        if (cp == ' ' && x == tx0) { p = q; continue; }
        x += draw_glyph(x, y, cp, C_WHITE);
        p = q;
      }
      if (!*p) done = 1;
    } else {
      static int blink;
      blink++;
      draw_text(SCREEN_W - 18, SCREEN_H - 18, "▼", (blink & 32) ? C_WHITE : C_DIM);
      if (pressed(KEY_A)) return;
    }
  }
}

// 십자키로 고르고 A로 결정. 반환: 고른 번호
static int choose(const char *prompt, const char *const *opts, int n) {
  int cur = 0, dirty = 1;
  while (1) {
    if (dirty) {
      clear_boxes(BOX_Y0 - n * LINE_H);
      draw_box(3, BOX_Y0 - n * LINE_H, SCREEN_W - 3, SCREEN_H - 3);
      draw_text(10, BOX_Y0 - n * LINE_H + 5, prompt, C_NAME);
      for (int i = 0; i < n; i++) {
        int y = BOX_Y0 - n * LINE_H + 5 + (i + 1) * (LINE_H + 2);
        draw_text(24, y, opts[i], i == cur ? C_WHITE : C_DIM);
        if (i == cur) draw_text(11, y, "▶", C_NAME);
      }
      dirty = 0;
    }
    vsync();
    poll_keys();
    if (pressed(KEY_UP)) { cur = (cur + n - 1) % n; dirty = 1; }
    if (pressed(KEY_DOWN)) { cur = (cur + 1) % n; dirty = 1; }
    if (pressed(KEY_A)) return cur;
  }
}

static const Line PROLOGUE[] = {
  { NARR, "2000년 3월. 새 천년이 밝았고, 세상은 생각보다 멀쩡했다." },
  { NARR, "나는 성민. 오늘부터 한빛중학교 1학년이다." },
  { NARR, "삐- 삐리리리- 치이이익-. 모뎀이 전화선을 타고 세상에 접속하는 소리." },
  { NARR, "저 너머에, 내가 모을 수 있는 모든 것이 있었다." },
  { HER,  "…그거, 필통에 붙인 스티커. 애니 맞지?" },
};

static const char *const ANSWERS[] = {
  "\"응, 나 그런 거 좋아해.\"",
  "\"아, 이거? 동생 거야.\"",
  "(말없이 고개만 끄덕인다)",
};
static const char *const REPLIES[] = {
  "그렇구나. …나도 사실.",
  "아… 그렇구나.",
  "…역시.",
};

int main(void) {
  REG_WAITCNT = 0x4317;   // 카트리지 읽기를 빠르게 (표준 대기 설정 + 미리 읽기)
  REG_DISPCNT = 0x0403;   // 비트맵 모드 3 + BG2
  restore_bg(0, SCREEN_H);
  while (1) {
    for (unsigned i = 0; i < sizeof PROLOGUE / sizeof PROLOGUE[0]; i++) say(PROLOGUE[i].who, PROLOGUE[i].text);
    int c = choose("뭐라고 대답할까?", ANSWERS, 3);
    say(HER, REPLIES[c]);
    say(NARR, "수집왕 GBA 시험 롬 · A: 다음 · 십자키: 고르기. 처음으로 돌아갑니다.");
  }
}
