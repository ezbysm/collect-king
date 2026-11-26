@ 수집왕 GBA — 시작 코드와 롬 헤더
@ 헤더 체크섬은 빌드 후 tools/fix-header.js가 채운다.
@ 닌텐도 로고 자리(156바이트)는 비워 둔다: 에뮬레이터는 그대로 실행되고,
@ 실기에서는 gbafix(devkitPro)로 로고를 넣어야 한다.

    .section .crt0, "ax"
    .arm
    .global _start
_start:
    b       start_vector
    .fill   156, 1, 0               @ 0x04 닌텐도 로고 (빈칸)
    .ascii  "COLLECTKING\0"         @ 0xA0 게임 제목 (12바이트)
    .ascii  "CKNG"                  @ 0xAC 게임 코드
    .ascii  "00"                    @ 0xB0 제작사 코드
    .byte   0x96                    @ 0xB2 고정값
    .byte   0x00                    @ 0xB3 기기 코드
    .byte   0x00                    @ 0xB4 장치 종류
    .fill   7, 1, 0                 @ 0xB5 예약
    .byte   0x00                    @ 0xBC 버전
    .byte   0x00                    @ 0xBD 헤더 체크섬
    .fill   2, 1, 0                 @ 0xBE 예약

start_vector:
    @ IRQ 모드 스택
    mov     r0, #0x12
    msr     cpsr_c, r0
    ldr     sp, =0x03007FA0
    @ 시스템 모드 스택 (main은 여기서 실행)
    mov     r0, #0x1F
    msr     cpsr_c, r0
    ldr     sp, =0x03007F00

    @ .data를 롬에서 IWRAM으로 복사
    ldr     r0, =__data_lma
    ldr     r1, =__data_start
    ldr     r2, =__data_end
1:  cmp     r1, r2
    ldrlt   r3, [r0], #4
    strlt   r3, [r1], #4
    blt     1b

    @ .bss를 0으로
    ldr     r1, =__bss_start
    ldr     r2, =__bss_end
    mov     r3, #0
2:  cmp     r1, r2
    strlt   r3, [r1], #4
    blt     2b

    ldr     r0, =main
    mov     lr, pc
    bx      r0
3:  b       3b

    .pool
