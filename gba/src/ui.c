// 수집왕 GBA — 화면 구성 요소: 대화, 선택지, 알림창, 진행 막대, 조각 모음, 작품 목록, 한글 키보드
#include "common.h"

#define LINE_H 15
#define BOX_BOTTOM (SCREEN_H - 2)

static int last_top = SCREEN_H;
// 이전 창이 있던 자리부터 지금 그릴 창 자리까지 뒤판으로 되돌린다
static void clear_to(int new_top) {
  int top = MIN(new_top, last_top);
  gfx_restore(top - 1, SCREEN_H);
  last_top = new_top;
}

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

// ---------- 대화 ----------
typedef struct { s16 x, y; u16 cp; } Gpos;
static Gpos gl[256];

// 한 쪽에 들어갈 글자 배치를 계산한다. 반환: 다음 쪽 시작 (없으면 0)
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
  char tmp[4]; u32 cp = gl[i].cp;
  if (cp < 0x80) { tmp[0] = (char)cp; tmp[1] = 0; }
  else if (cp < 0x800) { tmp[0] = (char)(0xC0 | (cp >> 6)); tmp[1] = (char)(0x80 | (cp & 63)); tmp[2] = 0; }
  else { tmp[0] = (char)(0xE0 | (cp >> 12)); tmp[1] = (char)(0x80 | ((cp >> 6) & 63)); tmp[2] = (char)(0x80 | (cp & 63)); tmp[3] = 0; }
  draw_text(VRAM, gl[i].x, gl[i].y, tmp, color);
}

void say(int who, int expr, const char *text) {
  static char last_buf[512];
  { int i = 0; for (; text[i] && i < 511; i++) last_buf[i] = text[i]; last_buf[i] = 0; }
  last_line = last_buf; last_who = who; last_expr = expr;
  if (who == W_HER) bgm(BGM_HER);
  const int y0 = 98;
  const char *page = text;
  while (page) {
    clear_to(y0);
    draw_box(3, y0, SCREEN_W - 3, BOX_BOTTOM);
    int tx = 10, ty = y0 + 5, tw = SCREEN_W - 26, lines = 3;
    if (who != W_NARR) {
      draw_face(9, y0 + 9, who, expr);
      draw_text(VRAM, 56, y0 + 3, who_name(who), C_NAME);
      tx = 56; ty = y0 + 20; tw = SCREEN_W - 56 - 16; lines = 2;
    }
    int n;
    const char *next = layout(page, tx, ty, tw, lines, &n);
    int shown = 0;
    while (shown < n) {
      vsync(); poll_keys();
      if (pressed(KEY_SELECT)) sound_toggle();
      if (pressed(KEY_A) || pressed(KEY_B)) { while (shown < n) glyph_at(shown++, C_WHITE); break; }
      glyph_at(shown++, C_WHITE);
      if (shown % 3 == 0 && who != W_NARR) sfx(SFX_BLIP);
    }
    // ▼ 깜빡이며 기다리기
    for (int blink = 0;; blink++) {
      vsync(); poll_keys();
      if (pressed(KEY_SELECT)) sound_toggle();
      draw_text(VRAM, SCREEN_W - 17, BOX_BOTTOM - 16, next ? "▶" : "▼", (blink & 32) ? C_WHITE : C_DIM);
      if (pressed(KEY_A) || pressed(KEY_B)) { sfx(SFX_CLICK); break; }
    }
    page = next;
  }
}
void narr(const char *text) { say(W_NARR, X_NORMAL, text); }

// ---------- 선택지 ----------
int choose(int who, int expr, const char *prompt, const Opt *opts, int n) {
  if (!prompt || !*prompt) { prompt = last_line; who = last_who; expr = last_expr; }
  (void)expr;
  int tw = SCREEN_W - 24;
  int plines = prompt ? MIN(2, count_lines(tw, prompt)) : 0;
  int vis = MIN(n, 5);
  int h = 6 + plines * LINE_H + vis * LINE_H + LINE_H + 4;
  int y0 = BOX_BOTTOM - h;
  int cur = 0, scroll = 0;
  while (cur < n && opts[cur].disabled) cur++;
  if (cur >= n) cur = 0;
  int dirty = 1;
  for (;;) {
    if (dirty) {
      clear_to(y0);
      draw_box(3, y0, SCREEN_W - 3, BOX_BOTTOM);
      int y = y0 + 4;
      if (plines) { draw_wrapped(VRAM, 10, y, tw, 2, prompt, who == W_NARR ? C_NAME : C_NAME); y += plines * LINE_H; }
      if (cur < scroll) scroll = cur;
      if (cur >= scroll + vis) scroll = cur - vis + 1;
      for (int i = scroll; i < scroll + vis; i++) {
        u16 c = opts[i].disabled ? C_GRAY : i == cur ? C_WHITE : C_DIM;
        if (i == cur) draw_text(VRAM, 10, y, "▶", C_NAME);
        draw_wrapped(VRAM, 24, y, SCREEN_W - 34, 1, opts[i].label, c);
        y += LINE_H;
      }
      if (scroll > 0) draw_text(VRAM, SCREEN_W - 18, y0 + 4 + plines * LINE_H, "↑", C_DIM);
      if (scroll + vis < n) draw_text(VRAM, SCREEN_W - 18, y - LINE_H, "↓", C_DIM);
      if (opts[cur].note) draw_wrapped(VRAM, 24, y, SCREEN_W - 34, 1, opts[cur].note, RGB(150, 200, 220));
      dirty = 0;
    }
    vsync(); poll_keys();
    if (pressed(KEY_SELECT)) sound_toggle();
    if (pressed(KEY_UP)) { cur = (cur + n - 1) % n; sfx(SFX_MOVE); dirty = 1; }
    if (pressed(KEY_DOWN)) { cur = (cur + 1) % n; sfx(SFX_MOVE); dirty = 1; }
    if (pressed(KEY_A)) {
      if (opts[cur].disabled) sfx(SFX_ERROR);
      else { sfx(SFX_CLICK); return cur; }
    }
  }
}

// ---------- 알림창 ----------
void notice(const char *title, const char *const *lines, int n) {
  sfx(SFX_NOTICE);
  const int tw = SCREEN_W - 28;
  int total = 0;
  for (int i = 0; i < n; i++) total += count_lines(tw, lines[i]);
  int start = 0;
  while (start < n) {
    // 이번 쪽에 들어갈 줄들
    int used = 0, end = start;
    while (end < n) { int l = count_lines(tw, lines[end]); if (used + l > 7 && end > start) break; used += l; end++; }
    int h = 20 + MIN(used, 7) * LINE_H + 8;
    int y0 = BOX_BOTTOM - h;
    clear_to(y0);
    draw_title_box(6, y0, SCREEN_W - 6, BOX_BOTTOM, title);
    int y = y0 + 19;
    for (int i = start; i < end; i++) {
      int l = MIN(count_lines(tw, lines[i]), 7);
      draw_wrapped(VRAM, 13, y, tw, l, lines[i], C_WHITE);
      y += l * LINE_H;
    }
    for (int blink = 0;; blink++) {
      vsync(); poll_keys();
      if (pressed(KEY_SELECT)) sound_toggle();
      draw_text(VRAM, SCREEN_W - 20, BOX_BOTTOM - 16, end < n ? "▶" : "▼", (blink & 32) ? C_WHITE : C_DIM);
      if (pressed(KEY_A) || pressed(KEY_B)) { sfx(SFX_CLICK); break; }
    }
    start = end;
  }
  (void)total;
}

// ---------- 진행 막대 ----------
void progress(const char *title, const char *label, int frames) {
  const int y0 = BOX_BOTTOM - 62;
  clear_to(y0);
  draw_title_box(6, y0, SCREEN_W - 6, BOX_BOTTOM, title);
  draw_wrapped(VRAM, 13, y0 + 19, SCREEN_W - 28, 1, label, C_WHITE);
  for (int f = 0; f <= frames; f++) {
    draw_bar(13, y0 + 38, SCREEN_W - 26, 12, f, frames, RGB(40, 80, 220));
    vsync();
  }
  for (int i = 0; i < 15; i++) vsync();
}

// ---------- 조각 모음 ----------
void defrag_anim(int frag) {
  sfx(SFX_HDD);
  enum { COLS = 26, ROWS = 11, NCELL = COLS * ROWS };
  static u8 cells[NCELL];
  int used = 0;
  for (int i = 0; i < NCELL; i++) {
    int r = rnd(1000);
    cells[i] = r < 500 ? 1 : r < 500 + frag * 4 ? 2 : 0;
    if (cells[i]) used++;
  }
  const int x0 = 18, y0 = 22;
  clear_to(0);
  draw_title_box(8, 6, SCREEN_W - 8, BOX_BOTTOM, "디스크 조각 모음 - C:");
  fill(VRAM, x0 - 2, y0 - 2, COLS * 8 + 3, ROWS * 8 + 3, RGB(240, 240, 240));
  const int frames = 150;
  for (int f = 0; f <= frames; f++) {
    int k = NCELL * f / frames;
    for (int i = 0; i < NCELL; i++) {
      int c = i < k ? (i < used ? 1 : 0) : cells[i];
      u16 col = i == k ? RGB(32, 192, 64) : c == 1 ? RGB(32, 64, 192) : c == 2 ? RGB(208, 48, 48) : RGB(232, 232, 232);
      fill(VRAM, x0 + (i % COLS) * 8, y0 + (i / COLS) * 8, 7, 7, col);
    }
    draw_bar(x0, y0 + ROWS * 8 + 6, COLS * 8, 10, f, frames, C_NAVY);
    if (f % 10 == 0) { fill(VRAM, x0, y0 + ROWS * 8 + 18, 90, 14, RGB(20, 20, 60)); draw_text(VRAM, x0, y0 + ROWS * 8 + 18, fmt("완료: %d%%", f * 100 / frames), C_WHITE); }
    if (f % 40 == 0) sfx(SFX_HDD);
    vsync();
  }
  for (int i = 0; i < 30; i++) vsync();
  last_top = 0;
}

// ---------- 작품 목록 ----------
int pick_works(const char *title, const u8 *list, int n, int max, InfoFn info, const char *hint, int can_cancel, const char *confirm, int *out) {
  if (n <= 0) return 0;
  // 부르는 쪽 문자열이 fmt의 돌려쓰는 버퍼일 수 있으니 복사해 둔다
  static char title_c[96], hint_c[160], confirm_c[64];
  #define COPYS(dst, src) do { int i_ = 0; if (src) for (; (src)[i_] && i_ < (int)sizeof(dst) - 1; i_++) dst[i_] = (src)[i_]; dst[i_] = 0; } while (0)
  COPYS(title_c, title); COPYS(hint_c, hint); COPYS(confirm_c, confirm);
  title = title_c; confirm = confirm_c; hint = hint ? hint_c : 0;
  // 정렬: 이번 달 새 작품 → 최근에 나온 작품
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
  int tab = 0, cur = 0, scroll = 0, nsel = 0, dirty = 1, rows_dirty = 0;
  static u8 shown[NWORKS];
  int ns = 0;
  const int y0 = 16, VIS = 5, ROW = 16;
  for (;;) {
    if (dirty) {
      ns = 0;
      for (int i = 0; i < n; i++) if (tabs[tab] < 0 || WORKS[order[i]].type == tabs[tab]) shown[ns++] = order[i];
      if (cur >= ns) cur = ns - 1;
      if (cur < 0) cur = 0;
      if (cur < scroll) scroll = cur;
      if (cur >= scroll + VIS) scroll = cur - VIS + 1;
      clear_to(y0);
      draw_title_box(3, y0, SCREEN_W - 3, BOX_BOTTOM, title);
      // 탭
      int tx = 8;
      for (int i = 0; i < nt; i++) {
        const char *name = tabs[i] < 0 ? "전체" : TYPES[tabs[i]].folder;
        int w = text_w(name) + 8;
        if (i == tab) fill(VRAM, tx - 2, y0 + 16, w, 14, RGB(60, 60, 120));
        draw_text(VRAM, tx + 2, y0 + 16, name, i == tab ? C_WHITE : C_DIM);
        tx += w + 2;
      }
      rows_dirty = 1;
      dirty = 0;
    }
    if (rows_dirty) {
      blend_dark(VRAM, 4, y0 + 31, SCREEN_W - 8, BOX_BOTTOM - y0 - 32);
      int y = y0 + 32;
      for (int i = scroll; i < MIN(ns, scroll + VIS); i++) {
        int w = shown[i];
        if (i == cur) fill(VRAM, 6, y - 1, SCREEN_W - 12, ROW, RGB(40, 50, 120));
        draw_icon(8, y - 1, WORKS[w].type, complete(w));
        int sel = -1;
        for (int k = 0; k < nsel; k++) if (out[k] == w) sel = k;
        draw_wrapped(VRAM, 27, y, SCREEN_W - 100, 1, WORKS[w].title, sel >= 0 ? C_NAME : C_WHITE);
        const char *cnt = fmt("%d/%d", owned(w), WORKS[w].units);
        int cw = text_w(cnt);
        draw_text(VRAM, SCREEN_W - 10 - cw, y, cnt, C_DIM);
        if (WORKS[w].appear == S.turn && S.turn > 0) draw_text(VRAM, SCREEN_W - 16 - cw - 24, y, "NEW", C_RED);
        if (sel >= 0 && max > 1) { fill(VRAM, 23, y + 1, 4, 12, C_NAME); }
        y += ROW;
      }
      if (!ns) draw_text(VRAM, 20, y0 + 40, "고를 수 있는 작품이 없다.", C_DIM);
      // 아래 설명 두 줄
      int dy = y0 + 32 + VIS * ROW + 1;
      if (ns && info) draw_wrapped(VRAM, 8, dy, SCREEN_W - 16, 1, info(shown[cur]), RGB(150, 200, 220));
      const char *h2 = max > 1 ? fmt("%d/%d개 · START %s", nsel, max, confirm)
                               : fmt("A %s · L/R 종류", confirm);
      draw_wrapped(VRAM, 8, dy + LINE_H, SCREEN_W - 16, 1, hint && nsel == 0 && max > 1 ? hint : h2, C_GRAY);
      rows_dirty = 0;
    }
    vsync(); poll_keys();
    if (pressed(KEY_SELECT)) sound_toggle();
    if (pressed(KEY_UP) && ns) { cur = (cur + ns - 1) % ns; sfx(SFX_MOVE); rows_dirty = 1; }
    if (pressed(KEY_DOWN) && ns) { cur = (cur + 1) % ns; sfx(SFX_MOVE); rows_dirty = 1; }
    if (pressed(KEY_LEFT) && ns) { cur = MAX(0, cur - VIS); rows_dirty = 1; }
    if (pressed(KEY_RIGHT) && ns) { cur = MIN(ns - 1, cur + VIS); rows_dirty = 1; }
    if (pressed(KEY_R)) { tab = (tab + 1) % nt; cur = scroll = 0; sfx(SFX_MOVE); dirty = 1; }
    if (pressed(KEY_L)) { tab = (tab + nt - 1) % nt; cur = scroll = 0; sfx(SFX_MOVE); dirty = 1; }
    if (pressed(KEY_B) && can_cancel) { sfx(SFX_CLICK); return -1; }
    if (pressed(KEY_A) && ns) {
      int w = shown[cur];
      if (max == 1) { out[0] = w; sfx(SFX_CLICK); return 1; }
      int found = -1;
      for (int k = 0; k < nsel; k++) if (out[k] == w) found = k;
      if (found >= 0) { for (int k = found; k < nsel - 1; k++) out[k] = out[k + 1]; nsel--; }
      else if (nsel < max) out[nsel++] = w;
      else { for (int k = 0; k < nsel - 1; k++) out[k] = out[k + 1]; out[nsel - 1] = w; }
      sfx(SFX_CLICK); rows_dirty = 1;
    }
    if (pressed(KEY_START)) {
      if (nsel > 0) { sfx(SFX_CLICK); return nsel; }
      sfx(SFX_ERROR);
    }
  }
}

// ---------- 한글 키보드 ----------
static const u16 CHO_CP[19] = { 0x3131, 0x3132, 0x3134, 0x3137, 0x3138, 0x3139, 0x3141, 0x3142, 0x3143, 0x3145, 0x3146, 0x3147, 0x3148, 0x3149, 0x314A, 0x314B, 0x314C, 0x314D, 0x314E };
static const u16 JONG_CP[28] = { 0, 0x3131, 0x3132, 0x3133, 0x3134, 0x3135, 0x3136, 0x3137, 0x3139, 0x313A, 0x313B, 0x313C, 0x313D, 0x313E, 0x313F, 0x3140, 0x3141, 0x3142, 0x3144, 0x3145, 0x3146, 0x3147, 0x3148, 0x314A, 0x314B, 0x314C, 0x314D, 0x314E };
// 겹받침: [앞 받침, 뒤 자음(초성 번호), 결과 받침]
static const u8 DOUBLE_JONG[11][3] = { { 1, 9, 3 }, { 4, 12, 5 }, { 4, 18, 6 }, { 8, 0, 9 }, { 8, 6, 10 }, { 8, 7, 11 }, { 8, 9, 12 }, { 8, 16, 13 }, { 8, 17, 14 }, { 8, 18, 15 }, { 17, 9, 18 } };
static int jong_of_cho(int c) { for (int j = 1; j < 28; j++) if (JONG_CP[j] == CHO_CP[c]) return j; return 0; }
static int cho_of_jong(int j) { for (int c = 0; c < 19; c++) if (CHO_CP[c] == JONG_CP[j]) return c; return -1; }

static void put_utf8(char *o, int *at, u32 cp) {
  if (cp < 0x80) o[(*at)++] = (char)cp;
  else if (cp < 0x800) { o[(*at)++] = (char)(0xC0 | (cp >> 6)); o[(*at)++] = (char)(0x80 | (cp & 63)); }
  else { o[(*at)++] = (char)(0xE0 | (cp >> 12)); o[(*at)++] = (char)(0x80 | ((cp >> 6) & 63)); o[(*at)++] = (char)(0x80 | (cp & 63)); }
  o[*at] = 0;
}

// 키 배치: 자음 19 + 지우기, 모음 21, 확인
static const u16 KEYS[5][10] = {
  { 0x3131, 0x3132, 0x3134, 0x3137, 0x3138, 0x3139, 0x3141, 0x3142, 0x3143, 0x3145 },
  { 0x3146, 0x3147, 0x3148, 0x3149, 0x314A, 0x314B, 0x314C, 0x314D, 0x314E, 1 },
  { 0x314F, 0x3150, 0x3151, 0x3152, 0x3153, 0x3154, 0x3155, 0x3156, 0x3157, 0x315B },
  { 0x315C, 0x3160, 0x3161, 0x3163, 0x3158, 0x3159, 0x315A, 0x315D, 0x315E, 0x315F },
  { 0x3162, 2, 2, 2, 2, 2, 2, 2, 2, 2 },
};

#define KB_KX 10
#define KB_KY 60
#define KB_CW 22
#define KB_CH 17
static void kb_key(int r, int c, int on) {
  u16 k = KEYS[r][c];
  if (k == 2 && c > 1) return;
  if (k == 2) c = 1;
  int x = KB_KX + c * KB_CW, y = KB_KY + r * KB_CH, w = k == 2 ? KB_CW * 9 : KB_CW;
  fill(VRAM, x, y, w - 2, KB_CH - 2, on ? RGB(255, 230, 128) : RGB(200, 200, 210));
  fill(VRAM, x, y + KB_CH - 3, w - 2, 1, RGB(120, 120, 140));
  const char *label = k == 1 ? "←" : k == 2 ? "확인 (START)" : 0;
  if (label) draw_text(VRAM, x + (w - 2 - text_w(label)) / 2, y + 1, label, RGB(20, 20, 40));
  else { char t[4]; int a = 0; put_utf8(t, &a, k); draw_text(VRAM, x + 5, y + 1, t, RGB(20, 20, 40)); }
}
// 입력 칸 다시 그리기. 커서 x 위치를 돌려준다
static int kb_field(int y0, const u16 *done, int nd, int cho, int jung, int jong) {
  fill(VRAM, 10, y0 + 20, SCREEN_W - 20, 20, RGB(248, 248, 240));
  char buf[64]; int at = 0; buf[0] = 0;
  for (int i = 0; i < nd; i++) put_utf8(buf, &at, done[i]);
  if (cho >= 0 && jung >= 0) put_utf8(buf, &at, 0xAC00 + (cho * 21 + jung) * 28 + jong);
  else if (cho >= 0) put_utf8(buf, &at, CHO_CP[cho]);
  else if (jung >= 0) put_utf8(buf, &at, 0x314F + jung);
  return draw_text(VRAM, 16, y0 + 23, buf, RGB(20, 20, 40));
}

int keyboard(const char *title, char *out, int max_syl, const char *initial) {
  u16 done[12]; int nd = 0;
  const char *p = initial ? initial : "";
  while (*p && nd < max_syl) done[nd++] = (u16)utf8_next(&p);
  int cho = -1, jung = -1, jong = 0;
  int cx = 0, cy = 0;
  const int y0 = 8;
  // 처음 한 번만 전체를 그린다
  clear_to(0);
  draw_title_box(3, y0, SCREEN_W - 3, BOX_BOTTOM, title);
  for (int r = 0; r < 5; r++) for (int c = 0; c < 10; c++) kb_key(r, c, r == cy && c == cx);
  draw_text(VRAM, 10, BOX_BOTTOM - 15, "A 입력 · B 지우기 · START 확인", C_GRAY);
  int ex = kb_field(y0, done, nd, cho, jung, jong), field_dirty = 0, blink = -1;
  for (;;) {
    if (field_dirty) { ex = kb_field(y0, done, nd, cho, jung, jong); field_dirty = 0; blink = -1; }
    // 커서 깜빡임: 커서 자리만 칠한다
    int bl = (frame_count >> 5) & 1;
    if (bl != blink) { fill(VRAM, ex + 1, y0 + 24, 2, 12, bl ? RGB(20, 20, 40) : RGB(248, 248, 240)); blink = bl; }
    int ox = cx, oy = cy;
    vsync(); poll_keys();
    if (pressed(KEY_SELECT)) sound_toggle();
    if (pressed(KEY_LEFT)) { cx = (cx + 9) % 10; if (KEYS[cy][cx] == 2) cx = 1; sfx(SFX_MOVE); }
    if (pressed(KEY_RIGHT)) { cx = (cx + 1) % 10; if (KEYS[cy][cx] == 2 && cx > 1) cx = 0; sfx(SFX_MOVE); }
    if (pressed(KEY_UP)) { cy = (cy + 4) % 5; if (KEYS[cy][cx] == 2) cx = 1; sfx(SFX_MOVE); }
    if (pressed(KEY_DOWN)) { cy = (cy + 1) % 5; if (KEYS[cy][cx] == 2) cx = 1; sfx(SFX_MOVE); }
    if (ox != cx || oy != cy) { kb_key(oy, ox, 0); kb_key(cy, cx, 1); }
    int key = 0;
    if (pressed(KEY_A)) key = KEYS[cy][cx];
    if (pressed(KEY_B)) key = 1;
    if (pressed(KEY_START)) key = 2;
    if (!key) continue;
    sfx(SFX_CLICK); field_dirty = 1;
    int composing = cho >= 0 || jung >= 0;
    if (key == 2) {
      // 조합 중인 글자를 확정하고 끝낸다
      if (cho >= 0 && jung >= 0 && nd < 12) done[nd++] = (u16)(0xAC00 + (cho * 21 + jung) * 28 + jong);
      cho = jung = -1; jong = 0;
      if (nd == 0) { sfx(SFX_ERROR); continue; }
      int at = 0; out[0] = 0;
      for (int i = 0; i < nd; i++) put_utf8(out, &at, done[i]);
      last_top = 0;
      return nd;
    }
    if (key == 1) {
      if (jong) {
        int dj = -1;
        for (int i = 0; i < 11; i++) if (DOUBLE_JONG[i][2] == jong) dj = DOUBLE_JONG[i][0];
        jong = dj > 0 ? dj : 0;
      } else if (jung >= 0) jung = -1;
      else if (cho >= 0) cho = -1;
      else if (nd > 0) nd--;
      continue;
    }
    if (key >= 0x3131 && key <= 0x314E) {   // 자음
      int c = -1;
      for (int i = 0; i < 19; i++) if (CHO_CP[i] == key) c = i;
      if (c < 0) continue;
      if (cho < 0 && jung < 0) { if (nd + 1 <= max_syl) cho = c; else sfx(SFX_ERROR); }
      else if (jung < 0) { if (nd + 1 < max_syl) { done[nd++] = CHO_CP[cho]; cho = c; } }
      else if (jong == 0) {
        int j = jong_of_cho(c);
        if (j && cho >= 0) jong = j;
        else if (nd + 1 < max_syl) { done[nd++] = (u16)(cho >= 0 ? 0xAC00 + (cho * 21 + jung) * 28 : 0x314F + jung); cho = c; jung = -1; }
      } else {
        int comb = 0;
        for (int i = 0; i < 11; i++) if (DOUBLE_JONG[i][0] == jong && DOUBLE_JONG[i][1] == c) comb = DOUBLE_JONG[i][2];
        if (comb) jong = comb;
        else if (nd + 1 < max_syl) { done[nd++] = (u16)(0xAC00 + (cho * 21 + jung) * 28 + jong); cho = c; jung = -1; jong = 0; }
      }
    } else {                                 // 모음
      int v = key - 0x314F;
      if (cho < 0 && jung < 0) { if (nd + 1 <= max_syl) jung = v; }
      else if (jung < 0) jung = v;
      else if (jong) {
        // 받침이 다음 글자의 첫소리로 넘어간다 (겹받침이면 뒤쪽만)
        int keep = 0, move = jong;
        for (int i = 0; i < 11; i++) if (DOUBLE_JONG[i][2] == jong) { keep = DOUBLE_JONG[i][0]; move = jong_of_cho(DOUBLE_JONG[i][1]); }
        int nc = cho_of_jong(move);
        if (nc >= 0 && nd + 1 < max_syl) { done[nd++] = (u16)(0xAC00 + (cho * 21 + jung) * 28 + keep); cho = nc; jung = v; jong = 0; }
      } else if (nd + 1 < max_syl) {
        done[nd++] = (u16)(cho >= 0 ? 0xAC00 + (cho * 21 + jung) * 28 : 0x314F + jung);
        cho = -1; jung = v;
      }
    }
    (void)composing;
  }
}
