// 수집왕 NDS — 화면 구성 요소
//  위 화면: 대사창, 작품 자세히 보기
//  아래 화면: 윈도우 98 바탕화면 위의 선택지, 알림창, 진행 창, 조각 모음, 작품 목록, 한글 키보드
// 모든 것은 터치와 버튼 둘 다로 쓸 수 있다.
#include "common.h"

#define WIN_X 2
#define WIN_Y 2
#define WIN_W (SCREEN_W - 4)
#define WIN_H (TASKBAR_Y - 4)
#define ADVANCE (KEY_A | KEY_B | KEY_TOUCH)

int bottom_mode = BOT_NONE;
// 작업 표시줄 시계 (fmt 버퍼를 쓰지 않는다: 선택지 글자를 만든 뒤에 불리기 때문)
const char *clock_str(void) {
  static char b[8] = "2000.03";
  if (!S.magic) return "2000.03";
  int y, m; date_of(MIN(S.turn, 35), &y, &m);
  b[0] = '0' + y / 1000; b[1] = '0' + y / 100 % 10; b[2] = '0' + y / 10 % 10; b[3] = '0' + y % 10;
  b[4] = '.'; b[5] = '0' + m / 10; b[6] = '0' + m % 10; b[7] = 0;
  return b;
}
void bottom_desktop(void) { desktop(clock_str()); bottom_mode = BOT_OTHER; }

static void copy_s(char *dst, int size, const char *src) { int i = 0; if (src) for (; src[i] && i < size - 1; i++) dst[i] = src[i]; dst[i] = 0; }

static const char *last_line;
static int last_who = W_NARR, last_expr = X_NORMAL;
void set_last_line(const char *s) { last_line = s; last_who = W_NARR; }

static const char *who_name(int who) {
  switch (who) {
    case W_HER: return S.her;
    case W_MOM: return "엄마";
    case W_DAD: return "아빠";
    case W_FRIEND: return "태식";
    case W_HYUNG: return "동호회 형 (밤의제왕)";
    case W_OWNER: return "용산 사장님";
  }
  return "";
}

// ---------- 아래 화면: 대사 중 ----------
// 지금 상태를 작은 '내 컴퓨터' 창으로 보여 주고, 아무 데나 누르면 넘어간다
static u32 say_bottom_key;
static void say_bottom(void) {
  u32 key = S.magic ? status_key() : 1;
  if (bottom_mode == BOT_SAY && key == say_bottom_key) return;
  bottom_desktop();
  u16 *d = BOT.vram;
  if (S.magic) {
    win_window(d, 24, 20, 208, 98, "내 컴퓨터");
    s32 used = hdd_used();
    draw_text_flat(d, 32, 40, fmt("가진 돈  %w", S.money), W_TEXT);
    draw_text_flat(d, 32, 56, fmt("공CD  %d장", S.blank), W_TEXT);
    draw_text_flat(d, 32, 72, fmt("C:  %m / %m", used, S.hdd_cap), used * 10 > S.hdd_cap * 9 ? W_RED : W_TEXT);
    win_bar(d, 32, 90, 192, 12, used / 10, S.hdd_cap / 10, used * 10 > S.hdd_cap * 9 ? W_RED : W_NAVY);
  }
  win_button(d, 78, 128, 100, 26, "다음 ▶", 0);
  draw_text_flat(d, 8, 158, "A · 터치", RGB(180, 230, 230));
  bottom_mode = BOT_SAY;
  say_bottom_key = key;
}

// ---------- 위 화면: 대화 ----------
typedef struct { s16 x, y; u16 cp; } Gpos;
static Gpos gl[256];
static const char *layout(const char *s, int x0, int y0, int w, int max_lines, int *count) {
  int x = x0, y = y0, line = 0, n = 0;
  const char *p = s;
  while (*p && n < 255) {
    const char *q = p;
    u32 cp = utf8_next(&q);
    int gw = glyph_width(cp);
    if (cp == '\n') { x = x0; y += LINE_H; line++; p = q; if (line >= max_lines) break; continue; }
    if (cp != ' ' && (p == s || p[-1] == ' ')) {
      int ww = 0;
      for (const char *r = p; *r && *r != ' ' && *r != '\n';) ww += glyph_width(utf8_next(&r));
      if (x + ww > x0 + w && x > x0 && ww <= w) { x = x0; y += LINE_H; line++; if (line >= max_lines) break; }
    }
    if (x + gw > x0 + w) { x = x0; y += LINE_H; line++; if (line >= max_lines) break; }
    if (cp == ' ' && x == x0) { p = q; continue; }
    gl[n].x = (s16)x; gl[n].y = (s16)y; gl[n].cp = (u16)cp; n++;
    x += gw; p = q;
  }
  *count = n;
  while (*p == ' ') p++;
  return *p ? p : 0;
}
static void glyph_at(int i, u16 color) {
  char tmp[4]; int a = 0; put_utf8(tmp, &a, gl[i].cp);
  draw_text(TOP.vram, gl[i].x, gl[i].y, tmp, color);
}

#define DLG_Y 122
void say(int who, int expr, const char *text) {
  static char last_buf[512];
  copy_s(last_buf, sizeof last_buf, text);
  text = last_buf;
  last_line = last_buf; last_who = who; last_expr = expr;
  if (who == W_HER) bgm(BGM_HER);
  // 대화 중에 돈·공CD가 바뀌었으면 위 화면 상태 막대도 새로
  if (S.magic && top_scene == 1 && status_key() != scene_key) scene_room();
  if (top_overlay) present(&TOP);
  say_bottom();
  const char *page = text;
  while (page) {
    restore(&TOP, 0, DLG_Y - 2, SCREEN_W, SCREEN_H - DLG_Y + 2);
    dark_box(&TOP, 2, DLG_Y, SCREEN_W - 2, SCREEN_H - 2);
    int tx = 9, ty = DLG_Y + 5, tw = SCREEN_W - 28, lines = 4;
    if (who != W_NARR) {
      draw_face(TOP.vram, 9, DLG_Y + 14, who, expr);
      draw_text(TOP.vram, 57, DLG_Y + 3, who_name(who), C_NAME);
      tx = 57; ty = DLG_Y + 19; tw = SCREEN_W - 57 - 18; lines = 3;
    }
    int n;
    const char *next = layout(page, tx, ty, tw, lines, &n);
    int shown = 0;
    while (shown < n) {
      frame();
      if (pressed(ADVANCE)) { while (shown < n) glyph_at(shown++, C_WHITE); break; }
      glyph_at(shown++, C_WHITE);
      if (shown < n) glyph_at(shown++, C_WHITE);
      if (shown % 4 < 2 && who != W_NARR) sfx(SFX_BLIP);
    }
    for (int blink = 0;; blink++) {
      frame();
      draw_text(TOP.vram, SCREEN_W - 17, SCREEN_H - 18, next ? "▶" : "▼", (blink & 32) ? C_WHITE : C_DIM);
      if (pressed(ADVANCE)) { sfx(SFX_CLICK); break; }
    }
    page = next;
  }
}
void narr(const char *text) { say(W_NARR, X_NORMAL, text); }

// ---------- 아래 화면: 위아래 넘김 단추 ----------
static void scroll_buttons(int x, int y0, int y1, int can_up, int can_down) {
  win_button(BOT.vram, x, y0, 18, 22, "▲", can_up ? 0 : 3);
  win_button(BOT.vram, x, y1 - 22, 18, 22, "▼", can_down ? 0 : 3);
  fill(BOT.vram, x, y0 + 22, 18, y1 - y0 - 44, RGB(224, 224, 224));
}

// ---------- 선택지 (아래 화면) ----------
int choose(int who, int expr, const char *prompt, const Opt *opts, int n) {
  static char prompt_c[400];
  if (!prompt || !*prompt) { prompt = last_line; who = last_who; expr = last_expr; }
  (void)expr;
  copy_s(prompt_c, sizeof prompt_c, prompt);
  prompt = prompt_c[0] ? prompt_c : 0;
  if (top_overlay) present(&TOP);
  int has_note = 0;
  for (int i = 0; i < n; i++) if (opts[i].note) has_note = 1;
  const int tw = WIN_W - 16;
  int plines = prompt ? MIN(3, count_lines(tw, prompt)) : 0;
  int y_list = WIN_Y + 20 + plines * LINE_H + (plines ? 3 : 0);
  int row = has_note ? 31 : 24;
  int area = WIN_Y + WIN_H - 5 - y_list;
  int vis = MIN(n, area / row);
  int scrolls = n > vis;
  int bw = WIN_W - 12 - (scrolls ? 22 : 0);
  int cur = 0, scroll = 0;
  while (cur < n && opts[cur].disabled) cur++;
  if (cur >= n) cur = 0;
  bottom_desktop();
  win_window(BOT.vram, WIN_X, WIN_Y, WIN_W, WIN_H, who != W_NARR && who >= 0 ? who_name(who) : "선택");
  if (plines) draw_wrapped(BOT.vram, WIN_X + 8, WIN_Y + 19, tw, plines, prompt, W_TEXT, 0);
  int dirty = 1;
  for (;;) {
    if (dirty) {
      if (cur < scroll) scroll = cur;
      if (cur >= scroll + vis) scroll = cur - vis + 1;
      fill(BOT.vram, WIN_X + 3, y_list, WIN_W - 6, vis * row, W_FACE);
      for (int k = 0; k < vis; k++) {
        int i = scroll + k, y = y_list + k * row;
        int st = opts[i].disabled ? 3 : i == cur ? 1 : 0;
        win_button(BOT.vram, WIN_X + 6, y, bw, row - 2, "", st);
        u16 c = st == 1 ? C_WHITE : st == 3 ? W_GRAYTX : W_TEXT;
        draw_wrapped(BOT.vram, WIN_X + 12, y + (has_note ? 1 : 4), bw - 12, 1, opts[i].label, c, 0);
        if (opts[i].note) draw_wrapped(BOT.vram, WIN_X + 12, y + 15, bw - 12, 1, opts[i].note, st == 1 ? RGB(190, 210, 255) : W_GRAYTX, 0);
      }
      if (scrolls) scroll_buttons(WIN_X + WIN_W - 26, y_list, y_list + vis * row - 2, scroll > 0, scroll + vis < n);
      dirty = 0;
    }
    frame();
    if (pressed(KEY_UP)) { cur = (cur + n - 1) % n; sfx(SFX_MOVE); dirty = 1; }
    if (pressed(KEY_DOWN)) { cur = (cur + 1) % n; sfx(SFX_MOVE); dirty = 1; }
    int pick = -1;
    if (pressed(KEY_A)) pick = cur;
    if (pressed(KEY_TOUCH)) {
      for (int k = 0; k < vis; k++)
        if (tapped_in(WIN_X + 6, y_list + k * row, bw, row - 2)) pick = scroll + k;
      if (scrolls && tapped_in(WIN_X + WIN_W - 26, y_list, 18, 22) && scroll > 0) { scroll--; cur = MIN(cur, scroll + vis - 1); sfx(SFX_MOVE); dirty = 1; }
      if (scrolls && tapped_in(WIN_X + WIN_W - 26, y_list + vis * row - 24, 18, 22) && scroll + vis < n) { scroll++; cur = MAX(cur, scroll); sfx(SFX_MOVE); dirty = 1; }
    }
    if (pick >= 0) {
      if (opts[pick].disabled) { cur = pick; dirty = 1; sfx(SFX_ERROR); continue; }
      sfx(SFX_CLICK);
      int k = pick - scroll, y = y_list + k * row;
      win_button(BOT.vram, WIN_X + 6, y, bw, row - 2, "", 2);
      draw_wrapped(BOT.vram, WIN_X + 13, y + (has_note ? 2 : 5), bw - 12, 1, opts[pick].label, W_TEXT, 0);
      wait_frames(5);
      return pick;
    }
  }
}

// ---------- 알림창 (아래 화면) ----------
void notice(const char *title, const char *const *lines, int n) {
  sfx(SFX_NOTICE);
  static char title_c[96];
  copy_s(title_c, sizeof title_c, title);
  if (top_overlay) present(&TOP);
  const int tw = WIN_W - 18, PAGE = 8;
  int start = 0;
  while (start < n) {
    int used = 0, end = start;
    while (end < n) { int l = count_lines(tw, lines[end]); if (used + l > PAGE && end > start) break; used += l; end++; }
    bottom_desktop();
    win_window(BOT.vram, WIN_X, WIN_Y, WIN_W, WIN_H, title_c);
    int y = WIN_Y + 20;
    for (int i = start; i < end; i++) {
      int l = MIN(count_lines(tw, lines[i]), PAGE);
      draw_wrapped(BOT.vram, WIN_X + 9, y, tw, l, lines[i], W_TEXT, 0);
      y += l * LINE_H;
    }
    const char *btn = end < n ? "다음 ▶" : "확인";
    win_button(BOT.vram, WIN_X + WIN_W - 84, WIN_Y + WIN_H - 28, 76, 22, btn, 0);
    if (n > PAGE || end < n || start > 0) draw_text_flat(BOT.vram, WIN_X + 9, WIN_Y + WIN_H - 25, end < n ? "더 있음" : "", W_GRAYTX);
    for (;;) {
      frame();
      if (pressed(ADVANCE)) {
        sfx(SFX_CLICK);
        win_button(BOT.vram, WIN_X + WIN_W - 84, WIN_Y + WIN_H - 28, 76, 22, btn, 2);
        wait_frames(4);
        break;
      }
    }
    start = end;
  }
}

// ---------- 진행 창 (아래 화면) ----------
void progress(const char *title, const char *label, int frames) {
  static char t[96], l[200];
  copy_s(t, sizeof t, title); copy_s(l, sizeof l, label);
  if (top_overlay) present(&TOP);
  bottom_desktop();
  u16 *d = BOT.vram;
  const int x = 12, y = 28, w = 232, h = 116;
  win_window(d, x, y, w, h, t);
  draw_wrapped(d, x + 10, y + 21, w - 20, 2, l, W_TEXT, 0);
  // 폴더에서 폴더로 날아가는 종이 (윈도우 98 복사 창 흉내)
  const int ay = y + 72, fx0 = x + 14, fx1 = x + w - 32, span = fx1 - fx0 - 18;
  const int bx = x + 10, by = y + 86, bw = w - 20;
  fill(d, bx, by, bw, 16, C_WHITE); win_frame(d, bx, by, bw, 16, 1);
  for (int f = 0; f <= frames; f++) {
    fill(d, x + 4, ay - 20, w - 8, 34, W_FACE);
    fill(d, fx0, ay, 18, 13, RGB(240, 200, 60)); fill(d, fx0, ay - 3, 8, 3, RGB(220, 180, 40));
    fill(d, fx1, ay, 18, 13, RGB(240, 200, 60)); fill(d, fx1, ay - 3, 8, 3, RGB(220, 180, 40));
    int p = (f * 4) % span, px = fx0 + 18 + p;
    int lift = p * (span - p) * 4 / (span * span / 4 + 1);    // 포물선 (최대 약 16)
    fill(d, px, ay - lift, 9, 11, C_WHITE); win_frame(d, px, ay - lift, 9, 11, 0);
    int blocks = (bw - 6) / 9, k = blocks * f / MAX(1, frames);
    for (int i = 0; i < k; i++) fill(d, bx + 3 + i * 9, by + 3, 7, 10, W_NAVY);
    frame();
  }
  wait_frames(15);
}

// ---------- 조각 모음 (아래 화면) ----------
void defrag_anim(int frag) {
  sfx(SFX_HDD);
  enum { COLS = 28, ROWS = 12, NCELL = COLS * ROWS };
  static u8 cells[NCELL];
  int used = 0;
  for (int i = 0; i < NCELL; i++) {
    int r = rnd(1000);
    cells[i] = r < 500 ? 1 : r < 500 + frag * 4 ? 2 : 0;
    if (cells[i]) used++;
  }
  if (top_overlay) present(&TOP);
  bottom_desktop();
  u16 *d = BOT.vram;
  win_window(d, WIN_X, WIN_Y, WIN_W, WIN_H, "디스크 조각 모음 - C:");
  const int x0 = 16, y0 = 24;
  fill(d, x0 - 3, y0 - 3, COLS * 8 + 5, ROWS * 8 + 5, C_WHITE);
  win_frame(d, x0 - 3, y0 - 3, COLS * 8 + 5, ROWS * 8 + 5, 1);
  const int frames = 150;
  for (int f = 0; f <= frames; f++) {
    int k = NCELL * f / frames;
    for (int i = 0; i < NCELL; i++) {
      int c = i < k ? (i < used ? 1 : 0) : cells[i];
      u16 col = i == k ? RGB(32, 192, 64) : c == 1 ? RGB(32, 64, 192) : c == 2 ? RGB(208, 48, 48) : RGB(232, 232, 232);
      fill(d, x0 + (i % COLS) * 8, y0 + (i / COLS) * 8, 7, 7, col);
    }
    win_bar(d, x0 - 3, y0 + ROWS * 8 + 8, COLS * 8 + 5, 14, f, frames, W_NAVY);
    if (f % 10 == 0) { fill(d, x0, y0 + ROWS * 8 + 26, 120, 14, W_FACE); draw_text_flat(d, x0, y0 + ROWS * 8 + 26, fmt("완료: %d%%", f * 100 / frames), W_TEXT); }
    if (f % 40 == 0) sfx(SFX_HDD);
    frame();
  }
  wait_frames(30);
}

// ---------- 작품 목록 ----------
// 아래 화면: 종류 탭 + 목록 (터치·버튼) / 위 화면: 커서가 있는 작품 자세히
static void work_detail(int w, InfoFn info, const int *out, int nsel, int max) {
  restore(&TOP, 0, 14, SCREEN_W, SCREEN_H - 14);
  dark_title_box(&TOP, 6, 20, SCREEN_W - 6, SCREEN_H - 6, "작품 정보");
  u16 *d = TOP.vram;
  draw_icon(d, 12, 40, WORKS[w].type, complete(w));
  int y = 40;
  y += LINE_H * MIN(2, count_lines(SCREEN_W - 46, WORKS[w].title));
  draw_wrapped(d, 34, 40, SCREEN_W - 46, 2, WORKS[w].title, C_NAME, 1);
  if (y < 58) y = 58;
  const char *g2 = WORKS[w].g1 != 255 ? fmt(", %s", GENRES[WORKS[w].g1].name) : "";
  draw_text(d, 14, y, fmt("%s · %s%s%s", TYPES[WORKS[w].type].name, GENRES[WORKS[w].g0].name, g2, WORKS[w].appear == S.turn && S.turn > 0 ? " · NEW" : ""), C_DIM);
  y += LINE_H + 2;
  if (info) { const char *t = info(w); int l = MIN(3, count_lines(SCREEN_W - 28, t)); draw_wrapped(d, 14, y, SCREEN_W - 28, l, t, C_INFO, 1); y += l * LINE_H; }
  y += 2;
  if (max > 1) {
    draw_text(d, 14, y, fmt("고른 작품 %d/%d", nsel, max), C_WHITE); y += LINE_H;
    for (int k = 0; k < nsel && y < SCREEN_H - 20; k++, y += LINE_H) draw_wrapped(d, 22, y, SCREEN_W - 40, 1, fmt("%d. %s", k + 1, WORKS[out[k]].title), C_NAME, 1);
  }
  top_overlay = 1;
}

int pick_works(const char *title, const u8 *list, int n, int max, InfoFn info, const char *hint, int can_cancel, const char *confirm, int *out) {
  if (n <= 0) return 0;
  static char title_c[96], hint_c[160], confirm_c[64];
  copy_s(title_c, sizeof title_c, title); copy_s(hint_c, sizeof hint_c, hint); copy_s(confirm_c, sizeof confirm_c, confirm);
  title = title_c; confirm = confirm_c; hint = hint ? hint_c : 0;
  static u8 order[NWORKS];
  for (int i = 0; i < n; i++) order[i] = list[i];
  for (int i = 1; i < n; i++) {
    u8 v = order[i]; int j = i - 1;
    #define KEY(w) ((WORKS[w].appear == S.turn && S.turn > 0 ? 100 : 0) + WORKS[w].appear)
    while (j >= 0 && KEY(order[j]) < KEY(v)) { order[j + 1] = order[j]; j--; }
    order[j + 1] = v;
  }
  int tabs[NTYPES + 1], nt = 0;
  tabs[nt++] = -1;
  for (int t = 0; t < NTYPES; t++) for (int i = 0; i < n; i++) if (WORKS[order[i]].type == t) { tabs[nt++] = t; break; }
  int tab = 0, cur = 0, scroll = 0, nsel = 0, dirty = 1, rows_dirty = 0, detail_dirty = 1;
  static u8 shown[NWORKS];
  int ns = 0;
  const int TAB_Y = WIN_Y + 19, LIST_Y = WIN_Y + 38, ROW = 17, VIS = 6;
  const int LIST_W = WIN_W - 34, FOOT_Y = LIST_Y + VIS * ROW + 4;
  int tab_x[NTYPES + 2];
  u16 *d = BOT.vram;
  for (;;) {
    if (dirty) {
      ns = 0;
      for (int i = 0; i < n; i++) if (tabs[tab] < 0 || WORKS[order[i]].type == tabs[tab]) shown[ns++] = order[i];
      if (cur >= ns) cur = ns - 1;
      if (cur < 0) cur = 0;
      bottom_desktop();
      win_window(d, WIN_X, WIN_Y, WIN_W, WIN_H, title);
      int tx = WIN_X + 5;
      for (int i = 0; i < nt; i++) {
        const char *name = tabs[i] < 0 ? "전체" : TYPES[tabs[i]].folder;
        int w = text_w(name) + 8;
        tab_x[i] = tx;
        win_button(d, tx, TAB_Y, w, 17, name, i == tab ? 1 : 0);
        tx += w + 1;
      }
      tab_x[nt] = tx;
      // 아래쪽 단추
      int bx = WIN_X + WIN_W - 6;
      const char *ok = max > 1 ? "받기 (START)" : "고르기";
      if (max > 1) ok = fmt("%s (START)", confirm);
      int okw = MIN(text_w(ok) + 14, 150);
      bx -= okw; win_button(d, bx, FOOT_Y + 1, okw, 22, ok, 0);
      if (can_cancel) { bx -= 52; win_button(d, bx, FOOT_Y + 1, 50, 22, "취소", 0); }
      rows_dirty = 1; detail_dirty = 1;
      dirty = 0;
    }
    if (rows_dirty) {
      if (cur < scroll) scroll = cur;
      if (cur >= scroll + VIS) scroll = cur - VIS + 1;
      fill(d, WIN_X + 5, LIST_Y, LIST_W, VIS * ROW, C_WHITE);
      win_frame(d, WIN_X + 4, LIST_Y - 1, LIST_W + 2, VIS * ROW + 2, 1);
      for (int k = 0; k < VIS && scroll + k < ns; k++) {
        int i = scroll + k, w = shown[i], y = LIST_Y + k * ROW;
        int sel = -1;
        for (int q = 0; q < nsel; q++) if (out[q] == w) sel = q;
        int on = i == cur;
        if (on) fill(d, WIN_X + 6, y, LIST_W - 2, ROW, W_NAVY);
        draw_icon(d, WIN_X + 7, y, WORKS[w].type, complete(w));
        if (max > 1) {   // 체크 칸
          fill(d, WIN_X + 25, y + 3, 11, 11, C_WHITE); win_frame(d, WIN_X + 25, y + 3, 11, 11, 1);
          if (sel >= 0) draw_text_flat(d, WIN_X + 26, y + 1, "✓", W_TEXT);
        }
        int tx0 = WIN_X + (max > 1 ? 39 : 26);
        const char *cnt = fmt("%d/%d", owned(w), WORKS[w].units);
        int cw = text_w(cnt);
        int new_w = WORKS[w].appear == S.turn && S.turn > 0 ? 24 : 0;
        draw_wrapped(d, tx0, y + 1, WIN_X + 5 + LIST_W - tx0 - cw - new_w - 8, 1, WORKS[w].title, on ? C_WHITE : sel >= 0 ? W_BLUE : W_TEXT, 0);
        if (new_w) draw_text_flat(d, WIN_X + 5 + LIST_W - cw - new_w - 4, y + 1, "NEW", on ? C_NAME : W_RED);
        draw_text_flat(d, WIN_X + 5 + LIST_W - cw - 3, y + 1, cnt, on ? C_WHITE : W_GRAYTX);
      }
      if (!ns) draw_text_flat(d, WIN_X + 12, LIST_Y + 4, "고를 수 있는 작품이 없다.", W_GRAYTX);
      scroll_buttons(WIN_X + WIN_W - 26, LIST_Y - 1, LIST_Y + VIS * ROW + 1, scroll > 0, scroll + VIS < ns);
      // 아래 안내
      fill(d, WIN_X + 4, FOOT_Y, 96, 24, W_FACE);
      const char *h2 = max > 1 ? fmt("%d/%d개 고름", nsel, max) : "두 번 누르면 결정";
      draw_text_flat(d, WIN_X + 6, FOOT_Y + 5, h2, W_GRAYTX);
      rows_dirty = 0;
      detail_dirty = 1;
    }
    if (detail_dirty) { if (ns) work_detail(shown[cur], info, out, nsel, max); detail_dirty = 0; if (hint && nsel == 0 && max > 1) draw_wrapped(TOP.vram, 14, SCREEN_H - 24, SCREEN_W - 28, 1, hint, C_GRAY, 1); }
    frame();
    int toggle = -1;
    if (pressed(KEY_UP) && ns) { cur = (cur + ns - 1) % ns; sfx(SFX_MOVE); rows_dirty = 1; }
    if (pressed(KEY_DOWN) && ns) { cur = (cur + 1) % ns; sfx(SFX_MOVE); rows_dirty = 1; }
    if (pressed(KEY_LEFT) && ns) { cur = MAX(0, cur - VIS); rows_dirty = 1; }
    if (pressed(KEY_RIGHT) && ns) { cur = MIN(ns - 1, cur + VIS); rows_dirty = 1; }
    if (pressed(KEY_R)) { tab = (tab + 1) % nt; cur = scroll = 0; sfx(SFX_MOVE); dirty = 1; }
    if (pressed(KEY_L)) { tab = (tab + nt - 1) % nt; cur = scroll = 0; sfx(SFX_MOVE); dirty = 1; }
    if (pressed(KEY_B) && can_cancel) { sfx(SFX_CLICK); present(&TOP); return -1; }
    if (pressed(KEY_A) && ns) toggle = cur;
    int want_confirm = pressed(KEY_START) != 0;
    if (pressed(KEY_TOUCH)) {
      for (int i = 0; i < nt; i++) if (tapped_in(tab_x[i], TAB_Y, tab_x[i + 1] - tab_x[i], 17) && i != tab) { tab = i; cur = scroll = 0; sfx(SFX_MOVE); dirty = 1; }
      for (int k = 0; k < VIS && scroll + k < ns; k++)
        if (tapped_in(WIN_X + 5, LIST_Y + k * ROW, LIST_W, ROW)) {
          int i = scroll + k;
          if (max > 1 || i == cur) toggle = i;
          cur = i; rows_dirty = 1; sfx(SFX_MOVE);
        }
      if (tapped_in(WIN_X + WIN_W - 26, LIST_Y - 1, 18, 22) && scroll > 0) { scroll--; cur = MIN(cur, scroll + VIS - 1); rows_dirty = 1; }
      if (tapped_in(WIN_X + WIN_W - 26, LIST_Y + VIS * ROW - 21, 18, 22) && scroll + VIS < ns) { scroll++; cur = MAX(cur, scroll); rows_dirty = 1; }
      // 아래 단추: 오른쪽 끝이 결정, 그 왼쪽이 취소
      if (tap_y >= FOOT_Y + 1 && tap_y < FOOT_Y + 23 && tap_x >= WIN_X + 104) {
        int okx = WIN_X + WIN_W - 6 - MIN(text_w(max > 1 ? fmt("%s (START)", confirm) : "고르기") + 14, 150);
        if (tap_x >= okx) { if (max > 1) want_confirm = 1; else if (ns) toggle = cur; }
        else if (can_cancel && tap_x >= okx - 52) { sfx(SFX_CLICK); present(&TOP); return -1; }
      }
    }
    if (toggle >= 0 && ns) {
      int w = shown[toggle];
      if (max == 1) { out[0] = w; sfx(SFX_CLICK); present(&TOP); return 1; }
      int found = -1;
      for (int k = 0; k < nsel; k++) if (out[k] == w) found = k;
      if (found >= 0) { for (int k = found; k < nsel - 1; k++) out[k] = out[k + 1]; nsel--; }
      else if (nsel < max) out[nsel++] = w;
      else { for (int k = 0; k < nsel - 1; k++) out[k] = out[k + 1]; out[nsel - 1] = w; }
      sfx(SFX_CLICK); rows_dirty = 1;
    }
    if (want_confirm) {
      if (nsel > 0) { sfx(SFX_CLICK); present(&TOP); return nsel; }
      sfx(SFX_ERROR);
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
static void card(const char *title, const char *buf, int cursor_on) {
  restore(&TOP, 24, 56, 208, 76);
  dark_title_box(&TOP, 24, 56, 232, 132, title);
  int ex = draw_text(TOP.vram, 34, 80, buf, C_WHITE);
  if (cursor_on) fill(TOP.vram, ex + 1, 81, 2, 12, C_NAME);
  draw_text(TOP.vram, 34, 106, "아래 화면의 키를 눌러 입력", C_GRAY);
  top_overlay = 1;
}
int keyboard(const char *title, char *out, int max_syl, const char *initial) {
  static char title_c[96];
  copy_s(title_c, sizeof title_c, title);
  u16 done[12]; int nd = 0;
  const char *p = initial ? initial : "";
  while (*p && nd < max_syl) done[nd++] = (u16)utf8_next(&p);
  int cho = -1, jung = -1, jong = 0, cx = 0, cy = 0;
  bottom_desktop();
  win_window(BOT.vram, WIN_X, WIN_Y, WIN_W, WIN_H - 2, title_c);
  for (int r = 0; r < 5; r++) for (int c = 0; c < 10; c++) if (!(KEYS[r][c] == 2 && c > 1)) kb_key(r, c, r == cy && c == cx);
  int field_dirty = 1, blink = -1;
  char buf[64]; buf[0] = 0;
  // 위 화면 카드 제목: '·' 앞부분
  char ctitle[48]; int ci = 0;
  for (const char *q = title_c; *q && ci < 46; q++) { if ((u8)q[0] == 0xC2 && (u8)q[1] == 0xB7) break; ctitle[ci++] = *q; }
  while (ci > 0 && ctitle[ci - 1] == ' ') ci--;
  ctitle[ci] = 0;
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
    if (bl != blink) { card(ctitle, buf, bl); blink = bl; }
    frame();
    int ox = cx, oy = cy, key = 0;
    if (pressed(KEY_LEFT)) { cx = (cx + 9) % 10; if (KEYS[cy][cx] == 2) cx = cx >= 1 ? 0 : cx; }
    if (pressed(KEY_RIGHT)) { cx = (cx + 1) % 10; if (KEYS[cy][cx] == 2 && cx > 1) cx = 0; }
    if (pressed(KEY_UP)) { cy = (cy + 4) % 5; if (KEYS[cy][cx] == 2) cx = 1; }
    if (pressed(KEY_DOWN)) { cy = (cy + 1) % 5; if (KEYS[cy][cx] == 2) cx = 1; }
    if (ox != cx || oy != cy) sfx(SFX_MOVE);
    if (pressed(KEY_A)) key = KEYS[cy][cx];
    if (pressed(KEY_B)) key = 1;
    if (pressed(KEY_START)) key = 2;
    int r, c;
    if (pressed(KEY_TOUCH) && kb_hit(tap_x, tap_y, &r, &c)) {
      key = KEYS[r][c]; cx = c; cy = r;
      kb_key(oy, ox, 0); kb_key(r, c, 2); wait_frames(4); ox = -1;
    }
    if (ox != cx || oy != cy) { if (ox >= 0) kb_key(oy, ox, 0); kb_key(cy, cx, 1); }
    if (!key) continue;
    sfx(SFX_CLICK); field_dirty = 1;
    if (key == 2) {
      if (cho >= 0 && jung >= 0 && nd < 12) done[nd++] = (u16)(0xAC00 + (cho * 21 + jung) * 28 + jong);
      cho = jung = -1; jong = 0;
      if (nd == 0) { sfx(SFX_ERROR); continue; }
      int at = 0; out[0] = 0;
      for (int i = 0; i < nd; i++) put_utf8(out, &at, done[i]);
      present(&TOP);
      return nd;
    }
    if (key == 1) {
      if (jong) { int dj = -1; for (int i = 0; i < 11; i++) if (DOUBLE_JONG[i][2] == jong) dj = DOUBLE_JONG[i][0]; jong = dj > 0 ? dj : 0; }
      else if (jung >= 0) jung = -1;
      else if (cho >= 0) cho = -1;
      else if (nd > 0) nd--;
      continue;
    }
    if (key >= 0x3131 && key <= 0x314E) {
      int k2 = -1;
      for (int i = 0; i < 19; i++) if (CHO_CP[i] == key) k2 = i;
      if (k2 < 0) continue;
      if (cho < 0 && jung < 0) { if (nd + 1 <= max_syl) cho = k2; else sfx(SFX_ERROR); }
      else if (jung < 0) { if (nd + 1 < max_syl) { done[nd++] = CHO_CP[cho]; cho = k2; } }
      else if (jong == 0) {
        int j = jong_of_cho(k2);
        if (j && cho >= 0) jong = j;
        else if (nd + 1 < max_syl) { done[nd++] = (u16)(cho >= 0 ? 0xAC00 + (cho * 21 + jung) * 28 : 0x314F + jung); cho = k2; jung = -1; }
      } else {
        int comb = 0;
        for (int i = 0; i < 11; i++) if (DOUBLE_JONG[i][0] == jong && DOUBLE_JONG[i][1] == k2) comb = DOUBLE_JONG[i][2];
        if (comb) jong = comb;
        else if (nd + 1 < max_syl) { done[nd++] = (u16)(0xAC00 + (cho * 21 + jung) * 28 + jong); cho = k2; jung = -1; jong = 0; }
      }
    } else {
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
