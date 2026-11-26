// 수집왕 GBA — 화면 쪽 진행: 일정 짜기, 내 컴퓨터, 도움말, 메인 화면, 시작
#include "common.h"

static void refresh(void) { scene_room(); }

// ---------- 일정 짜기 ----------
static const char *const ACT_SHORT[NACTS] = { "다운로드", "CD 굽기", "서점", "컴퓨터 정비", "용산", "동호회", "친구", "심부름", "공부", "일찍 자기" };

static void plan_slot(int i, int sel, int exam, const int *plan) {
  int x = 8 + i * 76;
  fill(VRAM, x, 33, 72, 30, sel ? RGB(80, 70, 30) : RGB(30, 30, 60));
  if (sel) { fill(VRAM, x, 33, 72, 1, C_NAME); fill(VRAM, x, 62, 72, 1, C_NAME); }
  draw_text(VRAM, x + 3, 33, exam && i == 0 ? "상순(시험)" : SLOT[i], C_DIM);
  draw_text(VRAM, x + 3, 47, plan[i] >= 0 ? ACT_SHORT[plan[i]] : "—", plan[i] >= 0 ? C_WHITE : C_GRAY);
}
static void plan_cell(int a, int on) {
  int x = 8 + (a % 2) * 114, yy = 67 + (a / 2) * 15;
  blend_dark(VRAM, x - 1, yy, 112, 15);
  if (on) fill(VRAM, x - 1, yy, 112, 15, RGB(40, 50, 120));
  draw_text(VRAM, x + 2, yy, ACT_NAMES[a], act_why(a) ? C_GRAY : C_WHITE);
}
static void plan_desc(int a) {
  blend_dark(VRAM, 6, 142, SCREEN_W - 12, 15);
  const char *why = act_why(a);
  draw_wrapped(VRAM, 8, 142, SCREEN_W - 16, 1, why ? why : ACT_DESC[a], why ? C_RED : RGB(150, 200, 220));
}

static int plan_month(int *plan) {
  int exam = is_exam_month();
  plan[0] = exam ? A_STUDY : -1; plan[1] = plan[2] = -1;
  int slot = exam ? 1 : 0, cur = 0;
  int y, m; date_of(S.turn, &y, &m);
  gfx_restore(0, SCREEN_H);
  draw_title_box(3, 15, SCREEN_W - 3, SCREEN_H - 2, fmt("%d년 %d월%s", y, m, exam ? " · 시험" : " 계획"));
  for (int i = 0; i < 3; i++) plan_slot(i, i == slot, exam, plan);
  for (int a = 0; a < NACTS; a++) plan_cell(a, a == cur);
  plan_desc(cur);
  int shown_start = 0;
  for (;;) {
    vsync(); poll_keys();
    if (pressed(KEY_SELECT)) sound_toggle();
    int full = plan[0] >= 0 && plan[1] >= 0 && plan[2] >= 0;
    if (full != shown_start) {
      // 다 채우면 START 안내
      fill(VRAM, SCREEN_W - 96, 16, 90, 14, RGB(16, 60, 200));
      if (full) draw_text(VRAM, SCREEN_W - 92, 16, "START 시작", C_NAME);
      shown_start = full;
    }
    int old = cur, old_slot = slot;
    if (pressed(KEY_UP)) cur = (cur + NACTS - 2) % NACTS;
    if (pressed(KEY_DOWN)) cur = (cur + 2) % NACTS;
    if (pressed(KEY_LEFT) || pressed(KEY_RIGHT)) cur ^= 1;
    if (pressed(KEY_L)) { do slot = (slot + 2) % 3; while (exam && slot == 0); }
    if (pressed(KEY_R)) { do slot = (slot + 1) % 3; while (exam && slot == 0); }
    if (pressed(KEY_A)) {
      if (act_why(cur)) sfx(SFX_ERROR);
      else {
        plan[slot] = cur; sfx(SFX_CLICK);
        plan_slot(slot, 1, exam, plan);
        for (int i = 0; i < 3; i++) if (plan[i] < 0) { slot = i; break; }
      }
    }
    if (pressed(KEY_B)) {
      sfx(SFX_CLICK);
      if (plan[slot] >= 0 && !(exam && slot == 0)) { plan[slot] = -1; plan_slot(slot, 1, exam, plan); }
      else return 0;
    }
    if (pressed(KEY_START)) {
      if (full) { sfx(SFX_CLICK); return 1; }
      sfx(SFX_ERROR);
    }
    if (old != cur) { sfx(SFX_MOVE); plan_cell(old, 0); plan_cell(cur, 1); plan_desc(cur); }
    if (old_slot != slot) { plan_slot(old_slot, 0, exam, plan); plan_slot(slot, 1, exam, plan); }
  }
}

// ---------- 내 컴퓨터 ----------
static void computer_view(void) {
  Traits t; compute_traits(&t);
  int page = 0;
  for (;;) {
    gfx_restore(0, SCREEN_H);
    static const char *const TT[4] = { "시스템", "장르 성향", "모은 매체", "수집 방식" };
    draw_title_box(3, 15, SCREEN_W - 3, SCREEN_H - 2, fmt("내 컴퓨터 - %s (%d/4)", TT[page], page + 1));
    int y = 33;
    #define ROWBAR(label, val, max, col, txt) do { draw_text(VRAM, 10, y, label, C_DIM); draw_bar(96, y + 2, 90, 10, val, max, col); draw_text(VRAM, 190, y, txt, C_WHITE); y += 15; } while (0)
    if (page == 0) {
      s32 used = hdd_used();
      ROWBAR("C: 드라이브", used / 10, S.hdd_cap / 10, used * 10 > S.hdd_cap * 9 ? RGB(200, 50, 50) : C_NAVY, mbstr(used));
      ROWBAR("조각화", S.frag, 100, RGB(200, 50, 50), fmt("%d%%", S.frag));
      ROWBAR("회선 안정성", S.line, 100, C_NAVY, fmt("%d%%", S.line));
      if (S.burner) ROWBAR("CD 드라이브", S.drive_broken ? 0 : S.drive, 100, C_NAVY, S.drive_broken ? "고장" : fmt("%d%%", S.drive));
      ROWBAR("엄마 인내심", S.mom, 100, S.mom < 30 ? RGB(200, 50, 50) : RGB(50, 140, 60), fmt("%d", S.mom));
      draw_wrapped(VRAM, 10, y, SCREEN_W - 20, 2, fmt("인터넷: %s · 공CD %d장 · 장비: %s%s%s%s", S.net ? "ADSL" : "56k 모뎀", S.blank,
                   S.burner ? "버너 " : "", S.scanner ? "스캐너 " : "", S.usb ? "USB " : "", S.fast_burner ? "고속CD-RW" : ""), RGB(150, 200, 220));

    } else if (page == 1) {
      s32 mx = 1;
      for (int g = 0; g < NGENRES; g++) mx = MAX(mx, t.genre[g]);
      for (int g = 0; g < NGENRES; g++) ROWBAR(GENRES[g].trait, t.genre[g], mx, GENRES[g].color, fmt("%d", (int)(t.genre[g] / 10)));
      draw_wrapped(VRAM, 10, y + 2, SCREEN_W - 20, 1, fmt("성향: %s", trait_label(&t)), C_NAME);
    } else if (page == 2) {
      draw_text(VRAM, 10, y, t.top >= 0 ? fmt("%s 작품 중 모은 비율", GENRES[t.top].name) : "매체 (1위 장르 작품 중)", C_NAME); y += 15;
      for (int m2 = 0; m2 < 4; m2++) ROWBAR(m2 == 1 ? "영상" : m2 == 2 ? "책" : MEDIA_NAMES[m2], t.media_pct[m2], 1000, RGB(58, 122, 154), fmt("%d%%", t.media_pct[m2] / 10));
      draw_wrapped(VRAM, 10, y + 2, SCREEN_W - 20, 2, fmt("실물 점수 %d · 서점에서 산 책, 받은 CD, 산 CD는 성향 점수가 1.5배", (int)(t.phys / 10)), RGB(150, 200, 220));
    } else {
      s32 mm = 1;
      for (int i = 0; i < 4; i++) mm = MAX(mm, t.method[i]);
      for (int i = 0; i < 4; i++) ROWBAR(METHOD_NAMES[i], t.method[i], mm, RGB(96, 96, 128), "");
    }
    draw_text(VRAM, 10, SCREEN_H - 17, "←→ 쪽 넘기기 · B 닫기", C_GRAY);
    (void)0;
    for (;;) {
      vsync(); poll_keys();
      if (pressed(KEY_SELECT)) sound_toggle();
      if (pressed(KEY_RIGHT) || pressed(KEY_A) || pressed(KEY_R)) { page = (page + 1) % 4; sfx(SFX_MOVE); break; }
      if (pressed(KEY_LEFT) || pressed(KEY_L)) { page = (page + 3) % 4; sfx(SFX_MOVE); break; }
      if (pressed(KEY_B)) { sfx(SFX_CLICK); return; }
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
    "· 조작: A 결정 · B 취소 · 십자키 이동 · L/R 목록 종류·일정 칸 · START 확인 · SELECT 소리",
  };
  notice("도움말", L, 8);
}

static const char *const MENU[4] = { "이번 달 계획", "내 컬렉션", "내 컴퓨터", "메뉴" };
static void hub_item(int i, int on) {
  int x = 9 + (i % 2) * 114, yy = 96 + 35 + (i / 2) * 15;
  blend_dark(VRAM, x - 2, yy, 110, 15);
  if (on) { fill(VRAM, x - 2, yy, 110, 15, RGB(40, 50, 120)); draw_text(VRAM, x, yy, "▶", C_NAME); }
  draw_text(VRAM, x + 13, yy, MENU[i], on ? C_WHITE : C_DIM);
}

// 메인 화면. 반환: 1 계획 확정, 0 타이틀로
int hub(int *plan) {
  bgm(BGM_ROOM);
  int cur = 0, dirty = 1;
  for (;;) {
    if (dirty) {
      S.screen = SCR_DESKTOP;
      refresh();
      int y, m; date_of(S.turn, &y, &m);
      Traits t; compute_traits(&t);
      const int y0 = 96;
      draw_box(3, y0, SCREEN_W - 3, SCREEN_H - 2);
      draw_text(VRAM, 9, y0 + 3, fmt("%d년 %d월 · %s %d학년", y, m, S.school, grade_of(S.turn)), C_NAME);
      if (S.ban_turn == S.turn) draw_text(VRAM, 9, y0 + 18, "이번 달은 컴퓨터 금지령!", C_RED);
      else draw_wrapped(VRAM, 9, y0 + 18, SCREEN_W - 18, 1, fmt("성향: %s · 용돈 +%w%s", trait_label(&t), S.last_allowance ? S.last_allowance : allowance(), is_exam_month() ? " · 시험 기간" : ""), RGB(170, 210, 230));
      for (int i = 0; i < 4; i++) hub_item(i, i == cur);
      dirty = 0;
    }
    vsync(); poll_keys();
    if (pressed(KEY_SELECT)) sound_toggle();
    int old = cur;
    if (pressed(KEY_UP) || pressed(KEY_DOWN)) cur ^= 2;
    if (pressed(KEY_LEFT) || pressed(KEY_RIGHT)) cur ^= 1;
    if (old != cur) { sfx(SFX_MOVE); hub_item(old, 0); hub_item(cur, 1); }
    if (!pressed(KEY_A) && !pressed(KEY_START)) continue;
    sfx(SFX_CLICK);
    if (cur == 0) { if (plan_month(plan)) return 1; }
    else if (cur == 1) { collection_view(); cur = 0; }
    else if (cur == 2) { computer_view(); cur = 0; }
    else {
      refresh();
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
  REG_WAITCNT = 0x4317;
  REG_DISPCNT = 0x0403;
  sound_init();
  meta_load();
  for (;;) {
    bgm(BGM_TITLE);
    scene_title();
    fill(bb, TITLE_CURSOR_X, TITLE_CURSOR_Y, TITLE_CURSOR_W, TITLE_CURSOR_H, RGB(154, 208, 255));
    gfx_present();
    S.magic = 0;
    Opt o[3] = { { "새로 시작", 0, 0 }, { "이어하기", save_exists() ? "지난번 저장한 달부터" : "저장된 게임이 없습니다", !save_exists() }, { "기록 보기", 0, 0 } };
    set_last_line(0);
    int c = choose(W_NARR, 0, 0, o, 3);
    if (c == 0) new_game_flow();
    else if (c == 1) { if (load_game()) { fade(1); scene_room(); fade(-1); hub_loop(); } }
    else records();
  }
}
