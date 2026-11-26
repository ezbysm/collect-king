// 수집왕 — 공통 정의. NDS판으로 빌드할 때는 nds/src/arm9/nds.h가 기종 부분을 맡는다
#pragma once
#include "game.h"
#ifdef NDS
#include "nds.h"
#else
#include "bg_data.h"
#include "font.h"

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
void copy_full(u16 *dst, const u16 *src);   // 화면 한 장 복사 (DMA)
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
int text_w(const char *s);
int draw_text(u16 *dst, int x, int y, const char *s, u16 color);
// 상자 안에 줄바꿈하며 그린다. 다 못 그리면 남은 위치를 돌려준다(쪽 넘김용)
const char *draw_wrapped(u16 *dst, int x, int y, int w, int max_lines, const char *s, u16 color);
int count_lines(int w, const char *s);
void draw_bar(int x, int y, int w, int h, int val, int max, u16 color);
void digits3x5(u16 *dst, int x, int y, int n, u16 c);

// ---------- 입력 ----------
extern u16 keys_now, keys_prev;
void poll_keys(void);
int pressed(u16 k);
int held(u16 k);

#endif
