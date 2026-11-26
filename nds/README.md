# 수집왕 NDS

웹판·GBA판 수집왕을 닌텐도 DS 롬(.nds)으로 옮긴 판입니다. 처음부터 끝까지(이름 짓기 → 중학교 3년 36개월 → 졸업 → 엔딩) GBA판과 **같은 규칙·같은 이야기**로 플레이할 수 있습니다.

- 롬: [`collect-king.nds`](collect-king.nds) (약 1.1MB)
- 에뮬레이터 캡처: [`screenshots/`](screenshots/)

## 두 화면 구성
- **위 화면**: 계절마다 바뀌는 방(CD 랙·책·포스터·사진·달력이 진행에 따라 늘어남)과 상태 막대, 대사창(초상화, 한 글자씩), 작품 자세히 보기, 일정 설명
- **아래 화면**: 윈도우 98 바탕화면. 메인 화면은 바탕화면 아이콘(이번 달 계획·내 컬렉션·내 컴퓨터·메뉴), 그 위에 선택지·알림창·다운로드 진행 창·디스크 조각 모음·작품 목록·한글 키보드가 창으로 뜸
- 대사 중에는 아래 화면에 작은 '내 컴퓨터' 창(가진 돈·공CD·하드 용량)

## 조작
터치와 버튼 어느 쪽으로도 끝까지 할 수 있습니다.

| 입력 | 하는 일 |
|---|---|
| 아래 화면 터치 | 아이콘·단추·선택지·키 누르기, 대사 넘기기. 작품 목록은 한 번 눌러 고르고 다시 눌러 결정 |
| A | 결정 · 대사 넘기기 · 일정 칸에 넣기 |
| B | 취소 · 지우기 · 닫기 |
| 십자키 | 고르기 |
| L / R | 작품 목록 종류 · 일정 칸 바꾸기 |
| START | 일정 확정 · 작품 여러 개 고른 뒤 확정 · 이름 확정 |
| SELECT | 소리 켜고 끄기 |

## 해 보기
- **아이폰**: Delta / **안드로이드**: DraStic, melonDS / **PC**: melonDS, DeSmuME
- **실기**: DS 플래시 카트리지, DSi·3DS의 TWiLight Menu++. (실기는 확인하지 못했습니다. 롬 헤더의 닌텐도 로고 자리는 비워 두었습니다)

### 저장
- 달마다 자동 저장, 타이틀에서 '이어하기'. 본 엔딩·만난 작품 기록은 따로 남습니다.
- 게임 카드의 저장 칩(EEPROM)에 저장합니다. 에뮬레이터가 저장 칩을 붙여 주도록 롬을 상품판 카드와 같은 모양(ARM9 0x4000, 보안 영역 자리, 게임 코드 `ZSJW`)으로 만들었습니다.
- DeSmuME에서 저장·재시작 후 이어하기를 확인했습니다(8KB EEPROM으로 인식). melonDS(Delta)는 모르는 게임에 64KB EEPROM을 붙이므로 같은 방식으로 동작할 것으로 봅니다.
- 에뮬레이터·실행기에 따라 저장 칩이 없으면 '이어하기'가 꺼진 채로 남습니다. 그럴 때는 에뮬레이터의 상태 저장(세이브 스테이트)을 쓰세요.

### 호환성을 위해 한 것
- 음악은 ARM7이 화면 갱신마다 따로 연주 (ARM9가 화면을 오래 그려도 끊기지 않음)
- 인터럽트를 쓰지 않음. 입력은 ARM7이 읽어 캐시 없는 메인 RAM으로 넘김
- 모든 데이터가 ARM9 바이너리에 들어 있어 실행 중 카드를 읽지 않음
- 헤더·보안 영역·배너 CRC를 채움

## GBA판과 함께 쓰는 코드
게임 규칙·이야기·활동은 GBA판 소스를 그대로 컴파일합니다. 한쪽을 고치면 두 판이 같이 바뀝니다.

| 파일 (`../gba/src`) | 내용 |
|---|---|
| `game.h`, `common.h` | 공통 정의 (`-DNDS`이면 기종 부분은 `src/arm9/nds.h`) |
| `game.c` | 상태, 규칙, 성향·엔딩 계산, 저장 (NDS는 `save.c`의 EEPROM으로) |
| `activity.c` | 활동, 위험 이벤트, 월말 정리, 한 판의 흐름, 컬렉션, 기록, 새 게임 |
| `story.c` | 프롤로그, 달별 이벤트, 졸업, 엔딩 |
| `text.c`, `data_gen.c`, `font_data.c` | 문장 다듬기, 작품·엔딩 표, 한글 글꼴 |

NDS 쪽 파일:

| 파일 | 내용 |
|---|---|
| `src/arm9/crt0.s`, `arm9.ld` | 시작 코드: 보안 영역 자리, 보호 장치(MPU)·캐시·TCM |
| `src/arm9/gfx.c`, `nds.h` | 두 화면 출력(모드 5 비트맵), 윈도우 98 창·단추, 글자, 입력, 화면 어둡게 |
| `src/arm9/ui.c` | 대사, 선택지, 알림창, 진행 창, 조각 모음, 작품 목록, 한글 키보드 |
| `src/arm9/scene.c` | 위 화면 방 장면 |
| `src/arm9/play.c` | 메인 화면(바탕화면 아이콘), 일정 짜기, 내 컴퓨터, 도움말, 시작 |
| `src/arm9/sound.c`, `src/arm7/sound7.c` | 소리 요청 / ARM7 연주 (GBA판과 같은 악보·효과음) |
| `src/arm9/save.c` | 카드 저장 칩(EEPROM) 읽기·쓰기 |
| `src/arm7/main.c` | X·Y 버튼, 터치 패널, 소리 |
| `tools/mkrom.js`, `make-banner.py`, `make-art.js` | 롬 헤더, DS 메뉴 아이콘, 그림 변환 |
| `tools/retro.c` | libretro DeSmuME로 화면 없이 터치·버튼을 넣으며 시험 |

## 빌드
```bash
sudo apt install gcc-arm-none-eabi libnewlib-arm-none-eabi   # 컴파일러
sudo apt install libretro-desmume libpng-dev                  # (선택) 화면 없이 시험
cd nds
make                       # → build/collect-king.nds
make test                  # 새 게임 → 메인 화면 → 저장 → 이어하기 (tools/test-script.txt), 캡처 build/test-*.png
make autoplay SEED=3       # 무작위로 터치·버튼을 눌러 한 판을 끝까지 (시험용 롬으로)
```
그림(`src/arm9/art_data.c`)을 다시 만들려면 저장소 루트에서 `python3 -m http.server 8765`를 띄우고 `make assets`.

글꼴: Galmuri (SIL Open Font License 1.1, `../gba/tools/Galmuri-LICENSE.txt`)
