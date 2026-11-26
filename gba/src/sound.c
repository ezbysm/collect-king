// 수집왕 GBA — 소리: GBA 음원 칩(PSG)으로 웹판과 같은 멜로디를 연주한다
//  채널1(사각파): 멜로디 / 채널3(파형 메모리, 삼각파): 베이스 / 채널2·4: 효과음
#include "common.h"

#define REG_SOUND1CNT_L (*(volatile u16 *)0x04000060)
#define REG_SOUND1CNT_H (*(volatile u16 *)0x04000062)
#define REG_SOUND1CNT_X (*(volatile u16 *)0x04000064)
#define REG_SOUND2CNT_L (*(volatile u16 *)0x04000068)
#define REG_SOUND2CNT_H (*(volatile u16 *)0x0400006C)
#define REG_SOUND3CNT_L (*(volatile u16 *)0x04000070)
#define REG_SOUND3CNT_H (*(volatile u16 *)0x04000072)
#define REG_SOUND3CNT_X (*(volatile u16 *)0x04000074)
#define REG_SOUND4CNT_L (*(volatile u16 *)0x04000078)
#define REG_SOUND4CNT_H (*(volatile u16 *)0x0400007C)
#define REG_SOUNDCNT_L  (*(volatile u16 *)0x04000080)
#define REG_SOUNDCNT_H  (*(volatile u16 *)0x04000082)
#define REG_SOUNDCNT_X  (*(volatile u16 *)0x04000084)
#define WAVE_RAM        ((volatile u32 *)0x04000090)

int sound_on = 1;

// ---------- 음표 ----------
// 웹판 js/sound.js와 같은 악보 (. = 이어서, - = 쉼표)
static const char ROOM_LEAD[] =
  "E5 . G5 . A5 G5 E5 .   C5 . . . A4 . C5 .   D5 . F5 . E5 D5 C5 .   D5 . . . - - G4 . "
  "E5 . G5 . C6 . B5 .    G5 . . . E5 . D5 .   C5 . D5 . E5 . F5 E5   D5 . . . . . - - "
  "C5 . E5 . G5 . E5 .    A4 . C5 . E5 . D5 .  F4 . A4 . C5 . A4 .    B4 . D5 . G5 . F5 . "
  "E5 . . . D5 . C5 .     B4 . . . G4 . B4 .   A4 . C5 . F5 . E5 .    D5 . . . C5 . . .";
static const char ROOM_BASS[] = "C3 A2 F2 G2 C3 E2 F2 G2 C3 A2 F2 G2 C3 E2 F2 G2";
static const char HER_LEAD[] =
  "A5 . . G5 F5 . C5 .    D5 . F5 . A5 . G5 .   F5 . . D5 Bb4 . D5 .   C5 . . . E5 . G5 . "
  "A5 . C6 . A5 . F5 .    E5 . . . C5 . E5 .    D5 . F5 . Bb5 . A5 .   G5 . . . . . - -";
static const char HER_BASS[] = "F2 D2 Bb2 C3 F2 A2 Bb2 C3";
static const char END_LEAD[] =
  "G5 . . . E5 . G5 .    D6 . . . B5 . G5 .    C6 . . . A5 . E5 .    B5 . . . G5 . . . "
  "A5 . . . F5 . A5 .    G5 . . . E5 . C5 .    F5 . A5 . C6 . A5 .   G5 . . . . . - -";
static const char END_BASS[] = "C3 G2 A2 E2 F2 C3 F2 G2";

#define HOLD 254
#define REST 0
static u8 lead[160], bass[160];
static int lead_len, bass_len;

static int parse(const char *s, u8 *out, int max) {
  int n = 0;
  static const int NOTE[7] = { 9, 11, 0, 2, 4, 5, 7 };  // A B C D E F G
  while (*s && n < max) {
    while (*s == ' ') s++;
    if (!*s) break;
    if (*s == '.') { out[n++] = HOLD; s++; continue; }
    if (*s == '-') { out[n++] = REST; s++; continue; }
    int note = NOTE[*s - 'A']; s++;
    if (*s == 'b') { note--; s++; } else if (*s == '#') { note++; s++; }
    int oct = *s++ - '0';
    out[n++] = (u8)((oct + 1) * 12 + note);
  }
  return n;
}
// 베이스: 마디마다 근음-5도-옥타브 아르페지오
static int arp(const char *roots, const s8 *pat, u8 *out) {
  u8 r[24]; int n = parse(roots, r, 24), k = 0;
  for (int i = 0; i < n; i++) for (int j = 0; j < 8; j++) out[k++] = (u8)(r[i] + pat[j]);
  return k;
}
static const s8 PAT_A[8] = { 0, 7, 12, 7, 0, 7, 12, 7 };
static const s8 PAT_B[8] = { 0, 7, 12, 16, 12, 7, 12, 7 };

// MIDI 번호 → 주파수(Hz x16) 표
static u32 freq16(int midi) {
  static const u16 BASE[12] = { 4186, 4435, 4699, 4978, 5274, 5588, 5920, 6272, 6645, 7040, 7459, 7902 }; // 8옥타브 C8..B8 (Hz)
  int oct = midi / 12 - 1, note = midi % 12;
  u32 f = (u32)BASE[note] << 4;           // x16
  return f >> (8 - oct);
}
static u16 sq_rate(int midi) { u32 f16 = freq16(midi); u32 v = 2048 - (131072u * 16) / f16; return (u16)(v & 2047); }
static u16 wave_rate(int midi) { u32 f16 = freq16(midi); u32 v = 2048 - (65536u * 16) / f16; return (u16)(v & 2047); }

static int cur_bgm = BGM_NONE, want_bgm = BGM_NONE;
static int bpm, step, tick_acc, lead_vol;

void sound_init(void) {
  REG_SOUNDCNT_X = 0x80;
  REG_SOUNDCNT_L = 0xFF77;
  REG_SOUNDCNT_H = 0x0002;
  // 파형 메모리: 삼각파
  REG_SOUND3CNT_L = 0x40;  // 뱅크 1을 쓰는 동안 뱅크 0에 쓴다
  static const u32 TRI[4] = { 0x67452301, 0xEFCDAB89, 0x98BADCFE, 0x10325476 };
  for (int i = 0; i < 4; i++) WAVE_RAM[i] = TRI[i];
  REG_SOUND3CNT_L = 0x00;
  REG_SOUND1CNT_L = 0x0008;  // 스윕 없음
}

void bgm(int id) { want_bgm = id; }

static void start_bgm(int id) {
  cur_bgm = id; step = 0; tick_acc = 0;
  if (id == BGM_ROOM || id == BGM_TITLE) { lead_len = parse(ROOM_LEAD, lead, 160); bass_len = arp(ROOM_BASS, PAT_A, bass); bpm = id == BGM_TITLE ? 100 : 88; lead_vol = 9; }
  else if (id == BGM_HER) { lead_len = parse(HER_LEAD, lead, 160); bass_len = arp(HER_BASS, PAT_B, bass); bpm = 76; lead_vol = 10; }
  else if (id == BGM_ENDING) { lead_len = parse(END_LEAD, lead, 160); bass_len = arp(END_BASS, PAT_A, bass); bpm = 68; lead_vol = 10; }
  else { REG_SOUND1CNT_H = 0; REG_SOUND1CNT_X = 0x8000; REG_SOUND3CNT_L = 0; }
}

static int hold_len(int i) { int n = 1; while (lead[(i + n) % lead_len] == HOLD && n < 16) n++; return n; }

static void music_step(void) {
  int i = step % lead_len;
  u8 ln = lead[i];
  if (ln != HOLD && ln != REST) {
    // 길이에 맞춰 천천히 줄어드는 소리
    int len = hold_len(i);
    int env_step = len >= 4 ? 7 : len >= 2 ? 4 : 2;
    REG_SOUND1CNT_H = (u16)((lead_vol << 12) | (env_step << 8) | (1 << 6));   // 듀티 25%
    REG_SOUND1CNT_X = (u16)(sq_rate(ln) | 0x8000);
  } else if (ln == REST) {
    REG_SOUND1CNT_H = 0; REG_SOUND1CNT_X = 0x8000;
  }
  u8 bn = bass[i % bass_len];
  REG_SOUND3CNT_L = 0x80;                     // 켜기
  REG_SOUND3CNT_H = 0x4000;                   // 볼륨 50%
  REG_SOUND3CNT_X = (u16)(wave_rate(bn) | 0x8000);
  step++;
}

// ---------- 효과음 ----------
typedef struct { u16 rate; u8 frames; u8 noise; } Blip;  // rate 0 = 쉼
static const Blip *sfx_seq;
static int sfx_len, sfx_pos, sfx_frame;

static Blip seq_buf[48];

static void sq2(u16 rate, int vol, int env) {
  REG_SOUND2CNT_L = (u16)((vol << 12) | (env << 8) | (2 << 6));
  REG_SOUND2CNT_H = (u16)(rate | 0x8000);
}
static void noise(int vol, int env, int div) {
  REG_SOUND4CNT_L = (u16)((vol << 12) | (env << 8));
  REG_SOUND4CNT_H = (u16)(div | 0x8000);
}

static int mk(int i, int midi, int frames, int noise_div) {
  seq_buf[i].rate = midi ? sq_rate(midi) : 0;
  seq_buf[i].frames = (u8)frames;
  seq_buf[i].noise = (u8)noise_div;
  return i + 1;
}

void sfx(int id) {
  if (!sound_on) return;
  int n = 0;
  switch (id) {
    case SFX_CLICK: n = mk(n, 96, 2, 0); break;
    case SFX_MOVE: n = mk(n, 88, 1, 0); break;
    case SFX_BLIP: n = mk(n, 79 + (frame_count % 3), 1, 0); break;
    case SFX_NOTICE: n = mk(n, 81, 6, 0); n = mk(n, 88, 10, 0); break;
    case SFX_ERROR: n = mk(n, 69, 8, 0); n = mk(n, 66, 10, 0); break;
    case SFX_COIN: n = mk(n, 83, 4, 0); n = mk(n, 88, 14, 0); break;
    case SFX_DONE: n = mk(n, 72, 4, 0); n = mk(n, 76, 4, 0); n = mk(n, 79, 4, 0); n = mk(n, 84, 10, 0); break;
    case SFX_ITEM: n = mk(n, 79, 3, 0); n = mk(n, 84, 3, 0); n = mk(n, 88, 3, 0); n = mk(n, 91, 12, 0); break;
    case SFX_STARTUP: n = mk(n, 56, 8, 0); n = mk(n, 63, 8, 0); n = mk(n, 68, 8, 0); n = mk(n, 72, 8, 0); n = mk(n, 75, 24, 0); break;
    case SFX_OFF: n = mk(n, 81, 3, 0); n = mk(n, 69, 3, 0); n = mk(n, 57, 3, 0); n = mk(n, 45, 6, 0); break;
    case SFX_THUMP: n = mk(n, 0, 6, 0x71); break;
    case SFX_ALARM: for (int i = 0; i < 4; i++) n = mk(n, i % 2 ? 76 : 81, 8, 0); break;
    case SFX_TRAY: n = mk(n, 0, 5, 0x52); n = mk(n, 50, 4, 0); break;
    case SFX_BURN: for (int i = 0; i < 12; i++) n = mk(n, 40 + i * 2, 4, 0); break;
    case SFX_HDD: for (int i = 0; i < 20; i++) n = mk(n, 0, 1 + (i * 7) % 5, i % 2 ? 0x22 : 0); break;
    case SFX_SCANNER: for (int i = 0; i < 10; i++) n = mk(n, 45 + (i % 2), 8, 0); break;
    case SFX_MODEM: {
      static const u8 dial[] = { 77, 84, 81, 88, 79, 86, 83 };
      for (int i = 0; i < 7; i++) { n = mk(n, dial[i], 5, 0); n = mk(n, 0, 2, 0); }
      n = mk(n, 96, 20, 0);
      for (int i = 0; i < 16; i++) n = mk(n, i % 3 ? 90 + (i * 5) % 9 : 0, 3, i % 3 ? 0 : 0x31);
      break;
    }
  }
  sfx_seq = seq_buf; sfx_len = n; sfx_pos = 0; sfx_frame = 0;
}

static void sfx_update(void) {
  if (!sfx_seq || sfx_pos >= sfx_len) return;
  const Blip *b = &sfx_seq[sfx_pos];
  if (sfx_frame == 0) {
    if (b->noise) noise(10, 1, b->noise);
    if (b->rate) sq2(b->rate, 11, 0); else REG_SOUND2CNT_L = 0, REG_SOUND2CNT_H = 0x8000;
  }
  if (++sfx_frame >= b->frames) { sfx_frame = 0; sfx_pos++; if (sfx_pos >= sfx_len) { REG_SOUND2CNT_L = 0; REG_SOUND2CNT_H = 0x8000; } }
}

void sound_update(void) {
  if (!sound_on) return;
  if (want_bgm != cur_bgm) start_bgm(want_bgm);
  if (cur_bgm != BGM_NONE && lead_len) {
    // 8분음표 하나 = 1800/bpm 프레임
    tick_acc += bpm;
    if (tick_acc >= 1800) { tick_acc -= 1800; music_step(); }
  }
  sfx_update();
}

void sound_toggle(void) {
  sound_on = !sound_on;
  if (!sound_on) { REG_SOUNDCNT_X = 0; cur_bgm = BGM_NONE; }
  else { sound_init(); cur_bgm = BGM_NONE; }
}
