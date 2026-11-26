@ 수집왕 NDS — ARM9 시작 코드
@ 보호 장치(MPU)로 메모리 구역을 나누고 캐시와 TCM을 켠 뒤, ITCM 코드를 옮기고 bss를 비우고 main으로 간다.
@ 메인 RAM 0x02000000~0x023FFFFF는 캐시를 쓰고, 그 거울 주소(0x02400000~)는 캐시 없이 ARM7과 주고받는 데 쓴다.
  .section .crt0, "ax"
  .arm
  .global _start
_start:
  msr cpsr_c, #0xD3              @ 슈퍼바이저, 인터럽트 끔
  mov r0, #0x04000000
  str r0, [r0, #0x208]           @ IME = 0 (하위 비트 0)

  @ 보호 장치·TCM·캐시를 끄고 캐시를 비운다
  ldr r1, =0x00002078
  mcr p15, 0, r1, c1, c0, 0
  mov r0, #0
  mcr p15, 0, r0, c7, c5, 0      @ 명령 캐시 무효화
  mcr p15, 0, r0, c7, c6, 0      @ 데이터 캐시 무효화
  mcr p15, 0, r0, c7, c10, 4     @ 쓰기 버퍼 비우기

  @ TCM 위치: DTCM 0x0B000000 (16KB), ITCM 0 (32MB 단위로 되풀이)
  ldr r0, =0x0B00000A
  mcr p15, 0, r0, c9, c1, 0
  mov r0, #0x20
  mcr p15, 0, r0, c9, c1, 1

  @ 구역 (번호가 클수록 우선). 값 = 시작 주소 | 크기 코드<<1 | 켜기
  ldr r0, =(0x04000000 | (25 << 1) | 1)   @ 0: 입출력·VRAM 64MB
  mcr p15, 0, r0, c6, c0, 0
  ldr r0, =(0xFFFF0000 | (15 << 1) | 1)   @ 1: BIOS 64KB
  mcr p15, 0, r0, c6, c1, 0
  ldr r0, =(0x02000000 | (23 << 1) | 1)   @ 2: 메인 RAM 전체(거울 포함) 16MB, 캐시 없음
  mcr p15, 0, r0, c6, c2, 0
  mov r0, #0                              @ 3: 안 씀
  mcr p15, 0, r0, c6, c3, 0
  ldr r0, =(0x0B000000 | (13 << 1) | 1)   @ 4: DTCM 16KB
  mcr p15, 0, r0, c6, c4, 0
  ldr r0, =(0x01FF8000 | (14 << 1) | 1)   @ 5: ITCM 32KB
  mcr p15, 0, r0, c6, c5, 0
  mov r0, #0                              @ 6: 안 씀
  mcr p15, 0, r0, c6, c6, 0
  ldr r0, =(0x02000000 | (21 << 1) | 1)   @ 7: 메인 RAM 4MB, 캐시
  mcr p15, 0, r0, c6, c7, 0

  mov r0, #0x80                  @ 캐시·쓰기 버퍼: 구역 7만
  mcr p15, 0, r0, c3, c0, 0      @ 쓰기 버퍼
  mcr p15, 0, r0, c2, c0, 0      @ 데이터 캐시
  mcr p15, 0, r0, c2, c0, 1      @ 명령 캐시
  ldr r0, =0x33333333            @ 모든 구역 읽고 쓰기
  mcr p15, 0, r0, c5, c0, 2
  mcr p15, 0, r0, c5, c0, 3

  @ 켜기: 보호 장치(0), 데이터 캐시(2), 명령 캐시(12), DTCM(16), ITCM(18)
  mrc p15, 0, r0, c1, c0, 0
  ldr r1, =((1 << 0) | (1 << 2) | (1 << 12) | (1 << 16) | (1 << 18))
  orr r0, r0, r1
  mcr p15, 0, r0, c1, c0, 0

  @ 스택 (DTCM 안)
  msr cpsr_c, #0xD2              @ IRQ
  ldr sp, =0x0B003F80
  msr cpsr_c, #0xD3              @ 슈퍼바이저
  ldr sp, =0x0B003FC0
  msr cpsr_c, #0xDF              @ 시스템
  ldr sp, =0x0B003E00

  @ ITCM 코드 옮기기
  ldr r0, =__itcm_lma
  ldr r1, =__itcm_start
  ldr r2, =__itcm_end
1:cmp r1, r2
  ldrlo r3, [r0], #4
  strlo r3, [r1], #4
  blo 1b
  @ bss 비우기
  ldr r1, =__bss_start
  ldr r2, =__bss_end
  mov r3, #0
2:cmp r1, r2
  strlo r3, [r1], #4
  blo 2b

  ldr r0, =main
  mov lr, pc
  bx r0
3:b 3b
  .pool
