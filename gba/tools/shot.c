// 롬을 libmgba로 화면 없이 실행하고, 정해진 버튼 입력을 넣으며 화면을 PNG로 저장한다.
// 사용법: ./build/shot build/collect-king.gba build
#include <mgba/core/core.h>
#include <png.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define W 240
#define H 160
enum { K_A = 1 << 0, K_B = 1 << 1, K_UP = 1 << 6, K_DOWN = 1 << 7 };

static color_t buf[W * H];

static void save_png(const char *path) {
  FILE *f = fopen(path, "wb");
  png_structp png = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  png_infop info = png_create_info_struct(png);
  png_init_io(png, f);
  png_set_IHDR(png, info, W, H, 8, PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
  png_write_info(png, info);
  unsigned char row[W * 3];
  for (int y = 0; y < H; y++) {
    for (int x = 0; x < W; x++) {
      uint32_t c = buf[y * W + x];   // mGBA 32비트 색: 0xXXBBGGRR
      row[x * 3] = c & 0xff; row[x * 3 + 1] = (c >> 8) & 0xff; row[x * 3 + 2] = (c >> 16) & 0xff;
    }
    png_write_row(png, row);
  }
  png_write_end(png, NULL);
  png_destroy_write_struct(&png, &info);
  fclose(f);
  printf("saved %s\n", path);
}

static struct mCore *core;
static void run(int frames, uint32_t keys) {
  core->setKeys(core, keys);
  for (int i = 0; i < frames; i++) core->runFrame(core);
}
static void tap(uint32_t k) { run(3, k); run(6, 0); }

int main(int argc, char **argv) {
  const char *rom = argc > 1 ? argv[1] : "build/collect-king.gba";
  const char *dir = argc > 2 ? argv[2] : "build";
  core = mCoreFind(rom);
  if (!core || !core->init(core)) { fprintf(stderr, "core init failed\n"); return 1; }
  mCoreInitConfig(core, NULL);
  core->setVideoBuffer(core, buf, W);
  if (!mCoreLoadFile(core, rom)) { fprintf(stderr, "load failed\n"); return 1; }
  core->reset(core);
  char path[512];
#define SHOT(name) snprintf(path, sizeof path, "%s/shot-%s.png", dir, name), save_png(path)

  run(90, 0); SHOT("1-first-line");          // 첫 줄이 타자 치듯 나오는 중
  run(60, 0); tap(K_A);                        // 다 나온 뒤 A → 둘째 줄
  tap(K_A); SHOT("2-skip");                    // A로 둘째 줄을 한 번에
  tap(K_A); run(120, 0); tap(K_A); run(120, 0); tap(K_A);
  run(80, 0); SHOT("3-her");                   // 그 아이 초상화 + 대사
  tap(K_A);
  run(30, 0); tap(K_DOWN); run(10, 0); SHOT("4-choice");   // 선택지에서 아래로 한 칸
  tap(K_A); run(60, 0); SHOT("5-reply");       // 고른 답에 대한 대답
  core->deinit(core);
  return 0;
}
