# 수집왕 GBA

웹판 수집왕을 게임보이 어드밴스(GBA) 롬으로 옮긴 판입니다. 처음부터 끝까지(이름 짓기 → 중학교 3년 36개월 → 졸업 → 엔딩) 웹판과 같은 규칙으로 플레이할 수 있습니다.

- 롬: [`collect-king.gba`](collect-king.gba) (약 1MB)
- 에뮬레이터 캡처: [`screenshots/`](screenshots/)

## 들어 있는 것
- **규칙**: 웹판 규칙 전체를 정수 계산으로 옮김. 활동 10가지, 시험·용돈·전화요금, 용산 가게, 서점, 동호회 교환, 위험 이벤트(엄마 종료, 인터넷 끊김, 바이러스, 드라이브 고장), 조각 모음·포맷
- **작품 113개**와 등장 시기, 장르 6가지 · 매체 4가지 성향 계산
- **이야기**: 프롤로그, 달마다 그 아이·친구·가족 이벤트, 졸업식, 진로 엔딩 32종 + 연애 에필로그 4종, 히로인 이름과 취향은 판마다 무작위
- **그림**: 웹판 도트(계절별 방 4가지, 미래의 방, 타이틀, 초상화 6명 × 표정 6, 종류 아이콘)를 240×160 화면으로 옮김. CD 랙, 책, 포스터, 사진, 달력은 진행에 따라 바뀜
- **글자**: 한글 11,172자 전체가 들어간 도트 글꼴(Galmuri11), 한글 조합 키보드로 이름과 학교 이름 입력
- **소리**: GBA 내장 음원으로 만든 배경음 4곡(타이틀, 방, 그 아이, 엔딩)과 효과음(모뎀 접속음, CD 굽기 등)
- **저장**: 카트리지 SRAM에 달마다 자동 저장, 타이틀에서 '이어하기'. 본 엔딩·작품 기록은 따로 남아 '기록 보기'에서 확인

웹판과 다른 점: 기기 위치로 학교 이름을 정하는 기능은 없고, 대신 학교 이름을 고르거나 직접 입력합니다.

## 조작
| 버튼 | 하는 일 |
|---|---|
| A | 다음 대사 · 결정 · 일정 칸에 넣기 |
| B | 취소 · 지우기 · 닫기 |
| 십자키 | 고르기 (작품 목록에서 좌우는 한 쪽씩 넘김) |
| L / R | 작품 목록 종류 바꾸기 · 일정 칸 바꾸기 |
| START | 일정 확정 · 작품 여러 개 고른 뒤 확정 · 이름 입력 확인 |
| SELECT | 소리 켜고 끄기 |

## 해 보기
- **아이폰·안드로이드**: GBA 에뮬레이터(Delta, Pizza Boy 등)에 `collect-king.gba`를 불러오기
- **PC**: mGBA 같은 에뮬레이터로 열기. 저장은 에뮬레이터가 `.sav` 파일로 남김
- **실기(플래시 카트리지)**: 롬 헤더의 닌텐도 로고 자리를 비워 두었습니다. devkitPro의 `gbafix collect-king.gba`로 로고를 넣어야 실기에서 켜집니다. 저장은 SRAM(32KB) 방식입니다.

### 호환성을 위해 한 것
- 인터럽트를 쓰지 않음. 입력은 화면 갱신마다, 그리고 오래 걸리는 그리기 중간에도 모아 두어 빨리 눌러도 놓치지 않음 (에뮬레이터의 BIOS 흉내와 상관없이 같게 동작)
- BIOS 함수를 부르지 않음
- 저장 방식이 자동으로 SRAM으로 잡히도록 롬에 `SRAM_V113` 표시
- 헤더 체크섬을 채움, 롬 크기를 2의 거듭제곱 단위로 채움
- 화면 모드 3(비트맵) 하나만 사용, 오래 걸리는 그리기는 IWRAM의 ARM 코드로

## 빌드
Ubuntu 기준:
```bash
sudo apt install gcc-arm-none-eabi libnewlib-arm-none-eabi   # 컴파일러
sudo apt install libmgba-dev libpng-dev                       # (선택) 자동 플레이 테스트용
cd gba
make                     # → build/collect-king.gba
make autoplay SEED=3     # libmgba로 화면 없이 한 판을 끝까지 자동 플레이, build/auto-*.png 캡처
```

생성 데이터(`src/data_gen.c`, `src/font_data.c`, `src/bg_data.c`)는 저장소에 들어 있습니다. 다시 만들려면:
```bash
# Galmuri11.bdf 받기
(cd tools && npm pack galmuri@2.40.3 && tar xzf galmuri-2.40.3.tgz package/dist/Galmuri11.bdf --strip-components=2)
# 저장소 루트에서 정적 서버를 띄운 뒤 (그림은 웹판 js/art.js로 그림)
python3 -m http.server 8765 &
cd gba && make assets
```
`tools/make-data.js`는 웹판 `js/data.js`와 같은 작품·장르·엔딩 표를 C로 옮깁니다. 웹판 데이터를 고친 뒤 `make assets`를 다시 돌리면 GBA판에도 반영됩니다.

## 구조
| 파일 | 내용 |
|---|---|
| `src/crt0.s`, `gba.ld` | 시작 코드, 롬 헤더, 메모리 배치 |
| `src/gfx.c` | 화면 출력, 창, 글자, 입력 |
| `src/ui.c` | 대화창, 선택지, 알림, 진행 막대, 작품 고르기, 한글 키보드 |
| `src/scene.c` | 방 장면 (계절 배경 + 수집품) |
| `src/game.c` | 게임 상태, 규칙, 성향·엔딩 계산, SRAM 저장 |
| `src/play.c` | 메인 화면, 일정 짜기, 활동, 위험 이벤트, 컬렉션·내 컴퓨터 |
| `src/story.c` | 프롤로그, 달별 이벤트, 졸업, 엔딩 |
| `src/sound.c` | 배경음과 효과음 |
| `src/text.c` | 조사(이/가), 돈·용량 표기 |
| `tools/make-data.js` | 웹판 데이터 → `src/data_gen.c` |
| `tools/make-font.js` | BDF 글꼴 → `src/font_data.c` |
| `tools/make-bg.js` | 웹판 그림 → `src/bg_data.c` (RGB555) |
| `tools/fix-header.js` | 헤더 체크섬, 롬 크기 맞추기 |
| `tools/autoplay.c` | libmgba로 자동 플레이하며 상태 확인 · 캡처 |

글꼴: Galmuri (SIL Open Font License 1.1, `tools/Galmuri-LICENSE.txt`)
