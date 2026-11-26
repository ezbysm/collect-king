# 수집왕 NDS — 빌드
#   make            → build/collect-king.nds
#   make assets     → 웹판 그림으로 src/arm9/art_data.c 다시 만들기 (저장소 루트에서 정적 서버 :8765 필요)
#   make test       → libretro DeSmuME로 화면 없이 시험 → build/test-*.png
CC = arm-none-eabi-gcc
OBJCOPY = arm-none-eabi-objcopy
ROM = build/collect-king.nds
FONT = ../gba/src
CFLAGS9 = -mcpu=arm946e-s -mthumb -mthumb-interwork -O2 -Wall -ffreestanding -std=gnu11 -ffunction-sections -fdata-sections -I$(FONT) -Isrc/arm9
CFLAGS7 = -mcpu=arm7tdmi -mthumb -mthumb-interwork -O2 -Wall -ffreestanding -std=gnu11
SRC9 = src/arm9/main.c src/arm9/gfx.c src/arm9/art_data.c $(FONT)/font_data.c
SRC7 = src/arm7/main.c

all: $(ROM)

build/arm9.elf: src/arm9/crt0.s $(SRC9) src/arm9/nds.h src/shared.h arm9.ld | build
	$(CC) $(CFLAGS9) -nostartfiles -T arm9.ld src/arm9/crt0.s $(SRC9) -o $@ -Wl,--gc-sections -lgcc

build/arm7.elf: src/arm7/crt0.s $(SRC7) src/shared.h arm7.ld | build
	$(CC) $(CFLAGS7) -nostartfiles -T arm7.ld src/arm7/crt0.s $(SRC7) -o $@ -Wl,--gc-sections -lgcc

build/%.bin: build/%.elf
	$(OBJCOPY) -O binary $< $@

build/banner.bin: tools/make-banner.py ../assets/icons/icon-36.png | build
	python3 tools/make-banner.py ../assets/icons/icon-36.png $@

$(ROM): build/arm9.bin build/arm7.bin build/banner.bin tools/mkrom.js
	node tools/mkrom.js $@ build/arm9.bin build/arm7.bin build/banner.bin

build:
	mkdir -p build

assets:
	cd .. && node nds/tools/make-art.js

build/retro: tools/retro.c | build
	gcc -O2 $< -o $@ -ldl -lpng

test: $(ROM) build/retro
	./build/retro $(ROM) < tools/test-script.txt

clean:
	rm -rf build

.PHONY: all assets test clean
