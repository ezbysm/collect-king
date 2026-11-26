// 롬을 libmgba로 화면 없이 끝까지 자동 플레이한다.
// 버튼을 규칙적으로 누르며 게임 상태(S.turn)를 메모리에서 읽어 진행을 확인하고, 중간중간 화면을 PNG로 저장한다.
// 사용법: ./build/autoplay build/collect-king.gba build <S 주소(16진)> [seed]
#include <mgba/core/core.h>
#include <mgba/core/log.h>
#include <mgba/internal/arm/arm.h>
#include <stdarg.h>
#include <png.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define W 240
#define H 160
enum { K_A = 1, K_B = 2, K_SELECT = 4, K_START = 8, K_RIGHT = 16, K_LEFT = 32, K_UP = 64, K_DOWN = 128, K_R = 256, K_L = 512 };
static color_t buf[W * H];
static void quiet(struct mLogger *l, int c, enum mLogLevel lv, const char *f, va_list a) { (void)l; (void)c; (void)lv; (void)f; (void)a; }
static struct mLogger silent = { quiet, 0 };
static struct mCore *core;
static unsigned long frames;

static void save_png(const char *path) {
  FILE *f = fopen(path, "wb");
  png_structp png = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  png_infop info = png_create_info_struct(png);
  png_init_io(png, f);
  png_set_IHDR(png, info, W, H, 8, PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
  png_write_info(png, info);
  unsigned char row[W * 3];
  for (int y = 0; y < H; y++) {
    for (int x = 0; x < W; x++) { uint32_t c = buf[y * W + x]; row[x * 3] = c & 0xff; row[x * 3 + 1] = (c >> 8) & 0xff; row[x * 3 + 2] = (c >> 16) & 0xff; }
    png_write_row(png, row);
  }
  png_write_end(png, NULL);
  png_destroy_write_struct(&png, &info);
  fclose(f);
}
static const char *seq_dir; static int seq_every;
static void save_png(const char *path);
static void run(int n, uint32_t keys) {
  core->setKeys(core, keys);
  for (int i = 0; i < n; i++) {
    core->runFrame(core); frames++;
    if (seq_dir && frames % seq_every == 0) { char p[512]; snprintf(p, sizeof p, "%s/%06lu.png", seq_dir, frames); save_png(p); }
  }
}
static void tap(uint32_t k) { run(4, k); run(4, 0); }

int main(int argc, char **argv) {
  const char *rom = argv[1], *dir = argv[2];
  uint32_t S_addr = (uint32_t)strtoul(argv[3], 0, 16);
  unsigned seed = argc > 4 ? atoi(argv[4]) : 1;
  if (argc > 6) { seq_dir = argv[5]; seq_every = atoi(argv[6]); }
  srand(seed);
  mLogSetDefaultLogger(&silent);
  core = mCoreFind(rom);
  core->init(core);
  mCoreInitConfig(core, NULL);
  core->setVideoBuffer(core, buf, W);
  mCoreLoadFile(core, rom);
  core->reset(core);
  // State 안의 turn 위치: magic(4) + name(24) + school(40) + her(24) + 5바이트 → 정렬 후 오프셋 98
  const uint32_t TURN_OFF = 98;
  char path[512];
  int last_turn = -99, shots = 0;
  unsigned long last_change = 0;
  run(60, 0);
  snprintf(path, sizeof path, "%s/auto-00-title.png", dir); save_png(path);
  // 새로 시작 → 이름 (ㄱ+ㅏ = 가) → 확인 → 학교 기본
  tap(K_A); run(20, 0);
  snprintf(path, sizeof path, "%s/auto-01-keyboard.png", dir); save_png(path);
  tap(K_A); tap(K_DOWN); tap(K_DOWN); tap(K_A); tap(K_START); run(20, 0);
  tap(K_A); run(30, 0);
  for (unsigned long step = 0; step < 400000; step++) {
    int turn = (int16_t)core->busRead16(core, S_addr + TURN_OFF);
    if (turn != last_turn) {
      printf("turn %d at frame %lu\n", turn, frames); fflush(stdout);
      last_turn = turn; last_change = frames;
      if (turn >= 0 && turn % 3 == 0 && shots < 14) { snprintf(path, sizeof path, "%s/auto-%02d-turn%d.png", dir, 10 + shots++, turn); save_png(path); }
    }
    if (frames - last_change > 60 * 60 * 6) { snprintf(path, sizeof path, "%s/auto-stuck.png", dir); save_png(path); printf("STUCK at turn %d\n", turn); const uint32_t ks[4] = { K_DOWN, K_A, K_B, K_START }; for (int i = 0; i < 4; i++) { tap(ks[i]); run(30, 0); snprintf(path, sizeof path, "%s/stuck-%d.png", dir, i); save_png(path); } return 1; }
    // 버튼 규칙: 대부분 A, 가끔 START(일정 확정), 가끔 방향키
    int r = rand() % 100;
    if (r < 66) tap(K_A);
    else if (r < 76) tap(K_START);
    else if (r < 84) tap(K_DOWN);
    else if (r < 89) tap(K_RIGHT);
    else if (r < 91) tap(K_R);
    else if (r < 97) tap(K_B);
    else run(3, 0);
    // 엔딩을 보고 타이틀로 돌아와 새 판이 시작되면(turn 0) 끝
    static int max_turn;
    if (turn > max_turn) max_turn = turn;
    if (max_turn >= 36 && turn == 0) break;
    if (turn >= 36) {
      static int shotted;
      if (!shotted && frames - last_change > 60 * 30) { snprintf(path, sizeof path, "%s/auto-90-ending.png", dir); save_png(path); shotted = 1; }
      if (frames - last_change > 60 * 200) break;
    }
  }
  snprintf(path, sizeof path, "%s/auto-99-final.png", dir); save_png(path);
  printf("done: turn %d, frames %lu (%.1f분 분량)\n", last_turn, frames, frames / 3600.0);
  return 0;
}
