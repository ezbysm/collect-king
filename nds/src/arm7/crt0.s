@ 수집왕 NDS — ARM7 시작 코드: 스택을 잡고 bss를 비운 뒤 main으로
  .section .crt0, "ax"
  .arm
  .global _start
_start:
  msr cpsr_c, #0xD3
  mov r0, #0x04000000
  str r0, [r0, #0x208]           @ IME = 0
  msr cpsr_c, #0xD2
  ldr sp, =0x0380FD80
  msr cpsr_c, #0xD3
  ldr sp, =0x0380FDC0
  msr cpsr_c, #0xDF
  ldr sp, =0x0380FD00
  ldr r1, =__bss_start
  ldr r2, =__bss_end
  mov r3, #0
1:cmp r1, r2
  strlo r3, [r1], #4
  blo 1b
  ldr r0, =main
  mov lr, pc
  bx r0
2:b 2b
  .pool
