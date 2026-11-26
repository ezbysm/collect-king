// 수집왕 NDS — ARM7이 읽은 입력(X·Y 버튼, 터치)을 ARM9에 넘기는 자리
// 메인 RAM 끝 근처를 캐시 없는 거울 주소로 함께 본다.
#pragma once
typedef struct {
  volatile unsigned int frame;        // ARM7이 갱신할 때마다 1씩
  volatile unsigned short xy;         // bit0 X, bit1 Y (눌림 = 1)
  volatile unsigned short pen;        // 1 = 터치 중
  volatile unsigned short tx, ty;     // 화면 좌표 (0..255, 0..191)
  volatile unsigned short rawx, rawy; // 터치 패널 원시값 (확인용)
} Shared;
#define SHARED ((Shared *)0x027FD000)
