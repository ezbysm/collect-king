# 수집왕 GBA 빌드 (Ubuntu: apt install gcc-arm-none-eabi libnewlib-arm-none-eabi)
#   make          롬 빌드 → build/collect-king.gba
#   make assets   글꼴·배경 C 파일 다시 만들기 (배경은 저장소 루트에서 정적 서버가 떠 있어야 함)
#   make shot     에뮬레이터(libmgba)로 실행해 화면 캡처 → build/shot-*.png
CC      = arm-none-eabi-gcc
OBJCOPY = arm-none-eabi-objcopy
ARCH    = -mthumb -mthumb-interwork -mcpu=arm7tdmi
CFLAGS  = $(ARCH) -O2 -Wall -ffreestanding -fno-builtin -std=c11
LDFLAGS = $(ARCH) -nostartfiles -T gba.ld -Wl,--gc-sections

SRC  = src/main.c src/font_data.c src/bg_data.c
OBJ  = $(SRC:src/%.c=build/%.o) build/crt0.o
ROM  = build/collect-king.gba

all: $(ROM)

build/%.o: src/%.c src/font.h | build
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
	node tools/make-font.js
	node tools/make-bg.js

build/shot: tools/shot.c | build
	gcc -O2 $< -o $@ -lmgba -lpng

shot: $(ROM) build/shot
	./build/shot $(ROM) build

clean:
	rm -f build/*.o build/*.elf $(ROM) build/shot build/shot-*.png

.PHONY: all assets shot clean
