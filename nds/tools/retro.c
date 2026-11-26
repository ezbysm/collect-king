// libretro DeSmuME 코어로 .nds 롬을 화면 없이 돌리는 시험 도구.
// 표준 입력으로 명령을 받는다:
//   wait N            N프레임 진행
//   press KEYS N      버튼(A B X Y L R START SELECT UP DOWN LEFT RIGHT, +로 묶음)을 N프레임 누르고 뗀다
//   touch X Y N       아래 화면 (X,Y)를 N프레임 누르고 뗀다
//   drag X1 Y1 X2 Y2 N 누른 채로 끌기 / pen X Y 누른 채로 두기 / hold KEYS 누른 채로 두기 / release 모두 떼기
//   shot FILE.png     지금 화면(위·아래 256x384)을 저장
//   peek ADDR         (지원 시) 메모리 값을 찍는다
// 사용법: ./build/retro rom.nds < script.txt
#include <dlfcn.h>
#include <png.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct retro_game_info { const char *path; const void *data; size_t size; const char *meta; };
struct retro_game_geometry { unsigned base_width, base_height, max_width, max_height; float aspect_ratio; };
struct retro_system_timing { double fps, sample_rate; };
struct retro_system_av_info { struct retro_game_geometry geometry; struct retro_system_timing timing; };
struct retro_variable { const char *key; const char *value; };

static int fmt = 0;              // 0: 0RGB1555, 1: XRGB8888, 2: RGB565
static uint8_t rgb[256 * 384 * 3];
static unsigned vw, vh;
static uint32_t joy;             // 누른 버튼 (libretro 번호)
static int ptr_x, ptr_y, ptr_down;
static int verbose = 0;

static bool env(unsigned cmd, void *data) {
  switch (cmd) {
    case 10: fmt = *(int *)data; return true;                              // 픽셀 형식
    case 9: case 31: *(const char **)data = "/tmp"; return true;           // 시스템·저장 폴더
    case 15: {                                                             // 코어 설정
      struct retro_variable *v = data;
      if (verbose) fprintf(stderr, "var? %s\n", v->key);
      if (strstr(v->key, "pointer_type")) { v->value = "touch"; return true; }
      if (strstr(v->key, "pointer_device") && !strstr(v->key, "_deadzone") && !strstr(v->key, "_accel")) { v->value = "none"; return true; }
      if (strstr(v->key, "pointer_mouse")) { v->value = "enable"; return true; }
      return false;
    }
    case 16: {                                                             // 설정 목록 (확인용)
      if (verbose) for (struct retro_variable *v = data; v->key; v++) fprintf(stderr, "opt %s = %s\n", v->key, v->value);
      return true;
    }
    case 17: *(bool *)data = false; return true;
    case 3: *(bool *)data = true; return true;
    default: return false;
  }
}
static void video(const void *d, unsigned w, unsigned h, size_t pitch) {
  if (!d) return;
  vw = w; vh = h > 384 ? 384 : h;
  for (unsigned y = 0; y < vh; y++)
    for (unsigned x = 0; x < w && x < 256; x++) {
      uint8_t *o = rgb + (y * 256 + x) * 3;
      if (fmt == 1) { uint32_t c = ((const uint32_t *)((const uint8_t *)d + y * pitch))[x]; o[0] = c >> 16; o[1] = c >> 8; o[2] = c; }
      else {
        uint16_t c = ((const uint16_t *)((const uint8_t *)d + y * pitch))[x];
        if (fmt == 2) { o[0] = (c >> 11) << 3; o[1] = ((c >> 5) & 63) << 2; o[2] = (c & 31) << 3; }
        else { o[0] = ((c >> 10) & 31) << 3; o[1] = ((c >> 5) & 31) << 3; o[2] = (c & 31) << 3; }
      }
    }
}
static void audio(int16_t l, int16_t r) { (void)l; (void)r; }
static size_t audio_batch(const int16_t *d, size_t n) { (void)d; return n; }
static void poll(void) {}
static int16_t input(unsigned port, unsigned dev, unsigned idx, unsigned id) {
  if (port) return 0;
  if (dev == 1) { if (id == 256) return (int16_t)joy; return (joy >> id) & 1; }   // 패드 (256 = 비트마스크)
  if (dev == 6 && idx == 0) {                                                       // 포인터(터치)
    if (id == 0) return ptr_x;
    if (id == 1) return ptr_y;
    if (id == 2) return ptr_down;
  }
  return 0;
}

static void save_png(const char *path) {
  FILE *f = fopen(path, "wb");
  if (!f) { perror(path); return; }
  png_structp png = png_create_write_struct(PNG_LIBPNG_VER_STRING, 0, 0, 0);
  png_infop info = png_create_info_struct(png);
  png_init_io(png, f);
  png_set_IHDR(png, info, 256, 384, 8, PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
  png_write_info(png, info);
  for (int y = 0; y < 384; y++) png_write_row(png, rgb + y * 256 * 3);
  png_write_end(png, 0);
  png_destroy_write_struct(&png, &info);
  fclose(f);
}

static void (*run)(void);
static void frames(int n) { for (int i = 0; i < n; i++) run(); }
static uint32_t keymask(const char *s) {
  static const char *names[] = { "B", "Y", "SELECT", "START", "UP", "DOWN", "LEFT", "RIGHT", "A", "X", "L", "R" };
  uint32_t m = 0; char buf[128]; strncpy(buf, s, 127); buf[127] = 0;
  for (char *t = strtok(buf, "+"); t; t = strtok(0, "+"))
    for (int i = 0; i < 12; i++) if (!strcmp(t, names[i])) m |= 1u << i;
  return m;
}

int main(int argc, char **argv) {
  const char *core = getenv("CORE") ? getenv("CORE") : "/usr/lib/x86_64-linux-gnu/libretro/desmume_libretro.so";
  verbose = getenv("VERBOSE") != 0;
  void *h = dlopen(core, RTLD_NOW);
  if (!h) { fprintf(stderr, "%s\n", dlerror()); return 1; }
#define SYM(t, n) __typeof__(t) *n = (__typeof__(t) *)dlsym(h, #n)
  SYM(void(bool (*)(unsigned, void *)), retro_set_environment);
  SYM(void(void (*)(const void *, unsigned, unsigned, size_t)), retro_set_video_refresh);
  SYM(void(void (*)(int16_t, int16_t)), retro_set_audio_sample);
  SYM(void(size_t (*)(const int16_t *, size_t)), retro_set_audio_sample_batch);
  SYM(void(void (*)(void)), retro_set_input_poll);
  SYM(void(int16_t (*)(unsigned, unsigned, unsigned, unsigned)), retro_set_input_state);
  SYM(void(void), retro_init);
  SYM(bool(const struct retro_game_info *), retro_load_game);
    run = (void (*)(void))dlsym(h, "retro_run");
  SYM(void *(unsigned), retro_get_memory_data);
  SYM(size_t(unsigned), retro_get_memory_size);
  retro_set_environment(env);
  retro_init();
  retro_set_video_refresh(video);
  retro_set_audio_sample(audio);
  retro_set_audio_sample_batch(audio_batch);
  retro_set_input_poll(poll);
  retro_set_input_state(input);
  FILE *f = fopen(argv[1], "rb");
  if (!f) { perror(argv[1]); return 1; }
  fseek(f, 0, SEEK_END); long n = ftell(f); fseek(f, 0, SEEK_SET);
  void *rom = malloc(n); if (fread(rom, 1, n, f) != (size_t)n) return 1; fclose(f);
  struct retro_game_info gi = { argv[1], rom, (size_t)n, 0 };
  if (!retro_load_game(&gi)) { fprintf(stderr, "load failed\n"); return 1; }
  char line[512];
  while (fgets(line, sizeof line, stdin)) {
    char cmd[32], a[256]; int x, y, k;
    if (sscanf(line, "%31s", cmd) != 1 || cmd[0] == '#') continue;
    if (!strcmp(cmd, "wait") && sscanf(line, "%*s %d", &k) == 1) frames(k);
    else if (!strcmp(cmd, "press") && sscanf(line, "%*s %255s %d", a, &k) == 2) { joy = keymask(a); frames(k); joy = 0; frames(2); }
    else if (!strcmp(cmd, "hold") && sscanf(line, "%*s %255s", a) == 1) joy = keymask(a);
    else if (!strcmp(cmd, "touch") && sscanf(line, "%*s %d %d %d", &x, &y, &k) == 3) {
      // 출력 화면(256x384) 전체가 -0x7fff..0x7fff. 아래 화면은 y+192
      ptr_x = (int)((x + 0.5) * 65536 / vw) - 32768;
      ptr_y = (int)((y + 192 + 0.5) * 65536 / vh) - 32768;
      ptr_down = 1; frames(k); ptr_down = 0; frames(2);
    }
    else if (!strcmp(cmd, "drag")) {
      // drag X1 Y1 X2 Y2 N: N프레임 동안 누른 채로 끌기
      int x2, y2;
      if (sscanf(line, "%*s %d %d %d %d %d", &x, &y, &x2, &y2, &k) == 5) {
        for (int i = 0; i <= k; i++) {
          int cx = x + (x2 - x) * i / (k ? k : 1), cy = y + (y2 - y) * i / (k ? k : 1);
          ptr_x = (int)((cx + 0.5) * 65536 / vw) - 32768; ptr_y = (int)((cy + 192 + 0.5) * 65536 / vh) - 32768;
          ptr_down = 1; frames(1);
        }
      }
    }
    else if (!strcmp(cmd, "pen") && sscanf(line, "%*s %d %d", &x, &y) == 2) {   // 누른 채로 두기 (release로 뗀다)
      ptr_x = (int)((x + 0.5) * 65536 / vw) - 32768; ptr_y = (int)((y + 192 + 0.5) * 65536 / vh) - 32768; ptr_down = 1;
    }
    else if (!strcmp(cmd, "release")) { ptr_down = 0; joy = 0; frames(2); }
    else if (!strcmp(cmd, "shot") && sscanf(line, "%*s %255s", a) == 1) save_png(a);
    else if (!strcmp(cmd, "peek") && sscanf(line, "%*s %x", &x) == 1) {
      // 메인 RAM(4MB) 안의 주소 (0x02xxxxxx)의 32비트 값
      uint8_t *ram = retro_get_memory_data(2); size_t sz = retro_get_memory_size(2);
      unsigned off = (unsigned)x & 0x3FFFFF;
      if (ram && off + 16 <= sz) { printf("%08x:", x); for (int i = 0; i < 16; i += 2) printf(" %04x", ram[off + i] | ram[off + i + 1] << 8); printf("\n"); }
      else printf("peek: no memory (%p, %zu)\n", (void *)ram, sz);
    }
    else if (!strcmp(cmd, "size")) printf("video %ux%u fmt %d\n", vw, vh, fmt);
  }
  return 0;
}
