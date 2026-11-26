/* 수집왕 NDS — ARM9 메모리 배치: 메인 RAM(4MB)에 통째로 올라가고, 빠른 코드는 ITCM, 스택은 DTCM */
OUTPUT_ARCH(arm)
ENTRY(_start)
MEMORY {
  ram  : ORIGIN = 0x02000000, LENGTH = 0x3F0000
  itcm : ORIGIN = 0x01FF8000, LENGTH = 32K
  dtcm : ORIGIN = 0x0B000000, LENGTH = 16K
}
SECTIONS {
  .text : { KEEP(*(.secure)) KEEP(*(.crt0)) *(.text .text.*) *(.rodata .rodata.*) . = ALIGN(4); } > ram
  .itcm : { __itcm_start = .; *(.itcm .itcm.*) . = ALIGN(4); __itcm_end = .; } > itcm AT > ram
  __itcm_lma = LOADADDR(.itcm);
  .data : { *(.data .data.*) . = ALIGN(4); } > ram
  .bss (NOLOAD) : { __bss_start = .; *(.bss .bss.*) *(COMMON) . = ALIGN(4); __bss_end = .; } > ram
  /DISCARD/ : { *(.ARM.exidx*) *(.ARM.attributes) *(.comment) }
}
