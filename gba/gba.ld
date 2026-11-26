/* 수집왕 GBA 링커 스크립트: 코드·데이터는 롬(카트리지), 변수는 IWRAM */
OUTPUT_ARCH(arm)
ENTRY(_start)

MEMORY {
  rom   (rx)  : ORIGIN = 0x08000000, LENGTH = 32M
  iwram (rwx) : ORIGIN = 0x03000000, LENGTH = 31K
  ewram (rwx) : ORIGIN = 0x02000000, LENGTH = 256K
}

SECTIONS {
  .text : {
    KEEP(*(.crt0))
    *(.text .text.*)
    *(.rodata .rodata.*)
    . = ALIGN(4);
  } > rom

  .ARM.exidx : { *(.ARM.exidx*) } > rom

  __data_lma = LOADADDR(.data);
  .data : {
    __data_start = .;
    *(.iwram .iwram.*)
    *(.data .data.*)
    . = ALIGN(4);
    __data_end = .;
  } > iwram AT > rom

  .bss (NOLOAD) : {
    __bss_start = .;
    *(.bss .bss.*)
    *(COMMON)
    . = ALIGN(4);
    __bss_end = .;
  } > iwram

  .ewram (NOLOAD) : { *(.ewram .ewram.*) } > ewram

  /DISCARD/ : { *(.comment) *(.note*) }
}
