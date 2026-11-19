# 수집왕

1987년생 소년의 중학교 3년(2000~2003)을 키우는 도트 그래픽 육성 시뮬레이션.
무엇을 모으느냐가 3년 뒤 어떤 사람이 되는지를 정합니다.

- 모바일 세로 화면용 웹 게임 (PC 브라우저에서도 실행)
- 기획 문서: [plan.md](plan.md)
- 등장 작품 목록: [works.md](works.md)

## 실행

빌드가 필요 없습니다. `index.html`을 브라우저로 열거나, 로컬 서버로 띄우세요.

```bash
python3 -m http.server 8000
# 브라우저에서 http://localhost:8000
```

## 구조

| 파일 | 내용 |
|---|---|
| `js/data.js` | 작품 목록, 장르, 파일 종류, 인물, 상점 |
| `js/game.js` | 게임 상태, 다운로드·굽기·포맷 규칙, 성향과 엔딩 계산, 저장 |
| `js/art.js` | 모든 도트 그래픽 (방, 초상화, 아이콘, 조각 모음 화면) |
| `js/sound.js` | 배경음과 효과음 (Web Audio로 코드에서 합성) |
| `js/ui.js` | 대화창, 선택지, 윈도우 98 스타일 창, 작품 목록 |
| `js/story.js` | 프롤로그, 월별 이벤트, 히로인 이야기, 졸업식, 엔딩 |
| `js/main.js` | 타이틀, 허브, 일정, 활동, 위험·회복 이벤트 |

| `tools/schedule.js` | 작품 등장 시기를 고르게 다시 배치 (`--write`로 data.js에 반영) |
| `tools/works.js` | works.md(작품 목록) 다시 만들기 |
| `tools/sim.js` | 밸런스 시뮬레이터: `node tools/sim.js 2` 로 목표별 자동 플레이 결과와 엔딩 분포 출력 |

글꼴: [Galmuri](https://github.com/quiple/galmuri) (SIL Open Font License 1.1)
