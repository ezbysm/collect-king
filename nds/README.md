# 수집왕 NDS (1단계 시험 롬)

웹판 수집왕을 닌텐도 DS 롬(.nds)으로 옮기는 작업입니다. 지금은 **1단계: 두 화면·한글·터치·버튼이 제대로 되는지 확인하는 시험 롬**입니다.

- 시험 롬: [`collect-king-test.nds`](collect-king-test.nds) (약 1MB)
- 에뮬레이터 캡처: [`screenshots/`](screenshots/)

## 시험 롬에서 되는 것
- **두 화면**: 위 화면에는 웹판 도트 그림(방, 타이틀)과 대사, 아래 화면에는 윈도우 98 느낌의 바탕화면 위에 메뉴·키보드·선택지
- **한글**: Galmuri11 도트 글꼴로 한글 11,172자 전체 (GBA판과 같은 글꼴 데이터)
- **터치**: 아래 화면의 단추, 한글 키보드 칸, 선택지를 손가락·펜으로 누르기. '입력 확인'의 그림판에서 선 긋기
- **버튼**: A·B·X·Y·L·R·START·SELECT·십자키 모두. 터치 대신 버튼만으로도 끝까지 진행 가능
- 시험 장면: 타이틀 → 이름 짓기(터치 한글 키보드) → 대사(초상화, 한 글자씩) → 선택지 → 대답 → 타이틀
- '입력 확인': 위 화면에 누르고 있는 버튼과 터치 좌표, 아래 화면에 그림판 (A로 색 바꾸기, '끝내기' 또는 START로 나가기)

아직 없는 것: 게임 진행, 소리, 저장 (2단계에서 GBA판 게임 전체를 옮기며 넣습니다)

## 조작
| 입력 | 하는 일 |
|---|---|
| 아래 화면 터치 | 단추·키·선택지 누르기, 대사 넘기기 |
| A | 결정 · 대사 넘기기 |
| B | 글자 지우기 · 대사 넘기기 |
| 십자키 | 고르기 |
| START | 이름 확정 |

## 해 보기
- **PC**: melonDS, DeSmuME 같은 에뮬레이터로 열기
- **안드로이드**: DraStic, melonDS 안드로이드판
- **아이폰**: Delta (DS 지원)
- **실기**: R4 같은 DS 플래시 카트리지, DSi·3DS의 TWiLight Menu++에서 `.nds`를 실행. 롬 헤더의 닌텐도 로고 자리는 비워 두었으며, 플래시 카트리지·TWiLight Menu++는 이를 확인하지 않습니다. (실기는 아직 확인하지 못했습니다)

## 빌드
Ubuntu 기준, GBA판과 같은 컴파일러를 씁니다 (devkitPro·libnds 없이 직접 만든 시작 코드):
```bash
sudo apt install gcc-arm-none-eabi libnewlib-arm-none-eabi   # 컴파일러
sudo apt install libretro-desmume libpng-dev                  # (선택) 화면 없이 시험
cd nds
make          # → build/collect-king.nds
make test     # libretro DeSmuME로 tools/test-script.txt대로 누르며 화면 캡처 → build/test-*.png
```
글꼴은 `../gba/src/font_data.c`를 함께 씁니다. 그림(`src/arm9/art_data.c`)을 다시 만들려면 저장소 루트에서 `python3 -m http.server 8765`를 띄우고 `make assets`.

## 구조
| 파일 | 내용 |
|---|---|
| `src/arm9/crt0.s`, `arm9.ld` | ARM9 시작 코드: 보호 장치(MPU)·캐시·TCM 설정 |
| `src/arm9/gfx.c`, `nds.h` | 두 화면 출력(모드 5 비트맵), 창·단추, 글자, 입력 |
| `src/arm9/main.c` | 1단계 시험 장면 (타이틀, 키보드, 대사, 선택지, 입력 확인) |
| `src/arm7/crt0.s`, `main.c`, `arm7.ld` | ARM7: X·Y 버튼과 터치 패널(SPI)을 읽어 ARM9에 넘김 |
| `src/shared.h` | 두 CPU가 함께 보는 입력 자리 (캐시 없는 메인 RAM) |
| `tools/make-art.js` | 웹판 그림 → 256×192 화면 그림, 초상화 |
| `tools/make-banner.py` | DS 메뉴에 보이는 아이콘(32×32, 16색)과 제목 |
| `tools/mkrom.js` | ARM9·ARM7·배너를 묶어 `.nds` 헤더와 CRC 채우기 |
| `tools/retro.c` | libretro DeSmuME 코어로 롬을 돌리며 터치·버튼 입력과 화면 캡처 |

## 다음 단계
2. GBA판 게임 전체를 옮기기: 위 화면은 방과 대사, 아래 화면은 일정·작품 목록·컬렉션을 터치로. 소리(ARM7 음원)와 저장

글꼴: Galmuri (SIL Open Font License 1.1, `../gba/tools/Galmuri-LICENSE.txt`)
