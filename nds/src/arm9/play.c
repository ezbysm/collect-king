// 수집왕 NDS — 화면 쪽 진행: 메인 화면(바탕화면 아이콘), 일정 짜기, 내 컴퓨터, 도움말, 시작
// 활동·이벤트·한 판의 흐름은 GBA판과 같은 gba/src/activity.c
#include "common.h"

static void refresh(void) { scene_room(); }

// ---------- 바탕화면 아이콘 (32x32, 코드로 그린다) ----------
static void icon_calendar(u16 *d, int x, int y) {
  fill(d, x + 3, y + 4, 26, 26, C_WHITE); win_frame(d, x + 3, y + 4, 26, 26, 0);
  fill(d, x + 4, y + 5, 24, 7, RGB(200, 40, 40));
  fill(d, x + 9, y + 2, 2, 6, W_SHADE); fill(d, x + 21, y + 2, 2, 6, W_SHADE);
  for (int r = 0; r < 3; r++) for (int c = 0; c < 4; c++) fill(d, x + 6 + c * 6, y + 14 + r * 5, 4, 3, r == 1 && c == 2 ? RGB(200, 40, 40) : RGB(120, 120, 140));
}
static void icon_folder(u16 *d, int x, int y) {
  fill(d, x + 3, y + 7, 12, 4, RGB(220, 180, 40));
  fill(d, x + 3, y + 10, 26, 18, RGB(240, 200, 60));
  fill(d, x + 3, y + 27, 26, 1, RGB(160, 120, 20)); fill(d, x + 28, y + 10, 1, 18, RGB(160, 120, 20));
  // 빼꼼 나온 CD
  for (int yy = -6; yy <= 6; yy++) for (int xx = -6; xx <= 6; xx++) if (xx * xx + yy * yy <= 40 && y + 10 + yy < y + 11) d[(y + 10 + yy) * SCREEN_W + x + 18 + xx] = (xx * xx + yy * yy <= 3) ? RGB(150, 150, 170) : RGB(210, 220, 240);
  fill(d, x + 6, y + 14, 20, 2, RGB(255, 230, 120));
}
static void icon_computer(u16 *d, int x, int y) {
  fill(d, x + 4, y + 3, 24, 19, RGB(225, 220, 200)); win_frame(d, x + 4, y + 3, 24, 19, 0);
  fill(d, x + 7, y + 6, 18, 13, RGB(0, 128, 128));
  fill(d, x + 9, y + 8, 6, 2, C_WHITE); fill(d, x + 9, y + 12, 9, 2, RGB(240, 240, 120));
  fill(d, x + 12, y + 22, 8, 3, RGB(200, 195, 175));
  fill(d, x + 6, y + 25, 20, 4, RGB(225, 220, 200)); win_frame(d, x + 6, y + 25, 20, 4, 0);
}
static void icon_menu(u16 *d, int x, int y) {
  static const u16 *dummy; (void)dummy;
  u16 c[4] = { RGB(230, 60, 40), RGB(60, 180, 60), RGB(40, 90, 220), RGB(240, 200, 40) };
  for (int i = 0; i < 4; i++) {
    int ox = x + 6 + (i % 2) * 11, oy = y + 6 + (i / 2) * 11;
    for (int r = 0; r < 9; r++) fill(d, ox + (r < 3 ? 1 : 0), oy + r, 9, 1, c[i]);
  }
}
static const char *const MENU[4] = { "이번 달 계획", "내 컬렉션", "내 컴퓨터", "메뉴" };
#define ICON_X(i) (40 + ((i) % 2) * 112)
#define ICON_Y(i) (22 + ((i) / 2) * 70)
static void hub_icon(int i, int on) {
  u16 *d = BOT.vram;
  int x = ICON_X(i), y = ICON_Y(i);
  restore(&BOT, x - 8, y, 80, 60);
  int ix = x + 16;
  if (i == 0) icon_calendar(d, ix, y);
  else if (i == 1) icon_folder(d, ix, y);
  else if (i == 2) icon_computer(d, ix, y);
  else icon_menu(d, ix, y);
  int tw = text_w(MENU[i]);
  int lx = x + 32 - tw / 2;
  if (on) fill(d, lx - 2, y + 36, tw + 4, 14, W_NAVY);
  draw_text(d, lx, y + 36, MENU[i], C_WHITE);
}
static void hub_bottom(int cur) {
  bottom_desktop();
  for (int i = 0; i < 4; i++) hub_icon(i, i == cur);
  draw_text(BOT.vram, 8, TASKBAR_Y - 16, "아이콘을 누르세요", RGB(180, 230, 230));
}
static void hub_top(void) {
  S.screen = SCR_DESKTOP;
  refresh();
  int y, m; date_of(S.turn, &y, &m);
  Traits t; compute_traits(&t);
  const int y0 = 134;
  dark_box(&TOP, 2, y0, SCREEN_W - 2, SCREEN_H - 2);
  draw_text(TOP.vram, 9, y0 + 4, fmt("%d년 %d월 · %s %d학년", y, m, S.school, grade_of(S.turn)), C_NAME);
  if (S.ban_turn == S.turn) draw_text(TOP.vram, 9, y0 + 21, "이번 달은 컴퓨터 금지령!", C_RED);
  else draw_wrapped(TOP.vram, 9, y0 + 21, SCREEN_W - 18, 2, fmt("성향: %s · 용돈 +%w%s", trait_label(&t), S.last_allowance ? S.last_allowance : allowance(), is_exam_month() ? " · 시험 기간" : ""), RGB(170, 210, 230), 1);
}

// ---------- 일정 짜기 ----------
static const char *const ACT_SHORT[NACTS] = { "다운로드", "CD 굽기", "서점", "컴퓨터 정비", "용산", "동호회", "친구", "심부름", "공부", "일찍 자기" };
#define SLOT_X(i) (6 + (i) * 82)
#define SLOT_Y 20
#define ACT_X(a) (7 + ((a) % 2) * 121)
#define ACT_Y(a) (56 + ((a) / 2) * 22)
static void plan_slot(int i, int sel, int exam, const int *plan) {
  u16 *d = BOT.vram;
  int x = SLOT_X(i);
  fill(d, x, SLOT_Y, 80, 32, sel ? RGB(255, 250, 220) : C_WHITE);
  win_frame(d, x, SLOT_Y, 80, 32, 1);
  if (sel) { fill(d, x + 2, SLOT_Y + 2, 76, 2, RGB(220, 160, 0)); }
  draw_text_flat(d, x + 4, SLOT_Y + 3, exam && i == 0 ? "상순 (시험)" : SLOT[i], W_GRAYTX);
  draw_text_flat(d, x + 4, SLOT_Y + 17, plan[i] >= 0 ? ACT_SHORT[plan[i]] : "—", plan[i] >= 0 ? W_TEXT : W_GRAYTX);
}
static void plan_cell(int a, int on) {
  win_button(BOT.vram, ACT_X(a), ACT_Y(a), 119, 21, ACT_NAMES[a], act_why(a) ? 3 : on ? 1 : 0);
}
static void plan_desc(int a, const int *plan) {
  restore(&TOP, 0, 118, SCREEN_W, SCREEN_H - 118);
  dark_box(&TOP, 2, 120, SCREEN_W - 2, SCREEN_H - 2);
  const char *why = act_why(a);
  draw_text(TOP.vram, 9, 124, ACT_NAMES[a], why ? C_GRAY : C_NAME);
  draw_wrapped(TOP.vram, 9, 140, SCREEN_W - 18, 2, why ? why : ACT_DESC[a], why ? C_RED : C_INFO, 1);
  int x = 9;
  for (int i = 0; i < 3; i++) x = draw_text(TOP.vram, x, 172, fmt("%s%s ", i ? "→ " : "", plan[i] >= 0 ? ACT_SHORT[plan[i]] : "?"), plan[i] >= 0 ? C_WHITE : C_GRAY);
  top_overlay = 1;
}
static void plan_taskbar(int full) {
  u16 *d = BOT.vram;
  win_button(d, SCREEN_W - 136, TASKBAR_Y + 3, 52, 15, "취소", 0);
  win_button(d, SCREEN_W - 80, TASKBAR_Y + 3, 76, 15, "시작 ▶", full ? 0 : 3);
}

static int plan_month(int *plan) {
  int exam = is_exam_month();
  plan[0] = exam ? A_STUDY : -1; plan[1] = plan[2] = -1;
  int slot = exam ? 1 : 0, cur = 0;
  int y, m; date_of(S.turn, &y, &m);
  present(&TOP);
  bottom_desktop();
  // 작업 표시줄 시계 자리를 비우고 단추를 둔다
  fill(BOT.vram, 52, TASKBAR_Y + 1, SCREEN_W - 52, 19, W_FACE);
  win_window(BOT.vram, 2, 2, SCREEN_W - 4, TASKBAR_Y - 4, fmt("%d년 %d월%s", y, m, exam ? " 계획 · 시험 기간" : " 계획"));
  for (int i = 0; i < 3; i++) plan_slot(i, i == slot, exam, plan);
  for (int a = 0; a < NACTS; a++) plan_cell(a, a == cur);
  plan_desc(cur, plan);
  int shown_full = -1;
  for (;;) {
    int full = plan[0] >= 0 && plan[1] >= 0 && plan[2] >= 0;
    if (full != shown_full) { plan_taskbar(full); shown_full = full; }
    frame();
    int old = cur, old_slot = slot, assign = -1;
    if (pressed(KEY_UP)) cur = (cur + NACTS - 2) % NACTS;
    if (pressed(KEY_DOWN)) cur = (cur + 2) % NACTS;
    if (pressed(KEY_LEFT) || pressed(KEY_RIGHT)) cur ^= 1;
    if (pressed(KEY_L)) { do slot = (slot + 2) % 3; while (exam && slot == 0); }
    if (pressed(KEY_R)) { do slot = (slot + 1) % 3; while (exam && slot == 0); }
    if (pressed(KEY_A)) assign = cur;
    int start = pressed(KEY_START) != 0, cancel = 0;
    if (pressed(KEY_TOUCH)) {
      for (int i = 0; i < 3; i++) if (tapped_in(SLOT_X(i), SLOT_Y, 80, 32) && !(exam && i == 0)) slot = i;
      for (int a = 0; a < NACTS; a++) if (tapped_in(ACT_X(a), ACT_Y(a), 119, 21)) { cur = a; assign = a; }
      if (tapped_in(SCREEN_W - 136, TASKBAR_Y + 1, 52, 19)) cancel = 1;
      if (tapped_in(SCREEN_W - 80, TASKBAR_Y + 1, 76, 19)) start = 1;
    }
    if (old != cur) { sfx(SFX_MOVE); plan_cell(old, 0); plan_cell(cur, 1); plan_desc(cur, plan); }
    if (assign >= 0) {
      if (act_why(assign)) sfx(SFX_ERROR);
      else {
        plan[slot] = assign; sfx(SFX_CLICK);
        plan_slot(slot, 1, exam, plan);
        for (int i = 0; i < 3; i++) if (plan[i] < 0) { slot = i; break; }
        plan_desc(cur, plan);
      }
    }
    if (pressed(KEY_B)) {
      sfx(SFX_CLICK);
      if (plan[slot] >= 0 && !(exam && slot == 0)) { plan[slot] = -1; plan_slot(slot, 1, exam, plan); plan_desc(cur, plan); }
      else cancel = 1;
    }
    if (old_slot != slot) { plan_slot(old_slot, 0, exam, plan); plan_slot(slot, 1, exam, plan); }
    if (cancel) { sfx(SFX_CLICK); present(&TOP); return 0; }
    if (start) {
      if (plan[0] >= 0 && plan[1] >= 0 && plan[2] >= 0) { sfx(SFX_CLICK); present(&TOP); return 1; }
      sfx(SFX_ERROR);
    }
  }
}

// ---------- 내 컴퓨터 ----------
static void computer_view(void) {
  Traits t; compute_traits(&t);
  int page = 0;
  static const char *const TT[4] = { "시스템", "장르 성향", "모은 매체", "수집 방식" };
  u16 *d = BOT.vram;
  for (;;) {
    bottom_desktop();
    fill(d, 52, TASKBAR_Y + 1, SCREEN_W - 52, 19, W_FACE);
    win_button(d, SCREEN_W - 60, TASKBAR_Y + 3, 56, 15, "닫기", 0);
    win_window(d, 2, 2, SCREEN_W - 4, TASKBAR_Y - 4, "내 컴퓨터");
    int tx = 7;
    int tabx[5];
    for (int i = 0; i < 4; i++) { int w = text_w(TT[i]) + 12; tabx[i] = tx; win_button(d, tx, 20, w, 18, TT[i], i == page ? 1 : 0); tx += w + 2; }
    tabx[4] = tx;
    int y = 44;
    #define ROWBAR(label, val, max, col, txt) do { draw_text_flat(d, 10, y, label, W_TEXT); win_bar(d, 96, y + 1, 104, 12, val, max, col); draw_text_flat(d, 206, y, txt, W_TEXT); y += 16; } while (0)
    if (page == 0) {
      s32 used = hdd_used();
      ROWBAR("C: 드라이브", used / 10, S.hdd_cap / 10, used * 10 > S.hdd_cap * 9 ? W_RED : W_NAVY, mbstr(used));
      ROWBAR("조각화", S.frag, 100, W_RED, fmt("%d%%", S.frag));
      ROWBAR("회선 안정성", S.line, 100, W_NAVY, fmt("%d%%", S.line));
      if (S.burner) ROWBAR("CD 드라이브", S.drive_broken ? 0 : S.drive, 100, W_NAVY, S.drive_broken ? "고장" : fmt("%d%%", S.drive));
      ROWBAR("엄마 인내심", S.mom, 100, S.mom < 30 ? W_RED : RGB(50, 140, 60), fmt("%d", S.mom));
      draw_wrapped(d, 10, y + 2, SCREEN_W - 24, 3, fmt("인터넷: %s · 공CD %d장 · 저장 공간 %m · 장비: %s%s%s%s", S.net ? "ADSL" : "56k 모뎀", S.blank, S.hdd_cap,
                   S.burner ? "버너 " : "", S.scanner ? "스캐너 " : "", S.usb ? "USB " : "", S.fast_burner ? "고속CD-RW" : ""), W_BLUE, 0);
    } else if (page == 1) {
      s32 mx = 1;
      for (int g = 0; g < NGENRES; g++) mx = MAX(mx, t.genre[g]);
      for (int g = 0; g < NGENRES; g++) ROWBAR(GENRES[g].trait, t.genre[g], mx, (u16)(0x8000 | GENRES[g].color), fmt("%d", (int)(t.genre[g] / 10)));
      draw_wrapped(d, 10, y + 4, SCREEN_W - 24, 2, fmt("성향: %s", trait_label(&t)), W_BLUE, 0);
    } else if (page == 2) {
      draw_text_flat(d, 10, y, t.top >= 0 ? fmt("%s 작품 중 모은 비율", GENRES[t.top].name) : "매체 (1위 장르 작품 중)", W_BLUE); y += 17;
      for (int m2 = 0; m2 < 4; m2++) ROWBAR(m2 == 1 ? "영상" : m2 == 2 ? "책" : MEDIA_NAMES[m2], t.media_pct[m2], 1000, RGB(58, 122, 154), fmt("%d%%", t.media_pct[m2] / 10));
      draw_wrapped(d, 10, y + 4, SCREEN_W - 24, 3, fmt("실물 점수 %d · 서점에서 산 책, 받은 CD, 산 CD는 성향 점수가 1.5배", (int)(t.phys / 10)), W_BLUE, 0);
    } else {
      s32 mm = 1;
      for (int i = 0; i < 4; i++) mm = MAX(mm, t.method[i]);
      for (int i = 0; i < 4; i++) ROWBAR(METHOD_NAMES[i], t.method[i], mm, RGB(96, 96, 128), "");
      draw_wrapped(d, 10, y + 4, SCREEN_W - 24, 3, "다운로드·스캔·교환·완결 중 어떤 방식으로 모았는지가 엔딩에 영향을 줍니다.", W_GRAYTX, 0);
    }
    for (;;) {
      frame();
      int np = page;
      if (pressed(KEY_RIGHT) || pressed(KEY_A) || pressed(KEY_R)) np = (page + 1) % 4;
      if (pressed(KEY_LEFT) || pressed(KEY_L)) np = (page + 3) % 4;
      if (pressed(KEY_TOUCH)) for (int i = 0; i < 4; i++) if (tapped_in(tabx[i], 20, tabx[i + 1] - tabx[i], 18)) np = i;
      if (np != page) { page = np; sfx(SFX_MOVE); break; }
      if (pressed(KEY_B) || tapped_in(SCREEN_W - 60, TASKBAR_Y + 1, 56, 19)) { sfx(SFX_CLICK); return; }
    }
  }
}

static void help(void) {
  const char *L[] = {
    "· 매달 일정 3칸을 고릅니다. 시험 기간(4·7·10·12월)엔 첫 칸이 공부로 고정돼요.",
    "· 다운로드는 인터넷 속도만큼 받고, 하드 공간이 차면 멈춥니다. 다 못 받은 파일은 다음에 이어서 받아요.",
    "· CD 굽기: 하드의 파일을 CD로 옮겨 공간을 비우고, 포맷에도 안전하게 지킵니다.",
    "· 무엇을(장르·매체) 어떻게(다운로드·스캔·교환·완결) 모았는지가 엔딩을 정합니다.",
    "· 엄마 인내심이 낮으면 밤에 컴퓨터를 꺼 버립니다. 조각화가 높으면 다운로드가 느려집니다.",
    "· 공CD 값은 해마다 내려갑니다. 친구·동호회·용산에는 돈이 들고, 모뎀 시절엔 밤샌 날마다 전화요금 몫을 냅니다.",
    "· 서점에서 산 책, 받은 CD, 산 CD는 실물이라 성향 점수가 1.5배입니다.",
    "· 조작: 아래 화면을 터치하거나 A 결정 · B 취소 · 십자키 이동 · L/R 목록 종류·일정 칸 · START 확인 · SELECT 소리",
  };
  notice("도움말", L, 8);
}

// 메인 화면. 반환: 1 계획 확정, 0 타이틀로
int hub(int *plan) {
  bgm(BGM_ROOM);
  int cur = 0, dirty = 1;
  for (;;) {
    if (dirty) { hub_top(); hub_bottom(cur); dirty = 0; }
    frame();
    int old = cur, go = 0;
    if (pressed(KEY_UP) || pressed(KEY_DOWN)) cur ^= 2;
    if (pressed(KEY_LEFT) || pressed(KEY_RIGHT)) cur ^= 1;
    if (pressed(KEY_TOUCH)) for (int i = 0; i < 4; i++) if (tapped_in(ICON_X(i) - 4, ICON_Y(i), 72, 54)) { cur = i; go = 1; }
    if (old != cur) { sfx(SFX_MOVE); hub_icon(old, 0); hub_icon(cur, 1); }
    if (pressed(KEY_A) || pressed(KEY_START)) go = 1;
    if (!go) continue;
    sfx(SFX_CLICK);
    if (old != cur) wait_frames(4);
    if (cur == 0) { if (plan_month(plan)) return 1; }
    else if (cur == 1) { collection_view(); cur = 0; }
    else if (cur == 2) { computer_view(); cur = 0; }
    else {
      Opt o[4] = { { "계속하기", 0, 0 }, { "도움말", 0, 0 }, { sound_on ? "소리 끄기 (SELECT)" : "소리 켜기 (SELECT)", 0, 0 }, { "저장하고 타이틀로", "이번 달 처음부터 이어집니다", 0 } };
      int c = choose(W_NARR, 0, "메뉴", o, 4);
      if (c == 1) help();
      if (c == 2) sound_toggle();
      if (c == 3) { save_game(); return 0; }
    }
    dirty = 1;
  }
}

int main(void) {
  gfx_init();
  sound_init();
  poll_input();
  meta_load();
  for (;;) {
    bgm(BGM_TITLE);
    scene_title();
    fill(TOP.bb, TITLE_CURSOR_X, TITLE_CURSOR_Y, TITLE_CURSOR_W, TITLE_CURSOR_H, RGB(154, 208, 255));
    present(&TOP);
    S.magic = 0;
    Opt o[3] = { { "새로 시작", 0, 0 }, { "이어하기", save_exists() ? "지난번 저장한 달부터" : "저장된 게임이 없습니다", !save_exists() }, { "기록 보기", 0, 0 } };
    set_last_line("수집왕 2000-2003");
    int c = choose(W_NARR, 0, 0, o, 3);
    if (c == 0) new_game_flow();
    else if (c == 1) { if (load_game()) { fade(1); scene_room(); fade(-1); hub_loop(); } }
    else records();
  }
}
