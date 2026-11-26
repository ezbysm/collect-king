# 수집왕 NDS — 빌드
#   make            → build/collect-king.nds
#   make assets     → 웹판 그림으로 src/arm9/art_data.c 다시 만들기 (저장소 루트에서 정적 서버 :8765 필요)
#   make test       → libretro DeSmuME로 화면 없이 시험 → build/test-*.png
#   make autoplay SEED=n [EVERY=프레임]  → 무작위로 눌러 한 판을 끝까지 (EVERY마다 build/autoN/에 캡처)
CC = arm-none-eabi-gcc
OBJCOPY = arm-none-eabi-objcopy
ROM = build/collect-king.nds
SEED ?= 1
EVERY ?= 0
FONT = ../gba/src
CFLAGS9 = -mcpu=arm946e-s -mthumb -mthumb-interwork -O2 -Wall -ffreestanding -std=gnu11 -ffunction-sections -fdata-sections -DNDS -Isrc/arm9 -I$(FONT)
CFLAGS7 = -mcpu=arm7tdmi -mthumb -mthumb-interwork -O2 -Wall -ffreestanding -std=gnu11
GBA = ../gba/src
SRC9 = src/arm9/play.c src/arm9/ui.c src/arm9/scene.c src/arm9/gfx.c src/arm9/sound.c src/arm9/save.c src/arm9/art_data.c \
       $(GBA)/activity.c $(GBA)/story.c $(GBA)/game.c $(GBA)/text.c $(GBA)/data_gen.c $(GBA)/font_data.c
SRC7 = src/arm7/main.c src/arm7/sound7.c

all: $(ROM)

build/arm9.elf: src/arm9/crt0.s $(SRC9) src/arm9/nds.h src/shared.h $(GBA)/game.h $(GBA)/common.h arm9.ld | build
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

# 시험용 롬: 지금 달을 화면 구석 픽셀로 알려 준다
build/autotest.nds: $(SRC9) src/arm9/crt0.s src/arm9/nds.h arm9.ld build/arm7.bin build/banner.bin
	$(CC) $(CFLAGS9) -DAUTOTEST -nostartfiles -T arm9.ld src/arm9/crt0.s $(SRC9) -o build/autotest9.elf -Wl,--gc-sections -lgcc
	$(OBJCOPY) -O binary build/autotest9.elf build/autotest9.bin
	node tools/mkrom.js $@ build/autotest9.bin build/arm7.bin build/banner.bin

autoplay: build/autotest.nds build/retro
	mkdir -p build/auto$(SEED)
	echo "auto $(SEED) build/auto$(SEED) $(EVERY)" | ./build/retro build/autotest.nds

test: $(ROM) build/retro
	./build/retro $(ROM) < tools/test-script.txt

clean:
	rm -rf build

.PHONY: all assets test autoplay clean
