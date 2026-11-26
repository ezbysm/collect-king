# 수집왕 GBA (1단계 시험 롬)

웹판 수집왕을 게임보이 어드밴스(GBA) 롬으로 옮기는 작업입니다. 지금은 **1단계: 기반 확인용 시험 롬**입니다.

- 시험 롬: [`collect-king-test.gba`](collect-king-test.gba) (약 107KB)
- 에뮬레이터 캡처: [`screenshots/`](screenshots/)

## 시험 롬에서 되는 것
- 웹판 도트 그래픽(가을 저녁의 방, 배경화면)을 240×160 화면에 옮긴 배경
- 한글 도트 글꼴(Galmuri11)로 대사 출력 — 게임 전체에 쓰이는 글자 901자를 넣음
- 타자 치듯 나오는 대사, A 버튼으로 빨리 넘기기 / 다음 대사, 단어 단위 줄바꿈
- 그 아이 초상화가 있는 대화창
- 십자키 위·아래로 고르고 A로 정하는 선택지

조작: **A** 다음·결정, **십자키 위/아래** 고르기

## 해 보기
- **아이폰**: GBA 에뮬레이터(예: Delta)에 `collect-king-test.gba`를 불러오기
- **PC**: mGBA 같은 에뮬레이터로 열기
- **실기(플래시 카트리지)**: 롬 헤더의 닌텐도 로고 자리를 비워 두었기 때문에, devkitPro의 `gbafix`로 로고를 넣어야 실기에서 켜집니다. 에뮬레이터에서는 그대로 실행됩니다.

## 빌드
Ubuntu 기준:
```bash
sudo apt install gcc-arm-none-eabi libnewlib-arm-none-eabi   # 컴파일러
sudo apt install libmgba-dev libpng-dev                       # (선택) 화면 캡처 테스트용
cd gba
make          # → build/collect-king.gba
make shot     # 에뮬레이터 코어로 실행하며 화면 캡처 → build/shot-*.png
```

글꼴·배경 데이터(`src/font_data.c`, `src/bg_data.c`)는 만들어 둔 것이 저장소에 있습니다. 다시 만들려면:
```bash
# Galmuri11.bdf 받기
(cd tools && npm pack galmuri@2.40.3 && tar xzf galmuri-2.40.3.tgz package/dist/Galmuri11.bdf --strip-components=2)
# 저장소 루트에서 정적 서버를 띄운 뒤 (배경은 웹판 js/art.js로 그림)
python3 -m http.server 8765 &   # 저장소 루트에서
cd gba && make assets
```

## 구조
| 파일 | 내용 |
|---|---|
| `src/crt0.s` | 시작 코드와 롬 헤더 |
| `gba.ld` | 메모리 배치 (코드·데이터는 카트리지, 변수는 IWRAM) |
| `src/main.c` | 화면 출력, 글자 그리기, 입력, 대화·선택지 |
| `src/font_data.c` | 한글 글꼴 (자동 생성) |
| `src/bg_data.c` | 배경과 초상화 그림 (자동 생성) |
| `tools/make-font.js` | 게임에 쓰인 글자만 BDF 글꼴에서 뽑기 |
| `tools/make-bg.js` | 웹판 그림을 GBA 색(RGB555)으로 바꾸기 |
| `tools/fix-header.js` | 롬 헤더 체크섬 채우기 |
| `tools/shot.c` | libmgba로 롬을 돌리며 화면 캡처 |

## 다음 단계
2. 장면: 방 그림을 상황에 따라 바꾸기, 대화창·선택지 다듬기, 메뉴 화면
3. 게임 진행: 월별 진행, 일정, 활동, 수집, 저장(SRAM)
4. 이야기: 이벤트, 히로인, 엔딩
5. 소리

글꼴: Galmuri (SIL Open Font License 1.1, `tools/Galmuri-LICENSE.txt`)
