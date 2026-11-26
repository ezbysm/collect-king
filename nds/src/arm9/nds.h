// 수집왕 NDS — ARM9 공통 정의: 하드웨어, 화면, 입력, 글자
#pragma once
#include "art_data.h"
#include "font.h"
#include "../shared.h"

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef signed short s16;
typedef int s32;

// ---------- 하드웨어 ----------
#define REG_DISPCNT     (*(volatile u32 *)0x04000000)
#define REG_DISPCNT_SUB (*(volatile u32 *)0x04001000)
#define REG_VCOUNT      (*(volatile u16 *)0x04000006)
#define REG_KEYINPUT    (*(volatile u16 *)0x04000130)
#define REG_POWCNT1     (*(volatile u32 *)0x04000304)
#define REG_EXMEMCNT    (*(volatile u16 *)0x04000204)
#define VRAMCNT_A       (*(volatile u8 *)0x04000240)
#define VRAMCNT_C       (*(volatile u8 *)0x04000242)

#define SCREEN_W 256
#define SCREEN_H 192
#define VRAM_TOP ((u16 *)0x06000000)
#define VRAM_BOT ((u16 *)0x06200000)

// 버튼 (KEYINPUT 10비트 + ARM7이 넘겨준 X·Y)
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
#define KEY_X      0x0400
#define KEY_Y      0x0800
#define KEY_TOUCH  0x1000   // 화면을 누른 순간

// 색: NDS 비트맵은 맨 윗비트(0x8000)가 1이어야 보인다
#define RGB(r, g, b) ((u16)(0x8000 | ((r) >> 3) | (((g) >> 3) << 5) | (((b) >> 3) << 10)))
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define ITCM __attribute__((section(".itcm"), target("arm"), noinline, long_call))

#define C_WHITE  RGB(255, 255, 255)
#define C_BLACK  RGB(0, 0, 0)
#define C_SHADOW RGB(10, 10, 30)
#define C_NAME   RGB(255, 230, 128)
#define C_BORDER RGB(232, 232, 240)
#define C_DIM    RGB(150, 170, 200)
#define C_GRAY   RGB(110, 110, 130)
#define C_GREEN  RGB(120, 230, 140)
#define C_RED    RGB(255, 120, 120)
// 윈도우 98 색 (아래 화면)
#define W_FACE   RGB(192, 192, 192)
#define W_LIGHT  RGB(255, 255, 255)
#define W_SHADE  RGB(128, 128, 128)
#define W_DARK   RGB(0, 0, 0)
#define W_DESK   RGB(0, 128, 128)
#define W_NAVY   RGB(0, 0, 128)
#define W_TEXT   RGB(0, 0, 0)
#define W_GRAYTX RGB(128, 128, 128)

// ---------- 화면 (gfx.c) ----------
// 화면마다 VRAM(보이는 곳)과 뒤판(배경을 기억해 두는 곳)이 있다
typedef struct { u16 *vram; u16 *bb; } Screen;
extern Screen TOP, BOT;
void gfx_init(void);
void vsync(void);
extern u32 frame_count;
void present(Screen *s);                                   // 뒤판 전체를 화면으로
void restore(Screen *s, int x, int y, int w, int h);       // 뒤판의 일부를 화면으로
void copy_image(u16 *dst, const u16 *src);                 // 256x192 그림 한 장
ITCM void fill(u16 *dst, int x, int y, int w, int h, u16 c);
ITCM void blend_dark(Screen *s, int x, int y, int w, int h);
void draw_img(u16 *dst, int x, int y, int w, int h, const u16 *img);
void draw_face(u16 *dst, int x, int y, int who, int expr);
void dark_box(Screen *s, int x0, int y0, int x1, int y1);          // 반투명 남색 창 (위 화면)
void dark_title_box(Screen *s, int x0, int y0, int x1, int y1, const char *title);
void win_frame(u16 *dst, int x, int y, int w, int h, int sunken); // 윈도우 98 테두리
void win_window(u16 *dst, int x, int y, int w, int h, const char *title);
void win_button(u16 *dst, int x, int y, int w, int h, const char *label, int state); // 0 보통, 1 고름, 2 눌림, 3 못 씀
void desktop(Screen *s, const char *clock);                       // 아래 화면 바탕화면 + 작업 표시줄

// ---------- 글자 ----------
u32 utf8_next(const char **p);
int glyph_width(u32 cp);
int text_w(const char *s);
int draw_text(u16 *dst, int x, int y, const char *s, u16 color);          // 그림자 있음
int draw_text_flat(u16 *dst, int x, int y, const char *s, u16 color);     // 그림자 없음
const char *draw_wrapped(u16 *dst, int x, int y, int w, int max_lines, const char *s, u16 color, int shadow);
int count_lines(int w, const char *s);
#define LINE_H 15
void put_utf8(char *o, int *at, u32 cp);
const char *fmt(const char *f, ...);
const char *josa(const char *word, const char *pair);

// ---------- 입력 ----------
void poll_input(void);
int pressed(u16 k);       // 이번 프레임에 새로 눌림
int held(u16 k);
extern int touch_x, touch_y, touch_down;   // 지금 누르는 자리
extern int tap_x, tap_y;                   // 마지막으로 누른 순간의 자리
extern u16 raw_tx, raw_ty;
int tapped_in(int x, int y, int w, int h); // 이번 프레임에 이 칸을 눌렀나
