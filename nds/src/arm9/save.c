// 수집왕 NDS — 저장: 게임 카드의 저장 칩(EEPROM)에 SPI로 읽고 쓴다
// 2바이트 주소, 32바이트 쪽 단위로 쓰기 (8KB·64KB EEPROM 모두 맞는 방식)
// 에뮬레이터는 이 명령을 보고 저장 칩을 흉내 낸다. 칩이 없으면 읽기는 0xFF, 쓰기는 무시된다.
#include "common.h"

#define REG_AUXSPICNT  (*(volatile u16 *)0x040001A0)
#define REG_AUXSPIDATA (*(volatile u8 *)0x040001A2)
#define SPI_HOLD 0xA040   // 카드 슬롯 켜기 | 저장 칩(SPI) 모드 | 칩 선택 유지
#define SPI_LAST 0xA000   // 이 바이트를 보내고 칩 선택을 푼다 (명령 끝)

static void spi_wait(void) { while (REG_AUXSPICNT & 0x80) {} }
// 한 바이트 주고받기. last면 명령의 마지막 바이트
static u8 spi(u8 b, int last) {
  REG_AUXSPICNT = last ? SPI_LAST : SPI_HOLD;
  REG_AUXSPIDATA = b;
  spi_wait();
  return REG_AUXSPIDATA;
}
static void spi_done(void) { spi_wait(); REG_AUXSPICNT = 0; }

static void wait_ready(void) {
  // 상태 레지스터의 '쓰는 중' 비트가 꺼질 때까지 (칩이 없으면 0xFF라 횟수 제한)
  for (int i = 0; i < 20000; i++) {
    spi(0x05, 0); u8 st = spi(0, 1); spi_done();
    if (!(st & 1)) return;
  }
}

void save_mem_read(int at, void *dst, int n) {
  u8 *p = dst;
  if (n <= 0) return;
  spi(0x03, 0); spi((u8)(at >> 8), 0); spi((u8)at, 0);
  for (int i = 0; i < n; i++) p[i] = spi(0, i == n - 1);
  spi_done();
}

void save_mem_write(int at, const void *src, int n) {
  const u8 *p = src;
  while (n > 0) {
    int chunk = 32 - (at & 31);
    if (chunk > n) chunk = n;
    spi(0x06, 1); spi_done();                    // 쓰기 허가
    spi(0x02, 0); spi((u8)(at >> 8), 0); spi((u8)at, 0);
    for (int i = 0; i < chunk; i++) spi(p[i], i == chunk - 1);
    spi_done();
    wait_ready();
    at += chunk; p += chunk; n -= chunk;
  }
}
