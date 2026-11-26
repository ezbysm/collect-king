// 수집왕 NDS — 1단계 시험 롬
// 위 화면: 방 장면과 대사 / 아래 화면: 윈도우 98 바탕화면 위의 메뉴, 키보드, 선택지 (터치·버튼 모두)
#include "nds.h"

enum { W_NARR = -1, W_HER, W_MOM, W_DAD, W_FRIEND, W_HYUNG, W_OWNER };
enum { X_NORMAL, X_SMILE, X_BLUSH, X_SAD, X_SURPRISE, X_ANGRY };

static char player[40] = "", heroine[24] = "한수진";
static const char *CLOCK = "2000.03";

static void wait_frames(int n) { while (n--) { vsync(); poll_input(); } }

// ---------- 위 화면: 방 ----------
static const char *const DIG[10] = {
  "111101101101111", "010110010010111", "111001111100111", "111001111001111", "101101111001001",
  "111100111001111", "111100111101111", "111001010010010", "111101111101111", "111101111001111",
};
static void digits3x5(u16 *dst, int x, int y, int n, u16 c) {
  char buf[8]; int len = 0;
  do { buf[len++] = '0' + n % 10; n /= 10; } while (n && len < 7);
  for (int i = len - 1; i >= 0; i--, x += 4)
    for (int k = 0; k < 15; k++) if (DIG[buf[i] - '0'][k] == '1') dst[(y + k / 3) * SCREEN_W + x + k % 3] = c;
}
static void top_room(int season, int month) {
  copy_image(TOP.bb, ART_ROOMS[season]);
  digits3x5(TOP.bb, ROOM_X + 102 - (month >= 10 ? 4 : 2), ROOM_Y + 20, month, RGB(51, 51, 51));
  // 상태 막대
  for (int i = 0; i < SCREEN_W * 14; i++) TOP.bb[i] = (u16)(0x8000 | ((TOP.bb[i] >> 2) & 0x1CE7));
  int x = draw_text(TOP.bb, 3, 0, CLOCK, C_WHITE);
  x = draw_text(TOP.bb, x + 8, 0, "중학교 1학년", C_DIM);
  if (player[0]) draw_text(TOP.bb, x + 8, 0, player, C_NAME);
  draw_text(TOP.bb, SCREEN_W - 3 - text_w("NDS 시험판"), 0, "NDS 시험판", C_GRAY);
  present(&TOP);
}

// ---------- 아래 화면 ----------
static void bottom_desktop(void) { desktop(&BOT, CLOCK); }
static void bottom_hint(const char *title, const char *text) {
  bottom_desktop();
  win_window(BOT.vram, 20, 40, 216, 90, title);
  draw_wrapped(BOT.vram, 30, 62, 196, 2, text, W_TEXT, 0);
  win_button(BOT.vram, 78, 98, 100, 24, "다음 ▶", 0);
}

// ---------- 대사 ----------
static const char *const WHO_NAME[6] = { 0, "엄마", "아빠", "친구", "형", "사장님" };
// 위 화면 아래쪽 창에 한 글자씩. A·B·터치로 빨리 넘기기 / 다음
static void say(int who, int expr, const char *text) {
  bottom_hint("대화", "A 버튼을 누르거나 아래 화면 아무 곳이나 누르면 다음으로 넘어가요.");
  const char *page = text;
  while (page) {
    int x0 = who >= 0 ? 56 : 10, y0 = 128;
    dark_box(&TOP, 2, y0, SCREEN_W - 2, SCREEN_H - 2);
    if (who >= 0) {
      draw_face(TOP.vram, 9, y0 + 11, who, expr);
      draw_text(TOP.vram, x0, y0 + 2, who == W_HER ? heroine : WHO_NAME[who], C_NAME);
    }
    int ty = who >= 0 ? y0 + 17 : y0 + 7, lines = who >= 0 ? 3 : 4;
    if (who < 0) lines = 4;
    int w = SCREEN_W - x0 - 10;
    // 이 쪽에 들어갈 만큼 글자 위치를 미리 잡고 한 글자씩
    const char *next = draw_wrapped(0, x0, ty, w, lines, page, 0, 1);
    const char *end = next ? next : page + 4096;
    char buf[512]; int n = 0;
    for (const char *p = page; *p && p < end && n < 500; p++) buf[n++] = *p;
    buf[n] = 0;
    int shown = 0, total = 0;
    for (const char *p = buf; *p; utf8_next(&p)) total++;
    int skip = 0;
    while (shown < total) {
      shown = skip ? total : MIN(total, shown + 2);
      // 앞쪽 shown 글자만 그린다
      char part[512]; int at = 0, k = 0;
      for (const char *p = buf; *p && k < shown; k++) { const char *q = p; utf8_next(&q); while (p < q) part[at++] = *p++; }
      part[at] = 0;
      blend_dark(&TOP, x0, ty, w + 4, lines * LINE_H);
      draw_wrapped(TOP.vram, x0, ty, w, lines, part, C_WHITE, 1);
      vsync(); poll_input();
      if (pressed(KEY_A | KEY_B | KEY_TOUCH)) skip = 1;
    }
    // ▼ 깜빡이며 기다리기
    for (;;) {
      u16 c = (frame_count >> 4) & 1 ? C_WHITE : RGB(60, 60, 120);
      draw_text(TOP.vram, SCREEN_W - 16, SCREEN_H - 17, "▼", c);
      vsync(); poll_input();
      if (pressed(KEY_A | KEY_B | KEY_TOUCH)) break;
    }
    page = next;
  }
}
static void narr(const char *t) { say(W_NARR, 0, t); }

// ---------- 선택지 (아래 화면) ----------
static int choose(const char *prompt, const char *const *opts, int n) {
  bottom_desktop();
  int h = 40 + n * 30;
  int y = (SCREEN_H - 20 - h) / 2;
  win_window(BOT.vram, 12, y, 232, h, "고르기");
  draw_text_flat(BOT.vram, 20, y + 20, prompt, W_TEXT);
  int cur = 0, dirty = 1;
  for (;;) {
    if (dirty) { for (int i = 0; i < n; i++) win_button(BOT.vram, 22, y + 38 + i * 30, 212, 26, opts[i], i == cur); dirty = 0; }
    vsync(); poll_input();
    if (pressed(KEY_UP)) { cur = (cur + n - 1) % n; dirty = 1; }
    if (pressed(KEY_DOWN)) { cur = (cur + 1) % n; dirty = 1; }
    if (pressed(KEY_A)) return cur;
    for (int i = 0; i < n; i++)
      if (tapped_in(22, y + 38 + i * 30, 212, 26)) {
        // 눌린 모양을 잠깐 보여 준다
        win_button(BOT.vram, 22, y + 38 + i * 30, 212, 26, opts[i], 2);
        wait_frames(8);
        return i;
      }
  }
}

// ---------- 한글 키보드 (아래 화면) ----------
static const u16 CHO_CP[19] = { 0x3131, 0x3132, 0x3134, 0x3137, 0x3138, 0x3139, 0x3141, 0x3142, 0x3143, 0x3145, 0x3146, 0x3147, 0x3148, 0x3149, 0x314A, 0x314B, 0x314C, 0x314D, 0x314E };
static const u16 JONG_CP[28] = { 0, 0x3131, 0x3132, 0x3133, 0x3134, 0x3135, 0x3136, 0x3137, 0x3139, 0x313A, 0x313B, 0x313C, 0x313D, 0x313E, 0x313F, 0x3140, 0x3141, 0x3142, 0x3144, 0x3145, 0x3146, 0x3147, 0x3148, 0x314A, 0x314B, 0x314C, 0x314D, 0x314E };
static const u8 DOUBLE_JONG[11][3] = { { 1, 9, 3 }, { 4, 12, 5 }, { 4, 18, 6 }, { 8, 0, 9 }, { 8, 6, 10 }, { 8, 7, 11 }, { 8, 9, 12 }, { 8, 16, 13 }, { 8, 17, 14 }, { 8, 18, 15 }, { 17, 9, 18 } };
static int jong_of_cho(int c) { for (int j = 1; j < 28; j++) if (JONG_CP[j] == CHO_CP[c]) return j; return 0; }
static int cho_of_jong(int j) { for (int c = 0; c < 19; c++) if (CHO_CP[c] == JONG_CP[j]) return c; return -1; }
// 1 = 지우기, 2 = 확인
static const u16 KEYS[5][10] = {
  { 0x3131, 0x3132, 0x3134, 0x3137, 0x3138, 0x3139, 0x3141, 0x3142, 0x3143, 0x3145 },
  { 0x3146, 0x3147, 0x3148, 0x3149, 0x314A, 0x314B, 0x314C, 0x314D, 0x314E, 1 },
  { 0x314F, 0x3150, 0x3151, 0x3152, 0x3153, 0x3154, 0x3155, 0x3156, 0x3157, 0x315B },
  { 0x315C, 0x3160, 0x3161, 0x3163, 0x3158, 0x3159, 0x315A, 0x315D, 0x315E, 0x315F },
  { 0x3162, 2, 2, 2, 2, 2, 2, 2, 2, 2 },
};
#define KB_X 8
#define KB_Y 46
#define KB_W 24
#define KB_H 24
static void kb_key(int r, int c, int state) {
  u16 k = KEYS[r][c];
  if (k == 2) c = 1;
  int x = KB_X + c * KB_W, y = KB_Y + r * KB_H, w = (k == 2 ? KB_W * 9 : KB_W) - 2;
  char t[8]; int a = 0; t[0] = 0;
  if (k > 2) put_utf8(t, &a, k);
  win_button(BOT.vram, x, y, w, KB_H - 2, k == 1 ? "←" : k == 2 ? "확인 (START)" : t, state);
}
static int kb_hit(int tx, int ty, int *r, int *c) {
  if (tx < KB_X || ty < KB_Y || tx >= KB_X + 10 * KB_W || ty >= KB_Y + 5 * KB_H) return 0;
  *c = (tx - KB_X) / KB_W; *r = (ty - KB_Y) / KB_H;
  if (KEYS[*r][*c] == 2) *c = 1;
  return 1;
}
// 위 화면: 학생증
static void student_card(const char *buf, int cursor_on) {
  restore(&TOP, 40, 40, 176, 70);
  dark_title_box(&TOP, 40, 40, 216, 110, "학생증");
  draw_text(TOP.vram, 50, 60, "한빛중학교 1학년", C_DIM);
  int ex = draw_text(TOP.vram, 50, 80, buf[0] ? buf : "", C_WHITE);
  if (cursor_on) fill(TOP.vram, ex + 1, 81, 2, 12, C_NAME);
}
static int keyboard(const char *title, char *out, int max_syl) {
  u16 done[12]; int nd = 0;
  int cho = -1, jung = -1, jong = 0, cx = 0, cy = 0;
  bottom_desktop();
  win_window(BOT.vram, 2, 2, 252, 168, title);
  for (int r = 0; r < 5; r++) for (int c = 0; c < 10; c++) if (!(KEYS[r][c] == 2 && c > 1)) kb_key(r, c, r == cy && c == cx);
  int field_dirty = 1, blink = -1;
  char buf[64];
  for (;;) {
    if (field_dirty) {
      int at = 0; buf[0] = 0;
      for (int i = 0; i < nd; i++) put_utf8(buf, &at, done[i]);
      if (cho >= 0 && jung >= 0) put_utf8(buf, &at, 0xAC00 + (cho * 21 + jung) * 28 + jong);
      else if (cho >= 0) put_utf8(buf, &at, CHO_CP[cho]);
      else if (jung >= 0) put_utf8(buf, &at, 0x314F + jung);
      fill(BOT.vram, 8, 21, 240, 22, C_WHITE); win_frame(BOT.vram, 8, 21, 240, 22, 1);
      draw_text_flat(BOT.vram, 14, 25, buf, W_TEXT);
      field_dirty = 0; blink = -1;
    }
    int bl = (frame_count >> 5) & 1;
    if (bl != blink) { student_card(buf, bl); blink = bl; }
    vsync(); poll_input();
    int ox = cx, oy = cy, key = 0;
    if (pressed(KEY_LEFT)) { cx = (cx + 9) % 10; if (KEYS[cy][cx] == 2) cx = KEYS[cy][0] == 2 ? 1 : (cx >= 1 ? 0 : cx); }
    if (pressed(KEY_RIGHT)) { cx = (cx + 1) % 10; if (KEYS[cy][cx] == 2 && cx > 1) cx = 0; }
    if (pressed(KEY_UP)) { cy = (cy + 4) % 5; if (KEYS[cy][cx] == 2) cx = 1; }
    if (pressed(KEY_DOWN)) { cy = (cy + 1) % 5; if (KEYS[cy][cx] == 2) cx = 1; }
    if (pressed(KEY_A)) key = KEYS[cy][cx];
    if (pressed(KEY_B)) key = 1;
    if (pressed(KEY_START)) key = 2;
    int r, c;
    if ((pressed(KEY_TOUCH)) && kb_hit(tap_x, tap_y, &r, &c)) {
      key = KEYS[r][c]; cx = c; cy = r;
      kb_key(oy, ox, 0); kb_key(r, c, 2); wait_frames(5); ox = -1;
    }
    if (ox != cx || oy != cy) { if (ox >= 0) kb_key(oy, ox, 0); kb_key(cy, cx, 1); }
    if (!key) continue;
    field_dirty = 1;
    if (key == 2) {
      if (cho >= 0 && jung >= 0 && nd < 12) done[nd++] = (u16)(0xAC00 + (cho * 21 + jung) * 28 + jong);
      cho = jung = -1; jong = 0;
      if (nd == 0) continue;
      int at = 0; out[0] = 0;
      for (int i = 0; i < nd; i++) put_utf8(out, &at, done[i]);
      return nd;
    }
    if (key == 1) {
      if (jong) { int dj = -1; for (int i = 0; i < 11; i++) if (DOUBLE_JONG[i][2] == jong) dj = DOUBLE_JONG[i][0]; jong = dj > 0 ? dj : 0; }
      else if (jung >= 0) jung = -1;
      else if (cho >= 0) cho = -1;
      else if (nd > 0) nd--;
      continue;
    }
    if (key >= 0x3131 && key <= 0x314E) {   // 자음
      int ci = -1;
      for (int i = 0; i < 19; i++) if (CHO_CP[i] == key) ci = i;
      if (ci < 0) continue;
      if (cho < 0 && jung < 0) { if (nd + 1 <= max_syl) cho = ci; }
      else if (jung < 0) { if (nd + 1 < max_syl) { done[nd++] = CHO_CP[cho]; cho = ci; } }
      else if (jong == 0) {
        int j = jong_of_cho(ci);
        if (j && cho >= 0) jong = j;
        else if (nd + 1 < max_syl) { done[nd++] = (u16)(cho >= 0 ? 0xAC00 + (cho * 21 + jung) * 28 : 0x314F + jung); cho = ci; jung = -1; }
      } else {
        int comb = 0;
        for (int i = 0; i < 11; i++) if (DOUBLE_JONG[i][0] == jong && DOUBLE_JONG[i][1] == ci) comb = DOUBLE_JONG[i][2];
        if (comb) jong = comb;
        else if (nd + 1 < max_syl) { done[nd++] = (u16)(0xAC00 + (cho * 21 + jung) * 28 + jong); cho = ci; jung = -1; jong = 0; }
      }
    } else {                                 // 모음
      int v = key - 0x314F;
      if (cho < 0 && jung < 0) { if (nd + 1 <= max_syl) jung = v; }
      else if (jung < 0) jung = v;
      else if (jong) {
        int keep = 0, move = jong;
        for (int i = 0; i < 11; i++) if (DOUBLE_JONG[i][2] == jong) { keep = DOUBLE_JONG[i][0]; move = jong_of_cho(DOUBLE_JONG[i][1]); }
        int nc = cho_of_jong(move);
        if (nc >= 0 && nd + 1 < max_syl) { done[nd++] = (u16)(0xAC00 + (cho * 21 + jung) * 28 + keep); cho = nc; jung = v; jong = 0; }
      } else if (nd + 1 < max_syl) {
        done[nd++] = (u16)(cho >= 0 ? 0xAC00 + (cho * 21 + jung) * 28 : 0x314F + jung);
        cho = -1; jung = v;
      }
    }
  }
}

// ---------- 입력 확인 화면 ----------
static void input_test(void) {
  // 위: 버튼 상태
  copy_image(TOP.bb, ART_ROOMS[2]);
  present(&TOP);
  dark_title_box(&TOP, 8, 8, SCREEN_W - 8, SCREEN_H - 8, "입력 확인");
  static const struct { u16 k; const char *name; int x, y; } B[] = {
    { KEY_L, "L", 20, 32 }, { KEY_R, "R", 200, 32 },
    { KEY_UP, "↑", 44, 62 }, { KEY_LEFT, "←", 20, 84 }, { KEY_RIGHT, "→", 68, 84 }, { KEY_DOWN, "↓", 44, 106 },
    { KEY_X, "X", 196, 62 }, { KEY_Y, "Y", 172, 84 }, { KEY_A, "A", 220, 84 }, { KEY_B, "B", 196, 106 },
    { KEY_SELECT, "SELECT", 72, 120 }, { KEY_START, "START", 136, 120 },
  };
  const int NB = sizeof B / sizeof B[0];
  // 아래: 그림판
  bottom_desktop();
  win_window(BOT.vram, 2, 2, 252, 168, "그림판 — 눌러서 그려 보세요");
  fill(BOT.vram, 7, 19, 242, 146, C_WHITE);
  win_frame(BOT.vram, 6, 18, 244, 148, 1);
  win_button(BOT.vram, 196, 146, 50, 18, "끝내기", 0);
  u16 last = 0xFFFF; int px = -1, py = -1, shown_x = -2, shown_y = -2, shown_pen = -1;
  static const u16 INK[6] = { RGB(0, 0, 0), RGB(200, 40, 40), RGB(40, 120, 220), RGB(40, 160, 60), RGB(200, 120, 0), RGB(140, 60, 180) };
  int ink = 0, strokes = 0;
  for (;;) {
    vsync(); poll_input();
    u16 now = 0;
    for (int i = 0; i < NB; i++) if (held(B[i].k)) now |= B[i].k;
    if (now != last) {
      for (int i = 0; i < NB; i++) {
        int on = (now & B[i].k) != 0, w = text_w(B[i].name) + 10;
        fill(TOP.vram, B[i].x, B[i].y, MAX(w, 20), 18, on ? C_NAME : RGB(70, 80, 150));
        draw_text_flat(TOP.vram, B[i].x + (MAX(w, 20) - text_w(B[i].name)) / 2, B[i].y + 2, B[i].name, on ? C_BLACK : C_WHITE);
      }
      last = now;
    }
    if (touch_down != shown_pen || touch_x != shown_x || touch_y != shown_y) {
      blend_dark(&TOP, 16, 146, 224, 30);
      if (touch_down) {
        draw_text(TOP.vram, 20, 146, fmt("터치 x %d, y %d", touch_x, touch_y), C_WHITE);
        draw_text(TOP.vram, 20, 161, fmt("패널 원시값 %d, %d", raw_tx, raw_ty), C_DIM);
      } else draw_text(TOP.vram, 20, 146, fmt("터치 없음 · 그은 선 %d개", strokes), C_DIM);
      shown_pen = touch_down; shown_x = touch_x; shown_y = touch_y;
    }
    if (tapped_in(196, 146, 50, 18) || pressed(KEY_START)) {
      win_button(BOT.vram, 196, 146, 50, 18, "끝내기", 2); wait_frames(8); return;
    }
    if (pressed(KEY_A)) ink = (ink + 1) % 6;
    if (touch_down && touch_x >= 8 && touch_x < 248 && touch_y >= 20 && touch_y < 164 && !(touch_x >= 196 && touch_y >= 146)) {
      if (px < 0) { strokes++; px = touch_x; py = touch_y; }
      // 앞 점과 이어서 굵게 긋기
      int dx = touch_x - px, dy = touch_y - py, steps = MAX(1, MAX(dx < 0 ? -dx : dx, dy < 0 ? -dy : dy));
      for (int s = 0; s <= steps; s++) {
        int x = px + dx * s / steps, y = py + dy * s / steps;
        fill(BOT.vram, MAX(8, x - 1), MAX(20, y - 1), 3, 3, INK[ink]);
      }
      px = touch_x; py = touch_y;
    } else px = -1;
  }
}

// ---------- 타이틀 ----------
static int title_screen(void) {
  copy_image(TOP.bb, ART_TITLE);
  present(&TOP);
  bottom_desktop();
  static const char *const MENU[3] = { "새로 시작", "이어하기", "입력 확인" };
  const int X = 48, Y = 50, BW = 160, BH = 26;
  win_window(BOT.vram, 36, 24, 184, 132, "수집왕 2000-2003");
  draw_text_flat(BOT.vram, 46, 140, "NDS 1단계 시험 롬", W_GRAYTX);
  int cur = 0, dirty = 1;
  u16 cursor_bg = ART_TITLE[(TITLE_CURSOR_Y + 1) * SCREEN_W + TITLE_CURSOR_X + TITLE_CURSOR_W + 2];
  for (;;) {
    if (dirty) { for (int i = 0; i < 3; i++) win_button(BOT.vram, X, Y + i * 30, BW, BH, MENU[i], i == 1 ? 3 : i == cur); dirty = 0; }
    // 모니터 속 커서 깜빡임
    if ((frame_count & 31) == 0) restore(&TOP, TITLE_CURSOR_X, TITLE_CURSOR_Y, TITLE_CURSOR_W, TITLE_CURSOR_H);
    if ((frame_count & 31) == 16) fill(TOP.vram, TITLE_CURSOR_X, TITLE_CURSOR_Y, TITLE_CURSOR_W, TITLE_CURSOR_H, cursor_bg);
    vsync(); poll_input();
    if (pressed(KEY_UP)) { cur = cur == 0 ? 2 : 0; dirty = 1; }
    if (pressed(KEY_DOWN)) { cur = cur == 2 ? 0 : 2; dirty = 1; }
    if (pressed(KEY_A)) return cur;
    for (int i = 0; i < 3; i++)
      if (i != 1 && tapped_in(X, Y + i * 30, BW, BH)) { win_button(BOT.vram, X, Y + i * 30, BW, BH, MENU[i], 2); wait_frames(8); return i; }
  }
}

// ---------- 시험 이야기 ----------
static void test_story(void) {
  player[0] = 0;
  top_room(0, 3);
  keyboard("학생증 · 이름을 지어 주세요 (1~6자)", player, 6);
  static const char *const HER[5] = { "한수진", "김지은", "이서연", "박하늘", "정유나" };
  const char *h = HER[frame_count % 5]; int i = 0;
  while (h[i]) { heroine[i] = h[i]; i++; } heroine[i] = 0;
  top_room(0, 3);
  narr(fmt("%s, 열세 살. 2000년 3월, 중학교 1학년이 되었다.", player));
  narr("이 롬은 닌텐도 DS판 수집왕의 1단계 시험판입니다. 두 화면, 한글 글꼴, 터치, 버튼을 확인합니다.");
  say(W_MOM, X_SMILE, fmt("%s, 학교 잘 다녀와! 컴퓨터는 숙제 다 하고 하는 거다?", josa(player, "아/야")));
  say(W_HER, X_NORMAL, "…안녕. 너도 이 반이야?");
  static const char *const OPTS[3] = { "응, 잘 부탁해", "(고개만 끄덕인다)", "그 CD 뭐야?" };
  int c = choose(fmt("%s에게 뭐라고 할까?", heroine), OPTS, 3);
  if (c == 0) say(W_HER, X_SMILE, fmt("응. 나는 %s. 잘 지내 보자.", heroine));
  else if (c == 1) say(W_HER, X_NORMAL, "…그래.");
  else say(W_HER, X_SURPRISE, "이거? 사촌 오빠가 구워 준 거야. 너도 이런 거 좋아해?");
  narr("1단계 시험은 여기까지입니다. 타이틀의 '입력 확인'에서 터치와 버튼을 더 시험해 볼 수 있어요.");
}

int main(void) {
  gfx_init();
  poll_input();
  for (;;) {
    int m = title_screen();
    if (m == 0) test_story();
    else if (m == 2) input_test();
  }
}
