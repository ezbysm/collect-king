// 수집왕 GBA — 공통 정의
#pragma once
#include "data_gen.h"
#include "bg_data.h"
#include "font.h"

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef signed char s8;
typedef short s16;
typedef int s32;

// ---------- 하드웨어 ----------
#define REG_DISPCNT   (*(volatile u16 *)0x04000000)
#define REG_VCOUNT    (*(volatile u16 *)0x04000006)
#define REG_KEYINPUT  (*(volatile u16 *)0x04000130)
#define REG_WAITCNT   (*(volatile u16 *)0x04000204)
#define REG_DMA3SAD   (*(volatile u32 *)0x040000D4)
#define REG_DMA3DAD   (*(volatile u32 *)0x040000D8)
#define REG_DMA3CNT   (*(volatile u32 *)0x040000DC)
#define VRAM          ((u16 *)0x06000000)
#define SRAM          ((volatile u8 *)0x0E000000)
#define SCREEN_W 240
#define SCREEN_H 160

#define KEY_A      0x0001
#define KEY_B      0x0002
#define KEY_SELECT 0x0004
#define KEY_START  0x0008
#define KEY_RIGHT  0x0010
#define KEY_LEFT   0x0020
#define KEY_UP     0x0040
#define KEY_DOWN   0x0080
#define KEY_R      0x0100
#define KEY_L      0x0200

#define RGB(r, g, b) ((u16)(((r) >> 3) | (((g) >> 3) << 5) | (((b) >> 3) << 10)))
#define EWRAM_BSS __attribute__((section(".ewram")))
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

// ---------- 색 ----------
#define C_WHITE   RGB(255, 255, 255)
#define C_SHADOW  RGB(10, 10, 30)
#define C_NAME    RGB(255, 230, 128)
#define C_BORDER  RGB(232, 232, 240)
#define C_DIM     RGB(150, 170, 200)
#define C_GRAY    RGB(110, 110, 130)
#define C_GREEN   RGB(120, 230, 140)
#define C_RED     RGB(255, 120, 120)
#define C_NAVY    RGB(0, 0, 128)

// ---------- 화면 (gfx.c) ----------
extern u16 bb[SCREEN_W * SCREEN_H];      // 장면 뒤판 (EWRAM)
void vsync(void);
void gfx_present(void);                  // 뒤판 전체를 화면으로
void gfx_restore(int y0, int y1);        // 뒤판의 일부 줄을 화면으로
void gfx_restore_rect(int x0, int y0, int x1, int y1);
void pset(u16 *dst, int x, int y, u16 c);
__attribute__((long_call)) void fill(u16 *dst, int x, int y, int w, int h, u16 c);
__attribute__((long_call)) void blend_dark(u16 *dst, int x, int y, int w, int h);
void draw_box(int x0, int y0, int x1, int y1);
void draw_title_box(int x0, int y0, int x1, int y1, const char *title);
void draw_img(u16 *dst, int x, int y, int w, int h, const u16 *img);
void draw_face(int x, int y, int who, int expr);
void draw_icon(int x, int y, int type, int done);
int glyph_width(u32 cp);
u32 utf8_next(const char **p);
int text_w(const char *s);
int draw_text(u16 *dst, int x, int y, const char *s, u16 color);
// 상자 안에 줄바꿈하며 그린다. 다 못 그리면 남은 위치를 돌려준다(쪽 넘김용)
const char *draw_wrapped(u16 *dst, int x, int y, int w, int max_lines, const char *s, u16 color);
int count_lines(int w, const char *s);
void draw_bar(int x, int y, int w, int h, int val, int max, u16 color);
void digits3x5(u16 *dst, int x, int y, int n, u16 c);
void fade(int dir);                      // 1: 어둡게, -1: 밝게

// ---------- 글자 다듬기 (text.c) ----------
const char *josa(const char *word, const char *pair);   // "지은" + "이/가" → "지은이"
const char *fmt(const char *f, ...);                     // %s %d %w(원) %m(MB) — 돌려쓰는 버퍼
const char *given_name(const char *full);                // 성을 뺀 이름

// ---------- 입력 ----------
extern u16 keys_now, keys_prev;
void poll_keys(void);
int pressed(u16 k);
int held(u16 k);
extern u32 frame_count;

// ---------- 소리 (sound.c) ----------
enum { BGM_NONE, BGM_TITLE, BGM_ROOM, BGM_HER, BGM_ENDING };
enum { SFX_CLICK, SFX_BLIP, SFX_NOTICE, SFX_ERROR, SFX_COIN, SFX_DONE, SFX_ITEM, SFX_STARTUP, SFX_OFF, SFX_THUMP, SFX_ALARM, SFX_TRAY, SFX_BURN, SFX_HDD, SFX_SCANNER, SFX_MODEM, SFX_MOVE };
void sound_init(void);
void sound_update(void);
void bgm(int id);
void sfx(int id);
void sound_toggle(void);
extern int sound_on;

// ---------- 인물 ----------
enum { W_NARR = -1, W_HER, W_MOM, W_DAD, W_FRIEND, W_HYUNG, W_OWNER };
enum { X_NORMAL, X_SMILE, X_BLUSH, X_SAD, X_SURPRISE, X_ANGRY };

// ---------- 화면 구성 요소 (ui.c) ----------
typedef struct { const char *label, *note; int disabled; } Opt;
void say(int who, int expr, const char *text);
void narr(const char *text);
int choose(int who, int expr, const char *prompt, const Opt *opts, int n);
void notice(const char *title, const char *const *lines, int n);
void progress(const char *title, const char *label, int frames);
void defrag_anim(int frag);
// 작품 고르기: 고른 번호(WORKS 색인)를 out에, 개수를 돌려준다. 취소하면 -1
typedef const char *(*InfoFn)(int w);
int pick_works(const char *title, const u8 *list, int n, int max, InfoFn info, const char *hint, int can_cancel, const char *confirm, int *out);
int keyboard(const char *title, char *out, int max_syllables, const char *initial);
void set_last_line(const char *s);

// ---------- 게임 상태와 규칙 (game.c) ----------
typedef struct { u16 hdd, cd, book, phys, part; } Entry;
enum { A_DOWNLOAD, A_BURN, A_SCAN, A_PC, A_YONGSAN, A_CLUB, A_FRIEND, A_CHORES, A_STUDY, A_REST, NACTS };
enum { F_OPENED = 1, F_SECRET = 2, F_KEPT = 4, F_RESUME = 8, F_TRICK = 16, F_WCHOME = 32 };
enum { GRAD_NONE, GRAD_CONFESS, GRAD_LETTER, GRAD_SILENT };
enum { SCR_DESKTOP, SCR_OFF };

typedef struct {
  u32 magic;
  char name[24], school[40], her[24];
  u8 taste, same_class, net, screen, grad;
  s16 turn, start_done, ban_turn;
  s32 money, hdd_cap, last_allowance;
  s16 blank, allowance_mod, month_study, phone_nights;
  s16 frag, line, drive;
  u8 drive_broken, burner, scanner, usb, fast_burner, hdd20, hdd40;
  s16 mom, friend, love;
  s32 m_download, m_scan, m_trade, m_share;     // 수집 방식 점수 x10
  s16 st_downloads, st_modem, st_burns, st_discs, st_defrags, st_formats, st_club, st_gifts, st_lost;
  s16 acts[NACTS];
  u16 flags;
  u32 rng;
  Entry col[NWORKS];
  u8 seen[NWORKS];
} State;

typedef struct {
  s32 genre[NGENRES];            // x10
  s32 total, phys, top_score;    // x10
  int top;                       // -1: 없음
  int top_share;                 // 1위 장르 비율 x1000
  int completed, owned_count;
  s32 method[4];                 // 아카이버, 손품장인, 나눔꾼, 완결주의자 (x10)
  int method_key;                // -1 없음
  int breadth;                   // 0 보통, 1 잡식가, 2 한 우물
  int media_pct[4];              // 1위 장르 작품 중 매체별 수집률 x1000 (게임, 영상, 책, 음악)
  int media_top;
} Traits;

typedef struct { u32 magic; u32 endings, romance; u8 seen[(NWORKS + 7) / 8]; u16 plays; } Meta;

extern State S;
extern Meta META;
#define TOTAL_TURNS 36
#define CD_MB 700
#define MANGA_PRICE 3500
#define NOVEL_PRICE 6000
#define BUNDLE_PRICE 15000

u32 rnd(u32 n);                  // 0..n-1
int chance(int permille);        // 천분율 확률
int clamp100(int v);
void date_of(int turn, int *y, int *m);
int grade_of(int turn);
int season_of(int turn);
int owned(int w);
int remaining(int w);
int complete(int w);
s32 hdd_used(void);
s32 hdd_free(void);
int available_works(u8 *out);    // 지금 구할 수 있는 작품 목록
int owned_works(u8 *out);
void add_units(int w, int n, int where, int method);
enum { WH_HDD, WH_CD, WH_BOOK };
enum { M_NONE, M_DOWNLOAD, M_SCAN, M_TRADE, M_BUY, M_BOOK };
typedef struct { int w, got, part; } DlRes;
int do_download(const int *ids, int n, s32 budget, DlRes *res, int *full, s32 *left);
int do_burn(int max_discs, int *used_mb, int *moved_works, int *moved_units);
int do_format(void);
int virus_damage(int *units);
int receive_disc(int w, int method);
void compute_traits(Traits *t);
const char *trait_label(const Traits *t);
int decide_ending(void);
int decide_romance(void);
int cd_price(void);
int allowance(void);
int taste_likes(int w);
void new_game(const char *name, const char *school);
int save_exists(void);
void save_game(void);
int load_game(void);
void save_clear(void);
void meta_load(void);
void meta_save(void);
const char *mbstr(s32 mb);
const char *wonstr(s32 n);
extern const char *const MEDIA_NAMES[4];
extern const char *const METHOD_NAMES[4];

// ---------- 장면 (scene.c) ----------
void scene_room(void);
void scene_future(void);
void scene_title(void);
void scene_status(void);

// ---------- 이야기 (story.c) ----------
void story_prologue(void);
void story_month_start(void);
void story_month_end(void);
void story_graduation(void);
void story_ending(void);
int give_from_collection(const char *title, const char *prompt, int *liked, int *usb);
void love_add(int n);

// ---------- 진행 (play.c) ----------
void run_activity(int act, int slot);
void month_upkeep(void);
void hub_loop(void);
const char *act_why(int act);
extern const char *const ACT_NAMES[NACTS];
