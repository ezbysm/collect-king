// 수집왕 NDS — ARM7: 화면 갱신마다 X·Y 버튼과 터치 패널을 읽어 ARM9에 넘긴다
#include "../shared.h"
typedef unsigned short u16;
typedef unsigned int u32;
#define REG_VCOUNT  (*(volatile u16 *)0x04000006)
#define REG_KEYXY   (*(volatile u16 *)0x04000136)
#define REG_SPICNT  (*(volatile u16 *)0x040001C0)
#define REG_SPIDATA (*(volatile u16 *)0x040001C2)
#define SPI_ENABLE 0x8000
#define SPI_HOLD   0x0800
#define SPI_TOUCH  0x0200
#define SPI_2MHZ   0x0001

static void spi_wait(void) { while (REG_SPICNT & 0x80) {} }
// 터치 패널(TSC2046)에서 12비트 값 하나 읽기
static int tsc_read(int cmd) {
  spi_wait();
  REG_SPICNT = SPI_ENABLE | SPI_2MHZ | SPI_TOUCH | SPI_HOLD;
  REG_SPIDATA = cmd;
  spi_wait();
  REG_SPIDATA = 0;
  spi_wait();
  int hi = REG_SPIDATA;
  REG_SPICNT = SPI_ENABLE | SPI_2MHZ | SPI_TOUCH;
  REG_SPIDATA = 0;
  spi_wait();
  int lo = REG_SPIDATA;
  return ((hi & 0x7F) << 5) | (lo >> 3);
}
// 여러 번 읽어 가운데 값을 쓴다 (튀는 값 거르기)
static int tsc_median(int cmd) {
  int v[5];
  for (int i = 0; i < 5; i++) v[i] = tsc_read(cmd);
  for (int i = 0; i < 5; i++) for (int j = i + 1; j < 5; j++) if (v[j] < v[i]) { int t = v[i]; v[i] = v[j]; v[j] = t; }
  return v[2];
}

// 본체 설정에 저장된 보정값 (펌웨어가 0x027FFC80에 복사해 둔다)
static int cal_ok, ax1, ay1, sx1, sy1, ax2, ay2, sx2, sy2;
static void load_calibration(void) {
  volatile unsigned char *p = (volatile unsigned char *)0x027FFC80;
  ax1 = p[0x58] | p[0x59] << 8; ay1 = p[0x5A] | p[0x5B] << 8; sx1 = p[0x5C]; sy1 = p[0x5D];
  ax2 = p[0x5E] | p[0x5F] << 8; ay2 = p[0x60] | p[0x61] << 8; sx2 = p[0x62]; sy2 = p[0x63];
  cal_ok = ax2 != ax1 && ay2 != ay1 && sx2 != sx1 && sy2 != sy1;
}
static int clampi(int v, int lo, int hi) { return v < lo ? lo : v > hi ? hi : v; }

int main(void) {
  load_calibration();
  Shared *sh = SHARED;
  sh->pen = 0; sh->xy = 0;
  for (;;) {
    while (REG_VCOUNT >= 192) {}
    while (REG_VCOUNT < 192) {}
    u16 k = REG_KEYXY;
    sh->xy = (u16)(~k & 3);
    int down = !(k & 0x40);
    if (down) {
      int rx = tsc_median(0xD4), ry = tsc_median(0x94);   // X: 채널 5, Y: 채널 1 (12비트, 차동)
      sh->rawx = rx; sh->rawy = ry;
      // 읽는 동안 손을 뗐으면 버린다
      if ((REG_KEYXY & 0x40) || rx == 0 || ry == 0 || rx >= 4095 || ry >= 4095) down = 0;
      else {
        int x, y;
        if (cal_ok) { x = sx1 + (rx - ax1) * (sx2 - sx1) / (ax2 - ax1) - 1; y = sy1 + (ry - ay1) * (sy2 - sy1) / (ay2 - ay1) - 1; }
        else { x = (rx - 256) * 256 / 3584; y = (ry - 256) * 192 / 3584; }
        sh->rawx = rx; sh->rawy = ry;
        sh->tx = clampi(x, 0, 255); sh->ty = clampi(y, 0, 191);
      }
    }
    sh->pen = down;
    sh->frame++;
  }
}
