# 수집왕 GBA 빌드 (Ubuntu: apt install gcc-arm-none-eabi libnewlib-arm-none-eabi)
#   make          롬 빌드 → build/collect-king.gba
#   make assets   글꼴·배경 C 파일 다시 만들기 (배경은 저장소 루트에서 정적 서버가 떠 있어야 함)
#   make autoplay 에뮬레이터 코어(libmgba)로 끝까지 자동 플레이 → build/auto-*.png
CC      = arm-none-eabi-gcc
OBJCOPY = arm-none-eabi-objcopy
ARCH    = -mthumb -mthumb-interwork -mcpu=arm7tdmi
CFLAGS  = $(ARCH) -O2 -Wall -ffreestanding -std=gnu11 -ffunction-sections -fdata-sections
LDFLAGS = $(ARCH) -nostartfiles -T gba.ld -Wl,--gc-sections

SRC  = src/play.c src/story.c src/ui.c src/scene.c src/game.c src/gfx.c src/text.c src/sound.c src/data_gen.c src/font_data.c src/bg_data.c
OBJ  = $(SRC:src/%.c=build/%.o) build/crt0.o
ROM  = build/collect-king.gba

all: $(ROM)

build/%.o: src/%.c src/common.h src/data_gen.h src/bg_data.h src/font.h | build
	$(CC) $(CFLAGS) -c $< -o $@

build/crt0.o: src/crt0.s | build
	$(CC) -marm -mcpu=arm7tdmi -c $< -o $@

build/collect-king.elf: $(OBJ) gba.ld
	$(CC) $(LDFLAGS) $(OBJ) -lgcc -o $@

$(ROM): build/collect-king.elf
	$(OBJCOPY) -O binary $< $@
	node tools/fix-header.js $@

build:
	mkdir -p build

assets:
	node tools/make-data.js
	node tools/make-font.js
	node tools/make-bg.js

# 자동 플레이: 에뮬레이터 코어(libmgba)로 처음부터 엔딩까지 진행하며 화면 캡처 → build/auto-*.png
build/autoplay: tools/autoplay.c | build
	gcc -O2 $< -o $@ -lmgba -lpng

autoplay: $(ROM) build/autoplay
	./build/autoplay $(ROM) build 0x$(shell arm-none-eabi-nm build/collect-king.elf | awk '$$3=="S"{print $$1}') $(or $(SEED),1)

clean:
	rm -f build/*.o build/*.elf $(ROM) build/autoplay build/auto-*.png

.PHONY: all assets autoplay clean
