// 수집왕 GBA — 게임 상태와 규칙 (웹판 js/game.js와 같은 규칙, 점수는 x10 정수)
#include "common.h"

State S EWRAM_BSS;
Meta META EWRAM_BSS;

#ifndef NDS
// 에뮬레이터가 SRAM 세이브를 알아보도록 넣는 표시
const char SAVE_TYPE[] __attribute__((aligned(4))) = "SRAM_V113";
#endif

const char *const MEDIA_NAMES[4] = { "게임", "영상(애니·영화)", "책(만화·소설)", "음악" };
const char *const METHOD_NAMES[4] = { "아카이버", "손품장인", "나눔꾼", "완결주의자" };

// ---------- 난수 ----------
static u32 seed_rng = 0x1234567;
u32 rnd(u32 n) {
  u32 *r = S.magic ? &S.rng : &seed_rng;
  u32 x = *r ? *r : 0x9E3779B9;
  x ^= x << 13; x ^= x >> 17; x ^= x << 5;
  *r = x;
  return n ? x % n : 0;
}
int chance(int permille) { return (int)rnd(1000) < permille; }
int clamp100(int v) { return v < 0 ? 0 : v > 100 ? 100 : v; }

// ---------- 날짜 ----------
void date_of(int turn, int *y, int *m) { *y = 2000 + (turn + 2) / 12; *m = (turn + 2) % 12 + 1; }
int grade_of(int turn) { return turn / 12 + 1; }
int season_of(int turn) {   // 0 봄 1 여름 2 가을 3 겨울
  int y, m; date_of(turn, &y, &m);
  return (m >= 3 && m <= 5) ? 0 : (m >= 6 && m <= 8) ? 1 : (m >= 9 && m <= 11) ? 2 : 3;
}

// ---------- 컬렉션 ----------
int owned(int w) { const Entry *e = &S.col[w]; return e->hdd + e->cd + e->book; }
int remaining(int w) { int r = WORKS[w].units - owned(w); return r > 0 ? r : 0; }
int complete(int w) { return owned(w) >= WORKS[w].units; }
s32 hdd_used(void) {
  s32 t = 0;
  for (int i = 0; i < NWORKS; i++) t += (s32)S.col[i].hdd * TYPES[WORKS[i].type].size + S.col[i].part;
  return t;
}
s32 hdd_free(void) { s32 f = S.hdd_cap - hdd_used(); return f > 0 ? f : 0; }
int available_works(u8 *out) {
  int n = 0;
  for (int i = 0; i < NWORKS; i++) if (WORKS[i].appear <= S.turn && remaining(i) > 0) out[n++] = (u8)i;
  return n;
}
int owned_works(u8 *out) {
  int n = 0;
  for (int i = 0; i < NWORKS; i++) if (owned(i) > 0) out[n++] = (u8)i;
  return n;
}
static void mark_seen(int w) {
  S.seen[w] = 1;
  if (!(META.seen[w >> 3] & (1 << (w & 7)))) { META.seen[w >> 3] |= 1 << (w & 7); meta_save(); }
}

void add_units(int w, int n, int where, int method) {
  if (n <= 0) return;
  Entry *e = &S.col[w];
  if (where == WH_HDD) e->hdd += n; else if (where == WH_CD) e->cd += n; else e->book += n;
  mark_seen(w);
  s32 pts = (s32)n * TYPES[WORKS[w].type].weight10;
  if (method == M_SCAN || method == M_TRADE || method == M_BUY || method == M_BOOK) e->phys += n;
  if (method == M_DOWNLOAD) S.m_download += pts * 6 / 10;
  if (method == M_SCAN) S.m_scan += pts * 3;
  if (method == M_TRADE) S.m_trade += pts * 15 / 10;
}

int do_download(const int *ids, int n, s32 budget, DlRes *res, int *full, s32 *left) {
  int cnt = 0;
  *full = 0;
  for (int k = 0; k < n; k++) {
    int w = ids[k];
    Entry *e = &S.col[w];
    int size = TYPES[WORKS[w].type].size, got = 0;
    while (budget > 0 && remaining(w) > 0) {
      s32 fr = hdd_free();
      if (fr <= 0) { *full = 1; break; }
      s32 take = MIN(MIN((s32)(size - e->part), budget), fr);
      e->part += take; budget -= take;
      if (e->part >= size) { e->part = 0; add_units(w, 1, WH_HDD, M_DOWNLOAD); got++; }
    }
    if (e->part > 0 || got > 0) mark_seen(w);
    res[cnt].w = w; res[cnt].got = got; res[cnt].part = e->part; cnt++;
    if (*full) break;
  }
  S.st_downloads++;
  if (S.net == 0) S.st_modem++;
  *left = budget;
  return cnt;
}

// 큰 파일부터 굽는다. 옮긴 작품을 moved_works/moved_units(최대 12개)에 적는다
int do_burn(int max_discs, int *used_mb, int *moved_works, int *moved_units) {
  int discs = MIN(S.blank, max_discs);
  s32 cap = (s32)discs * CD_MB, mbsum = 0;
  int nm = 0;
  static const u8 ORDER[NTYPES] = { T_MOVIE, T_GAME, T_ANIME, T_MUSIC, T_MANGA, T_NOVEL };
  for (int o = 0; o < NTYPES; o++)
    for (int w = 0; w < NWORKS; w++) {
      if (WORKS[w].type != ORDER[o] || !S.col[w].hdd) continue;
      int size = TYPES[WORKS[w].type].size, n = 0;
      while (S.col[w].hdd > 0 && cap >= size) { S.col[w].hdd--; S.col[w].cd++; cap -= size; mbsum += size; n++; }
      if (n && nm < 12) { moved_works[nm] = w; moved_units[nm] = n; nm++; }
    }
  int used = (mbsum + CD_MB - 1) / CD_MB;
  S.blank -= used; S.st_discs += used; S.st_burns++;
  if (used) S.m_download += used * 5;
  *used_mb = mbsum;
  moved_works[nm] = -1;
  return used;
}

int do_format(void) {
  int lost = 0;
  for (int i = 0; i < NWORKS; i++) { lost += S.col[i].hdd; S.col[i].hdd = 0; S.col[i].part = 0; }
  S.frag = 0; S.st_formats++; S.st_lost += lost;
  return lost;
}

int virus_damage(int *units) {
  u8 list[NWORKS]; int n = 0;
  for (int i = 0; i < NWORKS; i++) if (S.col[i].hdd > 0) list[n++] = (u8)i;
  if (!n) return -1;
  int w = list[rnd(n)];
  int k = (S.col[w].hdd * 3 + 9) / 10;
  if (k < 1) k = 1;
  S.col[w].hdd -= k; S.st_lost += k;
  *units = k;
  return w;
}

int receive_disc(int w, int method) {
  int n = MIN(remaining(w), CD_MB / TYPES[WORKS[w].type].size);
  if (n < 1) n = 1;
  add_units(w, n, WH_CD, method);
  S.st_discs++;
  return n;
}

// ---------- 성향 ----------
void compute_traits(Traits *t) {
  for (int g = 0; g < NGENRES; g++) t->genre[g] = 0;
  s32 total = 0, phys = 0, cpts = 0;
  int completed = 0, oc = 0;
  for (int w = 0; w < NWORKS; w++) {
    int n = owned(w);
    if (!n) continue;
    oc++;
    int wt = TYPES[WORKS[w].type].weight10;
    s32 ph = (s32)MIN(S.col[w].phys, n) * wt;
    phys += ph;
    s32 pts = (s32)n * wt + ph / 2;
    if (n >= WORKS[w].units) { completed++; cpts += pts; }
    total += pts;
    if (WORKS[w].g1 == 255) t->genre[WORKS[w].g0] += pts;
    else { t->genre[WORKS[w].g0] += pts * 7 / 10; t->genre[WORKS[w].g1] += pts * 7 / 10; }
  }
  s32 gsum = 0; int top = 0;
  for (int g = 0; g < NGENRES; g++) { gsum += t->genre[g]; if (t->genre[g] > t->genre[top]) top = g; }
  t->top = t->genre[top] > 0 ? top : -1;
  t->top_score = t->genre[top];
  t->top_share = gsum ? (int)((t->genre[top] * 1000) / gsum) : 0;
  t->total = total; t->phys = phys; t->completed = completed; t->owned_count = oc;
  t->method[0] = S.m_download;
  t->method[1] = S.m_scan;
  t->method[2] = S.m_share + S.m_trade;
  t->method[3] = (completed >= 8 && oc <= 12 && completed * 10 >= oc * 8) ? cpts * 8 / 10 * completed / oc : 0;
  int mk = 0;
  for (int i = 1; i < 4; i++) if (t->method[i] > t->method[mk]) mk = i;
  t->method_key = t->method[mk] > 0 ? mk : -1;
  t->breadth = 0;
  if (total > 200) t->breadth = t->top_share < 220 ? 1 : t->top_share > 550 ? 2 : 0;
  // 매체: 1위 장르의 작품 중 몇 %를 모았나 (게임, 영상, 책, 음악)
  s32 got[4] = { 0 }, pool[4] = { 0 };
  for (int w = 0; w < NWORKS && t->top >= 0; w++) {
    if (WORKS[w].g0 != t->top && WORKS[w].g1 != t->top) continue;
    int ty = WORKS[w].type, m = ty == T_GAME ? 0 : (ty == T_ANIME || ty == T_MOVIE) ? 1 : (ty == T_MANGA || ty == T_NOVEL) ? 2 : 3;
    int wt = TYPES[ty].weight10;
    pool[m] += (s32)WORKS[w].units * wt;
    got[m] += (s32)owned(w) * wt;
  }
  int rank[4];
  for (int m = 0; m < 4; m++) {
    t->media_pct[m] = pool[m] ? (int)(got[m] * 1000 / pool[m]) : 0;
    rank[m] = (got[m] > 0 && got[m] >= MIN(400, pool[m] * 7 / 10)) ? t->media_pct[m] : -1;
  }
  static const int JS_ORDER[4] = { 1, 2, 0, 3 };   // 웹판과 같은 순서로 동점 처리
  int best = -1;
  for (int k = 0; k < 4; k++) {
    int m = JS_ORDER[k];
    if (best < 0 || rank[m] > rank[best] || (rank[m] == rank[best] && got[m] > got[best])) best = m;
  }
  if (rank[best] > 0) t->media_top = best;
  else if (t->top >= 0) { int bm = JS_ORDER[0]; for (int k = 1; k < 4; k++) if (got[JS_ORDER[k]] > got[bm]) bm = JS_ORDER[k]; t->media_top = bm; }
  else t->media_top = -1;
}

const char *trait_label(const Traits *t) {
  if (t->top < 0) return "아직 없음";
  const char *a = GENRES[t->top].trait;
  const char *b = t->method_key >= 0 ? METHOD_NAMES[t->method_key] : 0;
  const char *c = t->breadth == 1 ? "잡식가" : t->breadth == 2 ? "한 우물" : 0;
  if (b && c) return fmt("%s · %s · %s", a, b, c);
  if (b) return fmt("%s · %s", a, b);
  return a;
}

int decide_ending(void) {
  Traits t; compute_traits(&t);
  if (t.top < 0 || (t.method_key != 3 && t.top_score < 1100)) return E_ORDINARY;
  int g = t.top;
  static const u8 LEG[6] = { A_DOWNLOAD, A_BURN, A_SCAN, A_YONGSAN, A_CLUB, A_FRIEND };
  int legend = t.breadth == 1 && t.total >= 5000 && t.completed >= 20 && t.phys >= 1200 && S.friend >= 60;
  for (int i = 0; i < 6; i++) if (S.acts[LEG[i]] < 4) legend = 0;
  if (legend) return E_LEGEND;
  switch (t.method_key) {
    case 1: return g == G_SF ? E_GAMEDEV : E_WEBTOON;
    case 2: return (g == G_HOT || g == G_SF) ? E_YONGSAN : (g == G_LIFE || g == G_FAN) ? E_TEACHER : E_RADIO;
    case 3: return t.breadth == 2 ? E_FANCLUB : g == G_FAN ? E_NOVELIST : E_EDITOR;
    default: return ENDING_TABLE[g][t.media_top < 0 ? 1 : t.media_top];
  }
}

int decide_romance(void) {
  int c = S.grad, L = S.love;
  if (c == GRAD_CONFESS && L >= 90) return 0;
  if ((c == GRAD_CONFESS || c == GRAD_LETTER) && L >= 60) return 1;
  if (L >= 35) return 2;
  return 3;
}

int cd_price(void) { int y, m; date_of(S.turn, &y, &m); return y == 2000 ? 2000 : y == 2001 ? 1200 : 700; }
int allowance(void) {
  int y, m; date_of(MIN(S.turn, 35), &y, &m);
  int base = y == 2000 ? 8000 : y == 2001 ? 10000 : 12000;
  return MAX(0, base + S.allowance_mod);
}
int taste_likes(int w) {
  if (S.taste == 0) return WORKS[w].g0 == G_LOVE || WORKS[w].g1 == G_LOVE;
  if (S.taste == 1) return WORKS[w].g0 == G_FAN || WORKS[w].g1 == G_FAN;
  return WORKS[w].type == T_MUSIC;
}

static void copy_str(char *dst, const char *src, int max) { int i = 0; while (src[i] && i < max - 1) { dst[i] = src[i]; i++; } dst[i] = 0; }

void new_game(const char *name, const char *school) {
  u32 seed = frame_count * 2654435761u + REG_VCOUNT + 1;
  u8 *p = (u8 *)&S;
  for (unsigned i = 0; i < sizeof S; i++) p[i] = 0;
  S.magic = 0x4B4A5331;   // "1SJK"
  S.rng = seed;
  copy_str(S.name, name, sizeof S.name);
  copy_str(S.school, school, sizeof S.school);
  const char *full = fmt("%s%s", HER_SURNAMES[rnd(N_HER_SURNAMES)], HER_GIVEN[rnd(N_HER_GIVEN)]);
  copy_str(S.her, full, sizeof S.her);
  S.taste = (u8)rnd(3);
  S.same_class = 1;
  S.money = 10000; S.blank = 3; S.hdd_cap = 3000;
  S.frag = 10; S.line = 80; S.drive = 100;
  S.mom = 70; S.friend = 30; S.love = 10;
  S.ban_turn = -1; S.start_done = -1;
}

// ---------- 저장 (SRAM) ----------
#define META_AT 0
#define SAVE_AT 512
#ifdef NDS
// NDS판: 카드 저장 칩 (nds/src/arm9/save.c)
#define sram_write save_mem_write
#define sram_read save_mem_read
#else
static void sram_write(int at, const void *src, int n) { const u8 *p = src; for (int i = 0; i < n; i++) SRAM[at + i] = p[i]; }
static void sram_read(int at, void *dst, int n) { u8 *p = dst; for (int i = 0; i < n; i++) p[i] = SRAM[at + i]; }
#endif

int save_exists(void) {
#ifndef NDS
  volatile const char *t = SAVE_TYPE; (void)t[0];
#endif
  u32 m; sram_read(SAVE_AT, &m, 4);
  return m == 0x4B4A5331;
}
void save_game(void) { sram_write(SAVE_AT, &S, sizeof S); }
int load_game(void) { if (!save_exists()) return 0; sram_read(SAVE_AT, &S, sizeof S); return 1; }
void save_clear(void) { u32 z = 0; sram_write(SAVE_AT, &z, 4); }
void meta_load(void) {
  sram_read(META_AT, &META, sizeof META);
  if (META.magic != 0x4154454D) { u8 *p = (u8 *)&META; for (unsigned i = 0; i < sizeof META; i++) p[i] = 0; META.magic = 0x4154454D; }
}
void meta_save(void) { sram_write(META_AT, &META, sizeof META); }
