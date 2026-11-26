// 수집왕 NDS — 두 CPU가 함께 보는 자리 (메인 RAM 끝 근처, 캐시 없는 거울 주소)
//  ARM7 → ARM9: X·Y 버튼, 터치
//  ARM9 → ARM7: 배경음·효과음 요청, 소리 켜고 끄기
#pragma once
typedef struct {
  volatile unsigned int frame;        // ARM7이 갱신할 때마다 1씩
  volatile unsigned short xy;         // bit0 X, bit1 Y (눌림 = 1)
  volatile unsigned short pen;        // 1 = 터치 중
  volatile unsigned short tx, ty;     // 화면 좌표 (0..255, 0..191)
  volatile unsigned short rawx, rawy; // 터치 패널 원시값 (확인용)
  volatile unsigned char bgm;         // 틀 배경음 번호 (BGM_*)
  volatile unsigned char sound_on;    // 0이면 모두 끔
  volatile unsigned char sfx_id;      // 효과음 번호 (SFX_*)
  volatile unsigned char sfx_seq;     // 바뀔 때마다 효과음 하나 (같은 소리를 연달아 낼 수 있게)
} Shared;
#define SHARED ((Shared *)0x027FD000)
