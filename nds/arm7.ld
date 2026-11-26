/* 수집왕 NDS — ARM7 메모리 배치: ARM7 전용 WRAM(64KB) */
OUTPUT_ARCH(arm)
ENTRY(_start)
MEMORY { wram : ORIGIN = 0x03800000, LENGTH = 0xE000 }
SECTIONS {
  .text : { KEEP(*(.crt0)) *(.text .text.*) *(.rodata .rodata.*) . = ALIGN(4); } > wram
  .data : { *(.data .data.*) . = ALIGN(4); } > wram
  .bss (NOLOAD) : { __bss_start = .; *(.bss .bss.*) *(COMMON) . = ALIGN(4); __bss_end = .; } > wram
  /DISCARD/ : { *(.ARM.exidx*) *(.ARM.attributes) *(.comment) }
}
