// 수집왕 NDS — ARM7 소리: GBA판(gba/src/sound.c)과 같은 악보·효과음을 DS 음원으로 연주한다
//  채널 8(사각파 PSG): 멜로디 / 채널 0(삼각파 PCM 반복): 베이스 / 채널 9(사각파): 효과음 / 채널 14(잡음): 효과음
//  GBA 음원의 자동 감쇠(엔벨로프)는 프레임마다 소리를 줄여 흉내 낸다.
#include "../shared.h"
typedef unsigned char u8;
typedef signed char s8;
typedef unsigned short u16;
typedef unsigned int u32;

#define REG_POWCNT2   (*(volatile u16 *)0x04000304)
#define REG_SOUNDCNT  (*(volatile u16 *)0x04000500)
#define REG_SOUNDBIAS (*(volatile u16 *)0x04000504)
#define SCHAN_CNT(n)  (*(volatile u32 *)(0x04000400 + (n) * 16))
#define SCHAN_VOL(n)  (*(volatile u8 *)(0x04000400 + (n) * 16))
#define SCHAN_SAD(n)  (*(volatile u32 *)(0x04000404 + (n) * 16))
#define SCHAN_TMR(n)  (*(volatile u16 *)(0x04000408 + (n) * 16))
#define SCHAN_PNT(n)  (*(volatile u16 *)(0x0400040A + (n) * 16))
#define SCHAN_LEN(n)  (*(volatile u32 *)(0x0400040C + (n) * 16))
#define START   (1u << 31)
#define PSG     (3u << 29)
#define PCM8    (0u << 29)
#define LOOP    (1u << 27)
#define PAN_C   (64u << 16)
#define CH_LEAD 8
#define CH_SFX  9
#define CH_BASS 0
#define CH_NOISE 14

enum { BGM_NONE, BGM_TITLE, BGM_ROOM, BGM_HER, BGM_ENDING };
enum { SFX_CLICK, SFX_BLIP, SFX_NOTICE, SFX_ERROR, SFX_COIN, SFX_DONE, SFX_ITEM, SFX_STARTUP, SFX_OFF, SFX_THUMP, SFX_ALARM, SFX_TRAY, SFX_BURN, SFX_HDD, SFX_SCANNER, SFX_MODEM, SFX_MOVE };

// ---------- 악보 (GBA판과 같다) ----------
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
  static const int NOTE[7] = { 9, 11, 0, 2, 4, 5, 7 };
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
static int arp(const char *roots, const s8 *pat, u8 *out) {
  u8 r[24]; int n = parse(roots, r, 24), k = 0;
  for (int i = 0; i < n; i++) for (int j = 0; j < 8; j++) out[k++] = (u8)(r[i] + pat[j]);
  return k;
}
static const s8 PAT_A[8] = { 0, 7, 12, 7, 0, 7, 12, 7 };
static const s8 PAT_B[8] = { 0, 7, 12, 16, 12, 7, 12, 7 };

// MIDI 번호 → 주파수(Hz x16)
static u32 freq16(int midi) {
  static const u16 BASE[12] = { 4186, 4435, 4699, 4978, 5274, 5588, 5920, 6272, 6645, 7040, 7459, 7902 };
  int oct = midi / 12 - 1, note = midi % 12;
  return ((u32)BASE[note] << 4) >> (8 - oct);
}
// 채널 타이머: 출력 = 16756991 / (-TMR) 샘플/초. 사각파는 8샘플이 한 주기, 삼각파는 32샘플
static u16 tmr_psg(int midi) { return (u16)(0x10000 - 33513984u / freq16(midi)); }
static u16 tmr_tri(int midi) { return (u16)(0x10000 - 8378496u / freq16(midi)); }

static s8 TRI[32] __attribute__((aligned(4)));

static int cur_bgm = BGM_NONE, bpm, step, tick_acc, lead_vol;
static int lead_v, lead_env, lead_env_t;      // 멜로디 소리 크기(0..127)와 줄어드는 빠르기
static int on = 1;

static void stop(int ch) { SCHAN_CNT(ch) = 0; }

void sound7_init(void) {
  REG_POWCNT2 |= 1;                 // 스피커 켜기
  REG_SOUNDCNT = 0x8000 | 127;      // 소리 켜기, 전체 크기 최대
  REG_SOUNDBIAS = 0x200;
  for (int i = 0; i < 16; i++) stop(i);
  for (int i = 0; i < 32; i++) TRI[i] = (s8)(i < 16 ? -120 + i * 16 : 120 - (i - 16) * 16);
}

static void start_bgm(int id) {
  cur_bgm = id; step = 0; tick_acc = 0;
  stop(CH_LEAD); stop(CH_BASS);
  if (id == BGM_ROOM || id == BGM_TITLE) { lead_len = parse(ROOM_LEAD, lead, 160); bass_len = arp(ROOM_BASS, PAT_A, bass); bpm = id == BGM_TITLE ? 100 : 88; lead_vol = 9; }
  else if (id == BGM_HER) { lead_len = parse(HER_LEAD, lead, 160); bass_len = arp(HER_BASS, PAT_B, bass); bpm = 76; lead_vol = 10; }
  else if (id == BGM_ENDING) { lead_len = parse(END_LEAD, lead, 160); bass_len = arp(END_BASS, PAT_A, bass); bpm = 68; lead_vol = 10; }
  else lead_len = 0;
}
static int hold_len(int i) { int n = 1; while (lead[(i + n) % lead_len] == HOLD && n < 16) n++; return n; }

static void music_step(void) {
  int i = step % lead_len;
  u8 ln = lead[i];
  if (ln != HOLD && ln != REST) {
    int len = hold_len(i);
    lead_env = len >= 4 ? 7 : len >= 2 ? 4 : 2;      // GBA판 엔벨로프 단계
    lead_v = lead_vol * 6; lead_env_t = 0;
    stop(CH_LEAD);
    SCHAN_TMR(CH_LEAD) = tmr_psg(ln);
    SCHAN_CNT(CH_LEAD) = START | PSG | (1u << 24) | PAN_C | lead_v;     // 듀티 25%
  } else if (ln == REST) { stop(CH_LEAD); lead_v = 0; }
  u8 bn = bass[i % bass_len];
  stop(CH_BASS);
  SCHAN_SAD(CH_BASS) = (u32)TRI;
  SCHAN_TMR(CH_BASS) = tmr_tri(bn);
  SCHAN_PNT(CH_BASS) = 0;
  SCHAN_LEN(CH_BASS) = 8;
  SCHAN_CNT(CH_BASS) = START | PCM8 | LOOP | PAN_C | 40;
  step++;
}

// ---------- 효과음 ----------
typedef struct { u8 midi, frames, noise; } Blip;   // midi 0 = 쉼
static Blip seq[48];
static int sfx_len, sfx_pos, sfx_frame, noise_v;
static u32 fc;

static int mk(int i, int midi, int frames, int noise_div) { seq[i].midi = (u8)midi; seq[i].frames = (u8)frames; seq[i].noise = (u8)noise_div; return i + 1; }

static void sfx_start(int id) {
  int n = 0;
  switch (id) {
    case SFX_CLICK: n = mk(n, 96, 2, 0); break;
    case SFX_MOVE: n = mk(n, 88, 1, 0); break;
    case SFX_BLIP: n = mk(n, 79 + (fc % 3), 1, 0); break;
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
  sfx_len = n; sfx_pos = 0; sfx_frame = 0;
}
// GBA 잡음 설정값(div) → 잡음 샘플 빠르기(Hz)
static u32 noise_hz(int div) { int r = div & 7, s = div >> 4; return r ? 524288u / r >> (s + 1) : 1048576u >> (s + 1); }

static void sfx_update(void) {
  if (noise_v > 0) { noise_v -= 12; if (noise_v <= 0) stop(CH_NOISE); else SCHAN_VOL(CH_NOISE) = (u8)noise_v; }
  if (sfx_pos >= sfx_len) return;
  const Blip *b = &seq[sfx_pos];
  if (sfx_frame == 0) {
    if (b->noise) {
      stop(CH_NOISE);
      u32 hz = noise_hz(b->noise); if (hz < 300) hz = 300;
      SCHAN_TMR(CH_NOISE) = (u16)(0x10000 - 16756991u / hz);
      noise_v = 70;
      SCHAN_CNT(CH_NOISE) = START | PSG | PAN_C | noise_v;
    }
    stop(CH_SFX);
    if (b->midi) { SCHAN_TMR(CH_SFX) = tmr_psg(b->midi); SCHAN_CNT(CH_SFX) = START | PSG | (3u << 24) | PAN_C | 60; }
  }
  if (++sfx_frame >= b->frames) { sfx_frame = 0; sfx_pos++; if (sfx_pos >= sfx_len) stop(CH_SFX); }
}

// 화면 갱신마다 한 번
void sound7_frame(void) {
  Shared *sh = SHARED;
  static u8 last_seq;
  fc++;
  if (!sh->sound_on) {
    if (on) { for (int i = 0; i < 16; i++) stop(i); on = 0; cur_bgm = -1; sfx_len = 0; }
    last_seq = sh->sfx_seq;
    return;
  }
  on = 1;
  if (sh->bgm != cur_bgm) start_bgm(sh->bgm);
  if (sh->sfx_seq != last_seq) { last_seq = sh->sfx_seq; sfx_start(sh->sfx_id); }
  if (cur_bgm != BGM_NONE && lead_len) {
    tick_acc += bpm;
    if (tick_acc >= 1800) { tick_acc -= 1800; music_step(); }
    // 멜로디 감쇠: GBA 엔벨로프 한 단계 = 약 lead_env 프레임마다 1/15씩
    if (lead_v > 0 && ++lead_env_t >= lead_env) { lead_env_t = 0; lead_v -= 6; if (lead_v < 0) lead_v = 0; SCHAN_VOL(CH_LEAD) = (u8)lead_v; }
  }
  sfx_update();
}
