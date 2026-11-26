// 수집왕 — 진행(GBA·NDS 공통): 활동, 위험·회복 이벤트, 월말 정리, 한 판의 흐름, 컬렉션, 기록, 새 게임
#include "common.h"

const char *const ACT_NAMES[NACTS] = { "밤새 다운로드", "CD 굽기", "서점 가기", "컴퓨터 정비", "용산 원정", "동호회 활동", "친구와 놀기", "심부름·집안일", "공부", "일찍 자기" };
const char *const ACT_DESC[NACTS] = {
  "작품을 골라 인터넷 속도만큼 받는다.", "하드의 파일을 CD로 옮겨 안전하게 보관.", "만화 ₩3,500·소설 ₩6,000. 스캐너 있으면 스캔.",
  "조각 모음, 스캔디스크, 회선 점검.", "차비 ₩2,000. 공CD·부품·하드·중고 CD.", "CD를 우편 교환. 공CD 1장+우표 ₩1,000.",
  "PC방·떡볶이 ₩4,000. CD를 주고받기도.", "용돈 +₩4,000, 엄마 인내심 회복.", "엄마 인내심 회복. 시험 성적→용돈.", "엄마 인내심 회복. 다운로드는 쉰다.",
};
const char *const SLOT[3] = { "상순", "중순", "하순" };
int is_exam_month(void) { int y, m; date_of(S.turn, &y, &m); return m == 4 || m == 7 || m == 10 || m == 12; }
static void refresh(void) { scene_room(); }
static s32 parts_mb(void) { s32 t = 0; for (int i = 0; i < NWORKS; i++) t += S.col[i].part; return t; }

const char *act_why(int a) {
  int banned = S.ban_turn == S.turn;
  switch (a) {
    case A_DOWNLOAD: if (banned) return "컴퓨터 금지령"; if (hdd_free() < 1) return "하드가 꽉 찼다"; return 0;
    case A_BURN:
      if (banned) return "컴퓨터 금지령";
      if (!S.burner) return "CD 버너가 없다";
      if (S.drive_broken) return "드라이브 고장";
      if (S.blank <= 0) return "공CD가 없다";
      if (hdd_used() - parts_mb() < 1) return "구울 파일이 없다";
      return 0;
    case A_SCAN: return S.money < MANGA_PRICE ? "책 살 돈이 없다" : 0;
    case A_PC: return banned ? "컴퓨터 금지령" : 0;
    case A_YONGSAN: return S.money < 2000 ? "차비가 없다" : 0;
    case A_CLUB: if (S.blank <= 0) return "보낼 공CD가 없다"; if (S.money < 1000) return "우표값이 없다"; return 0;
  }
  return 0;
}

// ---------- 위험 이벤트 ----------
static s32 ev_mom_shutdown(s32 budget, const char **note) {
  sfx(SFX_THUMP);
  Traits t; compute_traits(&t);
  S.screen = SCR_OFF; refresh();
  narr("새벽 두 시. 다운로드 창의 진행 막대를 멍하니 보고 있는데—");
  say(W_MOM, X_ANGRY, fmt("%s! 지금 몇 시야! 당장 꺼!", S.name));
  narr("딸깍. 엄마가 멀티탭 스위치를 내렸다.");
  sfx(SFX_OFF);
  Opt o[3] = { { "사과하고 잔다", "오늘 받은 것까지만. 엄마 인내심 회복", 0 }, { "엄마가 잠들면 몰래 다시 켠다", "대부분 받지만 들키면 큰일", 0 }, { "(탐정파) 모니터만 꺼 두고 본체는 살린다", "조용히 계속 받는다", 0 } };
  int n = (t.top == G_MYS || (S.flags & F_TRICK)) ? 3 : 2;
  int c = choose(W_NARR, 0, "어떻게 할까?", o, n);
  if (c == 0) { S.mom = clamp100(S.mom + 12); *note = "엄마에게 들켜 다운로드가 중간에 끊겼다."; return budget * 3 / 10; }
  if (c == 2) { S.mom = clamp100(S.mom - 4); *note = "모니터만 끄고 본체 팬 소리를 이불로 막았다."; return budget * 8 / 10; }
  if (chance(450)) {
    S.mom = clamp100(S.mom - 25); S.ban_turn = S.turn + 1;
    say(W_MOM, X_ANGRY, "내가 못 볼 줄 알았니? 다음 달은 컴퓨터 금지야!");
    *note = "몰래 켰다가 들켰다. 다음 달은 컴퓨터 금지령.";
    return budget * 35 / 100;
  }
  S.mom = clamp100(S.mom - 8);
  *note = "숨죽이고 다시 켰다. 들키지 않았다.";
  return budget * 85 / 100;
}

static s32 ev_disconnect(s32 budget, const char **note) {
  sfx(SFX_ERROR);
  Traits t; compute_traits(&t);
  narr(S.net == 0 ? "뚜- 뚜- 뚜-. 누군가 전화기를 들었다. 모뎀 연결이 끊겼다." : "새벽, 모뎀 불빛이 깜빡이더니 꺼졌다. 인터넷이 끊겼다.");
  Opt o[3]; int map[3], n = 0;
  if ((S.flags & F_RESUME) || t.method_key == 0) { o[n] = (Opt){ "이어받기로 다시 시작", (S.flags & F_RESUME) ? "동호회 형에게 배운 기술" : "(아카이버) 손실 없음", 0 }; map[n++] = 0; }
  o[n] = (Opt){ "다시 연결해 본다", "조금은 되찾을 수 있다", 0 }; map[n++] = 1;
  o[n] = (Opt){ "포기하고 잔다", "받은 데까지만", 0 }; map[n++] = 2;
  int c = map[choose(W_NARR, 0, "연결이 끊어졌습니다.", o, n)];
  S.line = clamp100(S.line - 5);
  if (c == 0) { *note = "끊긴 곳부터 이어받았다. 손실 없음."; return budget; }
  if (c == 1) { if (S.net == 0) S.mom = clamp100(S.mom - 5); *note = "몇 번 만에 다시 연결됐다. 처음부터 다시 받은 파일도 있었다."; return budget * 7 / 10; }
  *note = "인터넷이 끊겨 절반밖에 받지 못했다.";
  return budget / 2;
}

static void damage_notice(int w, int units, const char *prefix) {
  const char *L[1];
  L[0] = w >= 0 ? fmt("%s「%s」 %d%s 손상되어 지워졌다.", prefix, WORKS[w].title, units, josa(TYPES[WORKS[w].type].unit, "이/가")) : "다행히 손상된 파일은 없었다.";
  notice("검사 결과", L, 1);
}

static void ev_virus(void) {
  sfx(SFX_ALARM);
  S.screen = SCR_DESKTOP; refresh();
  narr("다음 날 아침, 컴퓨터를 켜자 화면에 이상한 창이 수십 개 떴다.");
  narr("…바이러스다.");
  Opt o[3]; int map[3], n = 0;
  o[n] = (Opt){ "백신으로 검사한다", "일부 파일이 손상될 수 있다", 0 }; map[n++] = 0;
  if (S.st_club > 0) { o[n] = (Opt){ "동호회 형에게 도와 달라고 한다", "동호회 활동을 해서 가능", 0 }; map[n++] = 1; }
  o[n] = (Opt){ "포맷한다", "하드에만 있던 파일은 전부 사라진다", 0 }; map[n++] = 2;
  int c = map[choose(W_NARR, 0, "어떻게 할까?", o, n)];
  if (c == 1) {
    say(W_HYUNG, X_SMILE, "그거 요즘 도는 웜이야. 내가 알려 주는 대로 해 봐. 레지스트리 열고…");
    narr("한 시간 만에 깨끗해졌다. 형은 역시 밤의 제왕이었다.");
    S.frag = clamp100(S.frag + 5);
    return;
  }
  int units = 0;
  if (c == 0) {
    progress("백신 검사", "바이러스를 찾는 중…", 80);
    int w = virus_damage(&units);
    S.frag = clamp100(S.frag + 10);
    damage_notice(w, units, "바이러스는 치료했지만, ");
    return;
  }
  int files = 0;
  for (int i = 0; i < NWORKS; i++) files += S.col[i].hdd;
  Opt o2[2] = { { "포맷한다", 0, 0 }, { "그만두고 백신 검사", 0, 0 } };
  if (choose(W_NARR, 0, fmt("정말 포맷할까? 하드에만 있는 파일 %d개가 사라진다. CD로 구운 파일은 안전하다.", files), o2, 2) == 1) {
    int w = virus_damage(&units);
    damage_notice(w, units, "");
    return;
  }
  progress("FORMAT C:", "C: 드라이브를 포맷하는 중…", 120);
  int lost = do_format();
  refresh();
  const char *L[] = { fmt("하드디스크가 깨끗해졌다. 사라진 파일: %d개.", lost), "윈도우를 새로 깔았다. 바탕화면이 낯설 만큼 깨끗했다.", "CD로 구워 둔 것들은 무사했다." };
  notice("포맷 완료", L, 3);
}

static void ev_drive_broken(void) {
  sfx(SFX_ERROR);
  Traits t; compute_traits(&t);
  S.drive_broken = 1;
  narr("위이잉- 탁. 탁. 탁.");
  narr("CD 트레이가 열리지도 닫히지도 않는다. 굽던 CD는 은빛 코스터가 되었다.");
  narr("CD 드라이브가 고장 났다.");
  Opt o[3]; int map[3], n = 0;
  if (t.top == G_SF) { o[n] = (Opt){ "(메카·SF광) 직접 분해해서 고쳐 본다", "성공할 수도, 아닐 수도", 0 }; map[n++] = 0; }
  o[n] = (Opt){ "아빠에게 말한다", "고쳐 줄지도 모른다", 0 }; map[n++] = 1;
  o[n] = (Opt){ "나중에 용산에서 고친다", fmt("수리비 %w", 15000), 0 }; map[n++] = 2;
  int c = map[choose(W_NARR, 0, "어떻게 할까?", o, n)];
  if (c == 0) {
    if (chance(600)) { S.drive_broken = 0; S.drive = 60; narr("드라이버로 나사를 풀고, 렌즈를 면봉으로 닦고, 벨트를 다시 걸었다."); narr("…돌아간다! 나는 천재일지도 모른다."); }
    else narr("나사가 하나 남았다. 어디 거였는지 모르겠다. 드라이브는 여전히 고장이다.");
  } else if (c == 1) {
    if (S.mom >= 50 && chance(700)) { S.drive_broken = 0; S.drive = 100; say(W_DAD, X_SMILE, "드라이브 하나 새로 사 왔다. 대신 엄마 말 잘 들어라."); narr("새 드라이브다!"); }
    else say(W_DAD, X_ANGRY, "요즘 성적도 그런데 CD만 굽고 있었냐? 네 용돈으로 고쳐.");
  } else narr("다음에 용산에 가면 고쳐야겠다.");
}

// ---------- 활동 ----------
static const char *dl_info(int w) {
  const Entry *e = &S.col[w];
  int size = TYPES[WORKS[w].type].size;
  s32 left = (s32)remaining(w) * size - e->part;
  if (e->part) return fmt("%s · %d/%d%s(+%d%%) · 남은 %m", GENRES[WORKS[w].g0].name, owned(w), WORKS[w].units, TYPES[WORKS[w].type].unit, e->part * 100 / size, left);
  return fmt("%s · %d/%d%s · %s당 %m · 남은 %m", GENRES[WORKS[w].g0].name, owned(w), WORKS[w].units, TYPES[WORKS[w].type].unit, TYPES[WORKS[w].type].unit, size, left);
}

static void act_download(const char *label) {
  s32 speed = S.net ? 2000 : 150;
  s32 budget = speed * (250 - S.frag) / 250;
  u8 list[NWORKS];
  int n = available_works(list);
  if (!n) { const char *L[] = { "지금 구할 수 있는 건 이미 다 모았다. 오랜만에 푹 잤다." }; notice(label, L, 1); return; }
  int ids[3];
  int k = pick_works(fmt("%s · 다운로드 목록", label), list, n, 3, dl_info, fmt("이번 밤 예상 %m · 남은 하드 %m · START로 받기", budget, hdd_free()), 0, "받기 시작", ids);
  refresh();
  if (k <= 0) return;
  const char *note = 0;
  int momP = S.mom < 45 ? (45 - S.mom) * 1000 / 60 + 50 : 20;
  if (chance(momP)) budget = ev_mom_shutdown(budget, &note);
  else if (chance((S.net ? 70 : 160) + (100 - S.line) * 4)) budget = ev_disconnect(budget, &note);
  S.screen = SCR_DESKTOP; refresh();
  if (S.net == 0) { sfx(SFX_MODEM); S.phone_nights++; }
  progress("다운로드 중…", fmt("%s%s · %s", WORKS[ids[0]].title, k > 1 ? " 외" : "", S.net ? "180KB/s" : "4.8KB/s"), 96);
  DlRes r[3]; int full; s32 left;
  int nr = do_download(ids, k, budget, r, &full, &left);
  sfx(SFX_DONE);
  S.frag = clamp100(S.frag + (S.net ? 9 : 5));
  S.line = clamp100(S.line - (S.net == 0 ? 6 : 4));
  S.mom = clamp100(S.mom - 9);
  refresh();
  const char *L[8]; int nl = 0;
  if (note) L[nl++] = note;
  for (int i = 0; i < nr; i++) {
    const Work *w = &WORKS[r[i].w];
    const char *unit = TYPES[w->type].unit;
    const char *got = r[i].got ? fmt("+%d%s", r[i].got, unit) : "진행 중";
    const char *pct = r[i].part ? fmt(" · 다음 %s %d%% 받음", unit, r[i].part * 100 / TYPES[w->type].size) : "";
    L[nl++] = fmt("%s: %s%s%s", w->title, got, pct, complete(r[i].w) ? " ★완결" : "");
  }
  if (full) L[nl++] = "하드디스크가 꽉 찼다! CD로 굽지 않으면 더 받을 수 없다.";
  else if (left > 50 && budget > 0) L[nl++] = fmt("목록을 다 받고도 %m만큼 시간이 남았다.", left);
  if (S.frag >= 60) L[nl++] = fmt("디스크 조각화 %d%% — 다운로드가 점점 느려진다.", S.frag);
  notice(fmt("%s · 다운로드 결과", label), L, nl);
  if (chance(S.net ? 60 : 40)) ev_virus();
  else if (hdd_free() * 10 < S.hdd_cap && !full) { const char *W[] = { "C: 드라이브의 공간이 부족합니다.", fmt("남은 공간: %m", hdd_free()) }; notice("경고", W, 2); }
}

static void act_burn(const char *label) {
  int maxd = S.fast_burner ? 10 : 6;
  sfx(SFX_TRAY);
  progress("CD 굽기", fmt("공CD에 굽는 중… (최대 %d장)", MIN(S.blank, maxd)), 96);
  sfx(SFX_BURN);
  int breakP = S.drive < 55 ? (60 - S.drive) * 1000 / 70 : 20;
  int limit = maxd, broke = 0;
  if (chance(breakP)) { broke = 1; limit = MIN(S.blank, maxd) / 2; }
  int mbsum, mw[13], mu[13];
  int used = do_burn(limit, &mbsum, mw, mu);
  S.drive = clamp100(S.drive - (S.fast_burner ? 3 + used : 6 + used * 3));
  S.frag = clamp100(S.frag + 3);
  S.mom = clamp100(S.mom - 2);
  refresh();
  const char *L[10]; int nl = 0;
  L[nl++] = fmt("CD %d장을 구웠다. (%m)", used, mbsum);
  for (int i = 0; mw[i] >= 0 && i < 5; i++) L[nl++] = fmt("· %s %d%s", WORKS[mw[i]].title, mu[i], TYPES[WORKS[mw[i]].type].unit);
  L[nl++] = fmt("매직으로 CD에 제목을 적었다. 남은 공CD %d장 · 드라이브 상태 %d%%", S.blank, S.drive);
  notice(fmt("%s · 굽기 결과", label), L, nl);
  if (broke) ev_drive_broken();
}

static const char *book_info(int w) { return fmt("%s · %d/%d권 · 권당 %w", GENRES[WORKS[w].g0].name, owned(w), WORKS[w].units, WORKS[w].type == T_NOVEL ? NOVEL_PRICE : MANGA_PRICE); }

static void act_scan(const char *label) {
  u8 list[NWORKS], all[NWORKS];
  int na = available_works(all), n = 0;
  for (int i = 0; i < na; i++) {
    int w = all[i], price = WORKS[w].type == T_NOVEL ? NOVEL_PRICE : MANGA_PRICE;
    if ((WORKS[w].type == T_MANGA || WORKS[w].type == T_NOVEL) && S.money >= price) list[n++] = (u8)w;
  }
  if (!n) { const char *L[] = { S.money < MANGA_PRICE ? "서점에 갔지만 책 살 돈이 없었다." : "서점에 있는 책은 이미 다 샀다." }; notice(fmt("%s · 서점", label), L, 1); return; }
  int out[1];
  pick_works(fmt("%s · 서점에서 무엇을 살까?", label), list, n, 1, book_info, 0, 0, S.scanner ? "사러 가기 (만화는 스캔까지)" : "사러 가기", out);
  refresh();
  int w = out[0], price = WORKS[w].type == T_NOVEL ? NOVEL_PRICE : MANGA_PRICE;
  int scan = S.scanner && WORKS[w].type == T_MANGA;
  int maxb = MIN(MIN(6, remaining(w)), S.money / price);
  if (scan) maxb = MIN(maxb, (int)(hdd_free() / TYPES[WORKS[w].type].size));
  if (maxb <= 0) { const char *L[] = { "하드에 스캔본을 저장할 공간이 없다. 먼저 CD로 굽자." }; notice(label, L, 1); return; }
  Opt o[6]; int vals[6], no = 0;
  static const int CH[5] = { 1, 2, 3, 4, 6 };
  for (int i = 0; i < 5; i++) if (CH[i] <= maxb) { o[no] = (Opt){ fmt("%d권 사기", CH[i]), wonstr(CH[i] * price), 0 }; vals[no++] = CH[i]; }
  if (vals[no - 1] != maxb) { o[no] = (Opt){ fmt("%d권 사기", maxb), wonstr(maxb * price), 0 }; vals[no++] = maxb; }
  int nb = vals[choose(W_NARR, 0, fmt("「%s」 몇 권을 살까? (한 번에 최대 6권)", WORKS[w].title), o, no)];
  S.money -= nb * price;
  sfx(SFX_COIN);
  if (scan) {
    sfx(SFX_SCANNER);
    progress("스캔 중…", fmt("「%s」 한 장, 한 장… (300dpi)", WORKS[w].title), 100);
    add_units(w, nb, WH_HDD, M_SCAN);
    S.frag = clamp100(S.frag + 3);
    refresh();
    static const char *const R3[3] = { "책을 너무 세게 눌러서 책등이 갈라졌다. 그래도 스캔본은 완벽하다.", "삐뚤어진 페이지는 다시 스캔했다. 세 번이나.", "스캐너 불빛이 지나갈 때마다 방이 환해졌다." };
    const char *L[] = { fmt("「%s」 %d권을 사서 스캔해 JPG로 저장했다. (%m) 책은 책장에.", WORKS[w].title, nb, nb * TYPES[WORKS[w].type].size), R3[rnd(3)], complete(w) ? "★ 완결까지 모았다!" : fmt("%d/%d권", owned(w), WORKS[w].units) };
    notice(fmt("%s · 서점과 스캔", label), L, 3);
    return;
  }
  add_units(w, nb, WH_BOOK, M_BOOK);
  refresh();
  const char *L[] = {
    fmt("「%s」 %d권을 사서 책장에 꽂았다. 비닐 냄새가 좋았다.", WORKS[w].title, nb),
    WORKS[w].type == T_MANGA ? "스캐너가 있으면 이 만화를 파일로도 만들 수 있을 텐데." : "한 장 한 장 넘기는 맛은 TXT 파일이 따라올 수 없다.",
    complete(w) ? "★ 완결까지 모았다!" : fmt("%d/%d권 · 실물은 성향 점수가 1.5배", owned(w), WORKS[w].units),
  };
  notice(fmt("%s · 서점", label), L, 3);
}

static void act_pc(const char *label) {
  int before = S.frag;
  defrag_anim(MAX(before, 20));
  S.frag = 0; S.line = clamp100(S.line + 20); S.st_defrags++;
  refresh();
  const char *L[4]; int nl = 0;
  L[nl++] = fmt("디스크 조각 모음 완료. 조각화 %d%% → 0%%", before);
  L[nl++] = "스캔디스크도 돌렸다. 하드디스크가 상쾌하게 돌아간다.";
  L[nl++] = "모뎀과 전화선 연결도 점검했다. (회선 안정성 회복)";
  if (S.burner && !S.drive_broken && S.drive < 100) { S.drive = clamp100(S.drive + 10); L[nl++] = "CD 드라이브 먼지도 털었다. (+10%)"; }
  notice(fmt("%s · 컴퓨터 정비", label), L, nl);
}

typedef struct { const char *name, *desc; } ShopItem;
enum { SH_CD10, SH_CD30, SH_LENS, SH_REPAIR, SH_SCANNER, SH_BURNER, SH_HDD20, SH_HDD40, SH_USB, SH_USED, SH_BUNDLE, SH_BACK };
static const ShopItem SHOP[] = {
  { "공CD 10장 묶음", "700MB CD-R." }, { "공CD 30장 스핀들", "케이크 통에 든 공CD. 많이 굽는 사람용." }, { "렌즈 클리너", "CD 드라이브 상태 +30" },
  { "CD 드라이브 수리", "고장 난 드라이브를 고친다." }, { "중고 평판 스캐너", "만화책 스캔 활동 해금." }, { "고속 CD-RW (24배속)", "한 번에 10장까지 굽고, 잘 고장 나지 않는다." },
  { "20GB 하드디스크 추가", "내 몫 저장 공간 +12GB" }, { "40GB 하드디스크 추가", "내 몫 저장 공간 +25GB" }, { "USB 메모리 128MB", "파일을 들고 다닐 수 있다. 귀중품." },
  { "중고 CD 뽑기", "영화나 게임이 구워진 중고 CD 한 장" }, { "중고 CD 묶음 고르기", "원하는 작품을 CD 3장 분량까지" }, { "돌아간다", 0 },
};
static int round100(int v) { return (v + 50) / 100 * 100; }
static int shop_price(int id) {
  switch (id) {
    case SH_CD10: return round100(cd_price() * 10 * 9 / 10);
    case SH_CD30: return round100(cd_price() * 30 * 8 / 10);
    case SH_LENS: return 5000; case SH_REPAIR: return 15000; case SH_SCANNER: return 45000; case SH_BURNER: return 40000;
    case SH_HDD20: return 80000; case SH_HDD40: return 110000; case SH_USB: return 35000; case SH_USED: return 6000; case SH_BUNDLE: return BUNDLE_PRICE;
  }
  return 0;
}
static int shop_avail(int id, u8 *used_pool, int *nused) {
  switch (id) {
    case SH_CD10: case SH_CD30: case SH_BACK: return 1;
    case SH_LENS: return S.burner;
    case SH_REPAIR: return S.burner && S.drive_broken;
    case SH_SCANNER: return !S.scanner && S.turn >= 10;
    case SH_BURNER: return S.burner && !S.fast_burner && S.turn >= 12;
    case SH_HDD20: return !S.hdd20 && S.turn >= 12;
    case SH_HDD40: return !S.hdd40 && S.turn >= 24;
    case SH_USB: return !S.usb && S.turn >= 24;
    case SH_USED: {
      *nused = 0;
      for (int w = 0; w < NWORKS; w++) if (WORKS[w].appear <= S.turn && remaining(w) > 0 && (WORKS[w].type == T_MOVIE || WORKS[w].type == T_GAME)) used_pool[(*nused)++] = (u8)w;
      return *nused > 0;
    }
    case SH_BUNDLE: return S.turn >= 12;
  }
  return 0;
}
static const char *bundle_info(int w) { return fmt("%d/%d%s · CD 3장에 최대 %d%s", owned(w), WORKS[w].units, TYPES[WORKS[w].type].unit, MIN(remaining(w), CD_MB / TYPES[WORKS[w].type].size * 3), TYPES[WORKS[w].type].unit); }

static void act_yongsan(void) {
  S.money -= 2000;
  narr(fmt("지하철 1호선, 용산역. 육교를 건너면 전자상가의 냄새가 났다. (차비 %w)", 2000));
  static const char *const HI[3] = { "어이 학생, 또 왔네? 오늘은 뭐 찾아?", "공CD 들어왔어. 케이크 통으로 사 가.", "학생, 요즘 뭐 굽길래 그렇게 자주 와?" };
  say(W_OWNER, X_SMILE, HI[rnd(3)]);
  for (;;) {
    refresh();
    Opt o[12]; int ids[12], n = 0;
    u8 used_pool[NWORKS]; int nused = 0;
    for (int id = 0; id <= SH_BACK; id++) {
      if (!shop_avail(id, used_pool, &nused)) continue;
      int price = shop_price(id);
      o[n] = (Opt){ id == SH_BACK ? SHOP[id].name : fmt("%s  %w", SHOP[id].name, price),
                    id == SH_CD10 ? fmt("700MB CD-R. 요즘 시세 장당 %w.", cd_price()) : SHOP[id].desc, id != SH_BACK && S.money < price };
      ids[n++] = id;
    }
    int c = ids[choose(W_OWNER, X_SMILE, fmt("가진 돈 %w", S.money), o, n)];
    if (c == SH_BACK) break;
    int price = shop_price(c);
    if (c == SH_USED) {
      shop_avail(SH_USED, used_pool, &nused);
      S.money -= price;
      int w = used_pool[rnd(nused)];
      int k = receive_disc(w, M_TRADE);
      sfx(SFX_ITEM);
      say(W_OWNER, X_SMILE, "이건 덤이야. 잘 돌아갈 거야, 아마.");
      narr(fmt("중고 CD에는 「%s」 들어 있었다! (+%d%s)", josa(WORKS[w].title, "이/가"), k, TYPES[WORKS[w].type].unit));
      continue;
    }
    if (c == SH_BUNDLE) {
      u8 list[NWORKS]; int nl = available_works(list), out[1];
      int k = pick_works("중고 CD 묶음 고르기", list, nl, 1, bundle_info, 0, 1, fmt("%w에 사기", BUNDLE_PRICE), out);
      if (k <= 0) continue;
      int w = out[0], got = 0;
      S.money -= price;
      for (int i = 0; i < 3 && remaining(w) > 0; i++) got += receive_disc(w, M_BUY);
      sfx(SFX_ITEM);
      refresh();
      say(W_OWNER, X_SMILE, "그거 찾는 사람 많아. 잘 골랐네.");
      narr(fmt("「%s」 +%d%s (CD 보관)", WORKS[w].title, got, TYPES[WORKS[w].type].unit));
      continue;
    }
    S.money -= price;
    sfx(SFX_COIN);
    switch (c) {
      case SH_CD10: S.blank += 10; break;
      case SH_CD30: S.blank += 30; break;
      case SH_LENS: S.drive = clamp100(S.drive + 30); break;
      case SH_REPAIR: S.drive_broken = 0; S.drive = 90; break;
      case SH_SCANNER: S.scanner = 1; break;
      case SH_BURNER: S.fast_burner = 1; S.drive_broken = 0; S.drive = 100; break;
      case SH_HDD20: S.hdd20 = 1; S.hdd_cap += 12000; break;
      case SH_HDD40: S.hdd40 = 1; S.hdd_cap += 25000; break;
      case SH_USB: S.usb = 1; break;
    }
    if (c == SH_HDD20 || c == SH_HDD40) {
      sfx(SFX_STARTUP); refresh();
      const char *L[] = { fmt("%s 직접 달았다. 점퍼 설정 때문에 두 번 다시 열었다.", josa(SHOP[c].name, "을/를")), fmt("저장 공간: %m", S.hdd_cap) };
      notice("하드디스크 추가", L, 2);
      continue;
    }
    static const char *const BYE[3] = { "고마워, 학생. 또 와.", "좋은 선택이야.", "그건 내가 보장하지." };
    say(W_OWNER, X_SMILE, BYE[rnd(3)]);
  }
}

static const char *club_info(int w) { return fmt("%s · %d/%d%s · CD 1장에 최대 %d%s", GENRES[WORKS[w].g0].name, owned(w), WORKS[w].units, TYPES[WORKS[w].type].unit, CD_MB / TYPES[WORKS[w].type].size, TYPES[WORKS[w].type].unit); }

static void act_club(const char *label) {
  S.st_club++;
  narr("동호회 게시판에 접속했다. 새벽에도 사람들이 바글바글하다.");
  static const char *const HI[3] = { "왔냐? 오늘 좋은 거 들어왔다.", "요즘 뭐 모으냐? 형한테 말해 봐.", "자막 새로 나온 거 봤냐?" };
  say(W_HYUNG, X_SMILE, S.st_club == 1 ? "어, 신입이네? 반가워. 여기 규칙은 하나야. 받았으면 나눠라." : HI[rnd(3)]);
  if (S.st_club == 2 && !(S.flags & F_RESUME)) {
    S.flags |= F_RESUME;
    say(W_HYUNG, X_SMILE, "너 아직도 끊기면 처음부터 받냐? 이어받기 기능 있는 프로그램 써. 링크 줄게.");
    const char *L[] = { "\"이어받기\"를 배웠다.", "인터넷이 끊겨도 손실 없이 다시 받을 수 있다." };
    notice("새 기술", L, 2);
  }
  u8 pool[NWORKS]; int np = available_works(pool);
  if (!np) { const char *L[] = { "이미 모든 걸 가지고 있었다. 형이 혀를 찼다." }; notice(label, L, 1); return; }
  // 섞어서 3개 (종류가 겹치지 않게 먼저)
  for (int i = np - 1; i > 0; i--) { int j = rnd(i + 1); u8 t = pool[i]; pool[i] = pool[j]; pool[j] = t; }
  u8 offers[3]; int no = 0;
  for (int i = 0; i < np && no < 3; i++) {
    int dup = 0;
    for (int k = 0; k < no; k++) if (WORKS[offers[k]].type == WORKS[pool[i]].type) dup = 1;
    if (!dup || no >= 2) offers[no++] = pool[i];
  }
  int out[1];
  pick_works(fmt("%s · 교환할 CD 고르기", label), offers, no, 1, club_info, 0, 0, "교환하기", out);
  int w = out[0];
  S.blank--; S.money -= 1000;
  int k = receive_disc(w, M_TRADE);
  sfx(SFX_ITEM);
  u8 tmp[NWORKS];
  if (owned_works(tmp) > 1) S.m_share += 30;
  refresh();
  const char *L[3]; int nl = 0;
  L[nl++] = fmt("내 CD를 구워 우편으로 보내고, 형의 CD를 받았다. (공CD 1장, 우표 %w)", 1000);
  L[nl++] = fmt("「%s」 +%d%s (CD 보관)", WORKS[w].title, k, TYPES[WORKS[w].type].unit);
  if (complete(w)) L[nl++] = "★ 완결!";
  notice(fmt("%s · 교환 결과", label), L, nl);
  if (chance(60)) ev_virus();
}

static void act_friend(void) {
  S.mom = clamp100(S.mom - 2);
  if (S.money < 4000) {
    S.friend = clamp100(S.friend + 5);
    narr("주머니에 돈이 없어서 운동장에서 공만 찼다.");
    say(W_FRIEND, X_SMILE, "다음엔 PC방 가자. 너 돈 생기면.");
    return;
  }
  S.money -= 4000;
  S.friend = clamp100(S.friend + 12);
  static const char *const PLAY[3] = { "태식이와 PC방에 갔다. 스타크래프트 한 판에 한 시간이 순식간이었다.", "오락실에서 철권을 했다. 태식이는 늘 같은 캐릭터만 한다.", "학교 끝나고 떡볶이를 먹고, 문방구 앞 게임기에서 놀았다." };
  narr(PLAY[rnd(3)]);
  u8 tmp[NWORKS];
  if (chance(500) && owned_works(tmp)) {
    say(W_FRIEND, X_SMILE, rnd(2) ? "야, 너 그거 있지? 하나만 구워 주라. 형이 보고 싶대." : "그거 나도 좀 보자. CD로 구워 줄 수 있냐?");
    Opt o[3] = { { "구워 준다", S.blank > 0 ? "공CD 1장" : "공CD가 없다", S.blank <= 0 }, { "내 CD를 빌려준다", "돌아올지는 모른다", 0 }, { "거절한다", 0, 0 } };
    int c = choose(W_NARR, 0, 0, o, 3);
    if (c == 0) { S.blank--; S.m_share += 80; S.friend = clamp100(S.friend + 8); S.st_gifts++; say(W_FRIEND, X_SMILE, "역시 너밖에 없다! 다음에 떡볶이 쏜다."); return; }
    if (c == 1) {
      S.m_share += 60; S.friend = clamp100(S.friend + 5);
      u8 cdw[NWORKS]; int nc = 0;
      for (int w = 0; w < NWORKS; w++) if (S.col[w].cd > 0) cdw[nc++] = (u8)w;
      if (nc && chance(300)) {
        int w = cdw[rnd(nc)]; S.col[w].cd--; S.st_lost++;
        say(W_FRIEND, X_SAD, "야… 미안. 그 CD 동생이 긁어 놨어.");
        narr(fmt("「%s」 CD 한 장이 돌아오지 않았다.", WORKS[w].title));
      } else say(W_FRIEND, X_SMILE, "다 보면 바로 돌려줄게!");
      return;
    }
    S.friend = clamp100(S.friend - 6);
    say(W_FRIEND, X_ANGRY, "치사하다, 진짜.");
    return;
  }
  if (chance(350)) {
    u8 pool[NWORKS]; int np = 0;
    for (int w = 0; w < NWORKS; w++) if (WORKS[w].appear <= S.turn && remaining(w) > 0 && (WORKS[w].type == T_GAME || WORKS[w].type == T_MUSIC)) pool[np++] = (u8)w;
    if (np) {
      int w = pool[rnd(np)];
      int k = receive_disc(w, M_TRADE);
      say(W_FRIEND, X_SMILE, "이거 우리 형이 구운 건데, 너 가져.");
      narr(fmt("「%s」 CD를 받았다! (+%d%s)", WORKS[w].title, k, TYPES[WORKS[w].type].unit));
    }
  }
}

void run_activity(int act, int slot) {
  const char *label = SLOT[slot];
  const char *why = act_why(act);
  refresh();
  if (why && act != A_SCAN) {
    const char *L[] = { fmt("%s. 할 수 없어서 그냥 일찍 잤다.", why) };
    notice(fmt("%s · %s", label, ACT_NAMES[act]), L, 1);
    S.mom = clamp100(S.mom + 4);
    return;
  }
  S.acts[act]++;
  switch (act) {
    case A_DOWNLOAD: act_download(label); break;
    case A_BURN: act_burn(label); break;
    case A_SCAN: act_scan(label); break;
    case A_PC: act_pc(label); break;
    case A_YONGSAN: act_yongsan(); break;
    case A_CLUB: act_club(label); break;
    case A_FRIEND: act_friend(); break;
    case A_CHORES: {
      S.money += 4000; S.mom = clamp100(S.mom + 10); sfx(SFX_COIN); refresh();
      static const char *const C3[3] = { "설거지를 하고 분리수거를 했다.", "슈퍼에 두부 심부름을 다녀왔다. 거스름돈은 내 것.", "베란다 청소를 했다. 먼지가 엄청났다." };
      const char *L[] = { C3[rnd(3)], fmt("용돈 +%w · 엄마 인내심 회복", 4000) };
      notice(fmt("%s · 심부름·집안일", label), L, 2);
      break;
    }
    case A_STUDY: {
      S.mom = clamp100(S.mom + 14); S.month_study++;
      static const char *const S3[3] = { "문제집을 풀었다. 수학은 여전히 어렵다.", "영어 단어를 외웠다. 다음 날 반은 잊었다.", "독서실에 갔다. 반은 잤다." };
      const char *L[] = { S3[rnd(3)], "엄마 인내심이 크게 회복되었다." };
      notice(fmt("%s · 공부", label), L, 2);
      break;
    }
    case A_REST: {
      S.mom = clamp100(S.mom + 9); S.line = clamp100(S.line + 3); S.screen = SCR_OFF; refresh();
      const char *L[] = { "오랜만에 열한 시 전에 잤다. 꿈에서도 다운로드 창이 보였다.", "엄마 인내심 회복" };
      notice(fmt("%s · 일찍 자기", label), L, 2);
      break;
    }
  }
}

// ---------- 월말 정리 ----------
static void exam_result(void) {
  int score = S.month_study + (S.mom >= 60 ? 1 : 0);
  if (score >= 3) {
    S.allowance_mod = 2000; S.money += 5000; S.mom = clamp100(S.mom + 10); sfx(SFX_COIN);
    say(W_DAD, X_SMILE, "성적이 올랐네? 잘했다. 이건 보너스.");
    const char *L[] = { "성적이 올랐다!", fmt("보너스 %w · 다음 시험까지 용돈 +%w", 5000, 2000) };
    notice("성적표", L, 2);
  } else if (score == 2) {
    S.allowance_mod = 0;
    const char *L[] = { "성적은 평소와 비슷했다.", "용돈은 그대로." };
    notice("성적표", L, 2);
  } else {
    S.allowance_mod = -2000; S.mom = clamp100(S.mom - 10);
    say(W_MOM, X_ANGRY, "이 점수가 뭐니? 컴퓨터만 하더니. 다음 시험까지 용돈 깎는다.");
    const char *L[] = { "성적이 떨어졌다.", fmt("다음 시험까지 용돈 -%w", 2000), "시험 기간에 공부를 더 하거나, 엄마 인내심을 60 이상으로 유지하면 성적이 오릅니다." };
    notice("성적표", L, 3);
  }
}

void month_upkeep(void) {
  S.mom = clamp100(S.mom + 4);
  if (S.phone_nights > 0) {
    int bill = S.phone_nights * 2000, paid = MIN(bill, S.money);
    S.money -= paid;
    if (paid < bill) S.mom = clamp100(S.mom - 8);
    const char *L[] = { fmt("이번 달 밤새 모뎀을 쓴 날: %d일", S.phone_nights), fmt("엄마가 전화요금 몫으로 %w 가져갔다.%s", paid, paid < bill ? " (모자란 만큼 잔소리를 들었다)" : "") };
    notice("전화요금", L, 2);
    S.phone_nights = 0;
  }
  if (is_exam_month()) exam_result();
  if (S.mom < 15 && S.ban_turn != S.turn + 1) {
    S.ban_turn = S.turn + 1;
    say(W_MOM, X_ANGRY, "너 요즘 컴퓨터밖에 안 하지? 다음 달은 컴퓨터 금지야. 공부해!");
    const char *L[] = { "다음 달에는 다운로드, 굽기, 정비를 할 수 없다.", "엄마 인내심: 공부, 심부름, 일찍 자기로 회복할 수 있습니다." };
    notice("컴퓨터 금지령", L, 2);
    S.mom = clamp100(S.mom + 20);
    return;
  }
  if (S.net && S.line < 45 && chance(500)) {
    S.line = 100;
    narr("인터넷이 자꾸 끊긴다고 신고했더니, 기사님이 와서 단자함을 고쳐 주었다.");
    narr("회선 안정성이 완전히 회복되었다.");
    return;
  }
  if (S.burner && !S.drive_broken && S.drive < 40 && chance(250)) {
    S.drive = clamp100(S.drive + 40);
    say(W_DAD, X_SMILE, "컴퓨터 가게 지나다가 렌즈 클리너라는 게 있길래 사 왔다.");
    narr("CD 드라이브 상태가 좋아졌다.");
    return;
  }
  if (S.frag >= 70 && chance(350)) {
    say(W_HYUNG, X_SMILE, "너 요즘 다운 속도 느리다며? 조각 모음 한 번 돌려. 밤에 켜 두고 자.");
    Opt o[2] = { { "돌려 두고 잔다", 0, 0 }, { "귀찮다", 0, 0 } };
    if (choose(W_NARR, 0, "오늘 밤 조각 모음을 돌려 둘까?", o, 2) == 0) {
      defrag_anim(S.frag); S.frag = 0; S.st_defrags++; refresh();
      const char *L[] = { "조각 모음이 끝났다. 하드디스크가 다시 빨라졌다." };
      notice("디스크 조각 모음", L, 1);
    }
  }
}

// ---------- 컬렉션 ----------
static const char *col_info(int w) {
  const Entry *e = &S.col[w];
  if (!owned(w)) return "없음 (본 적 있음)";
  return fmt("%s · %d/%d%s · HDD %d · CD %d · 책 %d", GENRES[WORKS[w].g0].name, owned(w), WORKS[w].units, TYPES[WORKS[w].type].unit, e->hdd, e->cd, e->book);
}
void collection_view(void) {
  u8 list[NWORKS]; int n = 0, seen = 0;
  for (int w = 0; w < NWORKS; w++) {
    int s = owned(w) || S.seen[w] || (META.seen[w >> 3] & (1 << (w & 7)));
    if (s) { list[n++] = (u8)w; seen++; }
  }
  if (!n) { const char *L[] = { "아직 모은 작품이 없다.", fmt("??? 아직 만나지 못한 작품 %d개", NWORKS) }; notice("C:\\내 컬렉션", L, 2); return; }
  Traits t; compute_traits(&t);
  int out[1];
  for (;;) {
    int k = pick_works(fmt("내 컬렉션 · %d작품 · 완결 %d", t.owned_count, t.completed), list, n, 1, col_info,
                       0, 1, fmt("자세히 (만난 작품 %d/%d)", seen, NWORKS), out);
    if (k <= 0) break;
    int w = out[0];
    const char *L[] = { WORKS[w].title, fmt("%s · %s%s", TYPES[WORKS[w].type].name, GENRES[WORKS[w].g0].name, WORKS[w].g1 != 255 ? fmt(", %s", GENRES[WORKS[w].g1].name) : ""), col_info(w), complete(w) ? "★ 완결" : fmt("%d%s 남음", remaining(w), TYPES[WORKS[w].type].unit) };
    notice("작품 정보", L, 4);
  }
}

// 게임 한 판. 반환하면 타이틀로
void hub_loop(void) {
  while (S.turn < TOTAL_TURNS) {
    if (S.start_done != S.turn) {
      S.month_study = 0;
      S.last_allowance = allowance();
      S.money += S.last_allowance;
      S.screen = SCR_DESKTOP;
      refresh();
      bgm(BGM_ROOM);
      story_month_start();
      S.start_done = S.turn;
      save_game();
    }
    int plan[3];
    if (!hub(plan)) return;
    for (int i = 0; i < 3; i++) run_activity(plan[i], i);
    S.screen = SCR_OFF;
    refresh();
    month_upkeep();
    story_month_end();
    S.turn++;
    save_game();
  }
  story_graduation();
  story_ending();
  save_clear();
}

// ---------- 타이틀 ----------
void records(void) {
  const char *L[NENDINGS + 8]; int n = 0, ne = 0, nr = 0, seen = 0;
  for (int e = 0; e < NENDINGS; e++) if (META.endings & (1u << e)) ne++;
  static const char *const ROM[4] = { "함께", "그 시절의 편지", "다시 만남", "기억 속의 그 아이" };
  for (int r = 0; r < 4; r++) if (META.romance & (1u << r)) nr++;
  for (int w = 0; w < NWORKS; w++) if (META.seen[w >> 3] & (1 << (w & 7))) seen++;
  L[n++] = fmt("진로 엔딩 %d/%d · 에필로그 %d/4 · 만난 작품 %d/%d · 플레이 %d번", ne, NENDINGS, nr, seen, NWORKS, META.plays);
  for (int r = 0; r < 4; r++) L[n++] = (META.romance & (1u << r)) ? ROM[r] : "??? (에필로그)";
  // 본 엔딩만 이름을 보여 준다 (fmt 버퍼를 쓰지 않도록 이름 그대로)
  for (int e = 0; e < NENDINGS; e++) if (META.endings & (1u << e)) L[n++] = ENDINGS[e].title;
  if (ne < NENDINGS) L[n++] = "??? 아직 보지 못한 엔딩이 더 있다.";
  notice("기록", L, n);
}

void new_game_flow(void) {
  char name[40], school[64];
  scene_title();
  if (keyboard("학생증 · 이름을 지어 주세요 (1~6자)", name, 6, 0) <= 0) return;
  scene_title();
  Opt o[2] = { { "한빛중학교", "기본 학교 이름", 0 }, { "직접 짓기", "학교 이름을 입력합니다 (뒤에 '중학교'가 붙어요)", 0 } };
  int c = choose(W_NARR, 0, fmt("%s의 학교는?", name), o, 2);
  if (c == 0) { int i = 0; const char *d = "한빛중학교"; while (d[i]) { school[i] = d[i]; i++; } school[i] = 0; }
  else {
    char base[40];
    if (keyboard("학교 이름 (중학교 앞부분, 1~5자)", base, 5, 0) <= 0) { const char *d = "한빛"; int i = 0; while (d[i]) { base[i] = d[i]; i++; } base[i] = 0; }
    const char *f = fmt("%s중학교", base);
    int i = 0; while (f[i] && i < 60) { school[i] = f[i]; i++; } school[i] = 0;
  }
  new_game(name, school);
  save_clear();
  fade(1);
  S.screen = SCR_OFF;
  scene_room();
  fade(-1);
  story_prologue();
  hub_loop();
}

