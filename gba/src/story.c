// 수집왕 GBA — 이야기와 이벤트 (웹판 js/story.js를 그대로 옮김)
#include "common.h"

#define HER S.her
#define HG given_name(S.her)
#define SCH S.school

void love_add(int n) { S.love = clamp100(S.love + (n > 0 ? (n * 55 + 50) / 100 : n)); }
static void mom_add(int n) { S.mom = clamp100(S.mom + n); }
static void friend_add(int n) { S.friend = clamp100(S.friend + n); }
static const TasteInfo *T(void) { return &TASTES[S.taste]; }
static void refresh(void) { scene_room(); }

static const char *gift_info(int w) { return fmt("%d%s 보유", owned(w), TYPES[WORKS[w].type].unit); }

// 내 컬렉션에서 하나 골라 구워 주기 / USB에 담아 주기. 반환: 작품 번호 (-1 없음)
int give_from_collection(const char *title, const char *prompt, int *liked, int *usb) {
  u8 list[NWORKS];
  int n = owned_works(list);
  *usb = S.usb;
  if (!n) { narr("줄 만한 게 아직 하나도 없었다. 내 하드디스크는 너무 텅 비어 있었다."); return -1; }
  if (!S.usb && S.blank <= 0) { narr("구워 줄 공CD가 한 장도 없었다…"); return -1; }
  narr(prompt);
  int out[3];
  int k = pick_works(title, list, n, 1, gift_info, 0, 1, S.usb ? "USB에 담기" : "CD로 굽기 (공CD 1장)", out);
  refresh();
  if (k <= 0) return -1;
  int w = out[0];
  if (!S.usb) S.blank--;
  S.m_share += 80;
  S.st_gifts++;
  *liked = taste_likes(w);
  return w;
}

// ---------- 프롤로그 ----------
void story_prologue(void) {
  S.screen = SCR_OFF;
  refresh();
  narr("2000년 3월. 새 천년이 밝았고, 세상은 생각보다 멀쩡했다.");
  narr(fmt("나는 %s. 오늘부터 %s 1학년이다.", S.name, SCH));
  narr(fmt("교문 앞 벚나무 아래에 '%s 입학식'이라고 적힌 현수막이 걸려 있었다.", SCH));
  narr("작년 겨울, 아빠가 큰맘 먹고 컴퓨터를 사 왔다. 펜티엄 III, 하드디스크 10기가.");
  say(W_MOM, X_ANGRY, "그 컴퓨터, 공부하라고 사 준 거다. 알지?");
  narr("물론 안다. 그래도 하드디스크에서 3기가쯤은… 내 몫이다.");
  S.screen = SCR_DESKTOP;
  refresh();
  sfx(SFX_MODEM);
  narr("삐- 삐리리리- 치이이익-.");
  narr("모뎀이 전화선을 타고 세상에 접속하는 소리. 그 소리를 들을 때마다 가슴이 뛰었다.");
  narr("저 너머에, 내가 모을 수 있는 모든 것이 있었다.");
  static const char *const L[] = {
    "매달 세 번의 일정을 고르세요.",
    "무엇을 모으느냐에 따라 성향이 생기고, 성향이 3년 뒤의 엔딩을 정합니다.",
    "하드디스크 용량과 인터넷 속도를 잘 살피세요. CD로 구운 파일은 안전합니다.",
    "조작: A 결정 · B 취소/지우기 · 십자키 고르기 · L/R 목록 종류 · SELECT 소리 켜고 끄기",
  };
  notice("수집왕 안내", L, 4);
}

// ---------- 월초 고정 이벤트 ----------
void story_month_start(void) {
  int t = S.turn, y, m;
  date_of(t, &y, &m);

  if (m == 1 && t > 0) {   // 세뱃돈
    int gift = 40000 + (y - 2001) * 10000;
    S.money += gift / 2;
    sfx(SFX_COIN);
    narr(fmt("%d년 설날. 친척들에게 받은 세뱃돈이 %w.", y, gift));
    say(W_MOM, X_SMILE, "반은 엄마가 맡아 둘게.");
    narr(fmt("…그래서 내 손에 남은 건 %w. 나머지 반은 어디로 가는 걸까. 영원한 수수께끼다.", gift / 2));
  }

  if (t == 2 && S.st_modem >= 2) {   // 전화요금 고지서
    say(W_MOM, X_ANGRY, fmt("%s! 이번 달 전화요금이 얼만 줄 알아? 칠만 원이야, 칠만 원!", S.name));
    narr("밤마다 모뎀이 전화선을 붙잡고 있었으니까. 할 말이 없었다.");
    Opt o[2] = { { "잘못했다고 싹싹 빈다", "엄마 인내심 회복, 용돈 일부 반납", 0 }, { "\"밤에만 했는데…\"라고 변명한다", "엄마 인내심 하락", 0 } };
    if (choose(W_NARR, 0, "어떻게 할까?", o, 2) == 0) { S.money = MAX(0, S.money - 10000); mom_add(10); say(W_MOM, X_SAD, "…다음부턴 조심해. 용돈에서 만 원은 깐다."); }
    else { mom_add(-15); say(W_MOM, X_ANGRY, "밤에 하면 요금이 안 나오니? 당분간 지켜볼 거야."); }
  }

  if (t == 3) {   // CD 버너
    say(W_DAD, X_SMILE, "회사 김 과장이 CD 굽는 기계를 싸게 넘긴다더라. 하나 달아 줄까?");
    narr("CD-RW 드라이브! 이제 하드에 쌓인 파일을 CD에 구워 보관할 수 있다.");
    S.burner = 1; S.blank += 5;
    sfx(SFX_ITEM);
    refresh();
    static const char *const L[] = { "CD-RW 드라이브가 설치되었다.", "공CD 5장을 덤으로 받았다.", "이제 일정에서 \"CD 굽기\"를 고를 수 있습니다. 구운 파일은 하드에서 빠지고 안전하게 보관됩니다." };
    notice("새 장비", L, 3);
  }

  if (t == 7) {   // ADSL
    narr("10월의 토요일 오후. 초고속 인터넷 기사님이 다녀갔다.");
    narr("모뎀 대신 작은 상자가 깜빡였다. ADSL.");
    narr("하룻밤에 겨우 한 편 받던 것이, 이제는 열 편, 스무 편.");
    narr("더 이상 삐- 삐리리- 하는 소리는 들리지 않았다. …조금, 아주 조금 서운했다.");
    S.net = 1; S.line = 90;
    sfx(SFX_STARTUP);
    const char *L[] = { "인터넷이 ADSL로 바뀌었다.", fmt("다운로드 한 번에 받을 수 있는 양: %m → %m", 150, 2000) };
    notice("초고속 인터넷", L, 2);
  }

  if (t == 12 || t == 24) {   // 반 배정
    int same = chance(400 + S.love * 5);
    S.same_class = (u8)same;
    narr(fmt("%s %d학년 반 배정 날. 게시판 앞은 사람들로 북적였다.", SCH, grade_of(t)));
    if (same) { narr(fmt("%s 같은 반이었다. 나도 모르게 주먹을 쥐었다.", josa(HER, "이/가"))); love_add(3); }
    else narr(fmt("%s 옆 반이었다. 복도 끝 교실이 갑자기 멀게 느껴졌다.", josa(HER, "은/는")));
  }

  if (t == 13 && !S.scanner) {
    say(W_HYUNG, X_SMILE, "만화책 직접 스캔해서 올리는 사람들 있잖아. 그게 진짜 고수야. 용산 가면 중고 스캐너 4만 5천 원이면 산다.");
    narr("내 손으로 만든 파일. …용돈을 모아야겠다.");
  }

  if (t == 24) say(W_FRIEND, X_SURPRISE, "야, 너 USB 메모리라고 알아? 열쇠고리만 한 게 플로피 백 장이 들어간대. 용산에 나왔다더라.");

  if (t == 30) {   // PC 업그레이드
    if (S.hdd40) {
      say(W_DAD, X_SMILE, "하드 바꿔 주려고 했더니 벌써 네 돈으로 달았다며? …이건 용돈에 보태라.");
      S.money += 20000; sfx(SFX_COIN);
    } else if (S.mom >= 60) {
      say(W_DAD, X_SMILE, "요즘 엄마 말도 잘 듣는다며. 고입 준비도 해야 하니까… 40기가 하드 하나 달아 주마.");
      narr("40기가. 숫자를 듣는 것만으로 숨이 막혔다.");
      S.hdd40 = 1; S.hdd_cap += 25000; S.frag = 0;
      sfx(SFX_STARTUP);
      refresh();
      static const char *const L[] = { "40GB 하드디스크가 추가되었다. (내 몫 +25GB)", "디스크 조각화도 깨끗해졌다." };
      notice("PC 업그레이드", L, 2);
    } else {
      say(W_DAD, X_ANGRY, "하드를 바꿔 줄까 했는데… 엄마가 요즘 너 컴퓨터만 한다더라. 필요하면 네 돈으로 사.");
      narr("용산에서 40GB 하드가 11만 원이라던데.");
    }
  }
}

// ---------- 월말 고정 이벤트 (히로인 중심) ----------
void story_month_end(void) {
  int t = S.turn;
  const TasteInfo *Ta = T();

  if (t == 0) {   // 짝꿍
    narr(fmt("새 학기 첫 주, %s 1학년 교실. 담임 선생님이 번호순으로 짝을 정했다.", SCH));
    narr(fmt("내 옆자리는 %s. 조용하고, 필기를 예쁘게 하는 아이였다.", HER));
    say(W_HER, X_SURPRISE, "…그거, 필통에 붙인 스티커. 애니 맞지?");
    Opt o[3] = { { "\"응, 나 그런 거 좋아해.\"", 0, 0 }, { "\"아, 이거? 동생 거야.\"", 0, 0 }, { "(말없이 고개만 끄덕인다)", 0, 0 } };
    int c = choose(W_NARR, 0, "뭐라고 대답할까?", o, 3);
    if (c == 0) {
      love_add(8); S.flags |= F_OPENED;
      say(W_HER, X_SMILE, "그렇구나. …나도 사실.");
      narr(fmt("%s 말을 하다 말고 가방을 꼭 닫았다. 지퍼 틈으로 %s 언뜻 보였다.", josa(HER, "은/는"), josa(Ta->hint_item, "이/가")));
    } else if (c == 1) {
      love_add(2);
      say(W_HER, X_SAD, "아… 그렇구나.");
      narr(fmt("그 애가 서랍에 무언가를 급히 밀어 넣었다. %s… 였던 것 같다.", Ta->hint_item));
    } else {
      love_add(4);
      say(W_HER, X_SMILE, "…역시.");
      narr(fmt("그 애는 작게 웃고 다시 필기를 시작했다. 필통 안에 %s 보였다.", josa(Ta->hint_item, "이/가")));
    }
  }

  if (t == 5) {   // 여름방학
    static const char *const PLACE[3] = { "동네 만화 대여점", "도서 대여점의 판타지 소설 코너", "학교 앞 음반 가게" };
    narr(fmt("여름방학. 매미가 시끄럽던 오후, %s에서 %s 마주쳤다.", PLACE[S.taste], josa(HER, "을/를")));
    say(W_HER, X_BLUSH, "어… 안녕. 여기서 보네.");
    Opt o[2] = { { "\"뭐 찾는 거 있어? 같이 보자.\"", 0, 0 }, { "인사만 하고 지나간다", 0, 0 } };
    if (choose(W_NARR, 0, 0, o, 2) == 0) {
      love_add((S.flags & F_OPENED) ? 9 : 6);
      say(W_HER, X_BLUSH, "…비밀로 해 줄 거지? 반 애들한텐.");
      narr("고개를 끄덕였다. 그날, 우리에게 작은 비밀이 하나 생겼다.");
      S.flags |= F_SECRET;
    } else { love_add(1); narr("괜히 쑥스러워서 고개만 까딱하고 나왔다. 밖은 너무 더웠다."); }
  }

  if (t == 8) {   // 도서실
    static const char *const WHAT[3] = { "순정 만화 스캔본을 출력한 종이 묶음", "A4 용지에 빼곡히 출력한 판타지 소설", "가사를 손으로 옮겨 적은 노트와 공CD 한 장" };
    narr(fmt("점심시간의 도서실. 구석 자리에서 %s %s 몰래 보고 있었다.", josa(HER, "이/가"), josa(WHAT[S.taste], "을/를")));
    say(W_HER, X_SURPRISE, "앗…!");
    int have = 0;
    for (int w = 0; w < NWORKS; w++) if (owned(w) && taste_likes(w)) have = 1;
    Opt o[3] = { { "모른 척 지나간다", 0, 0 }, { have ? "\"그거 나도 있어. 하드에 다 있어.\"" : "\"그거 재밌지? 나도 좋아해.\"", 0, 0 }, { "\"너 이런 거 보는구나?\" 하고 놀린다", 0, 0 } };
    int c = choose(W_NARR, 0, 0, o, 3);
    if (c == 0) { love_add(4); S.flags |= F_KEPT; narr("못 본 척 책장 사이로 걸어갔다. 등 뒤에서 작게 숨을 내쉬는 소리가 들렸다."); }
    else if (c == 1) {
      love_add(have ? 12 : 6);
      say(W_HER, X_SMILE, have ? "진짜? …나중에 나도 좀 보여 줘." : "…응. 이거, 끝까지 다 읽으면 얘기해 줄게.");
      say(W_HER, X_BLUSH, Ta->hint_line);
    } else { love_add(-8); say(W_HER, X_SAD, "…"); narr("그 애는 종이를 접어 가방에 넣고 나가 버렸다. 괜한 말을 했다."); }
  }

  if (t == 9) {   // 크리스마스 카드
    narr("12월. 교실에 크리스마스 카드가 오가기 시작했다.");
    Opt o[2] = { { "쓴다", "반 아이들 전부에게 쓰는 척하면서", 0 }, { "쓰지 않는다", 0, 0 } };
    if (choose(W_NARR, 0, fmt("%s에게 카드를 쓸까?", HG), o, 2) == 0) {
      love_add(5);
      if (S.love >= 30) say(W_HER, X_BLUSH, "고마워. …나도 너한테 쓰려고 했는데.");
      else say(W_HER, X_SMILE, "아, 고마워. 메리 크리스마스.");
    }
  }

  if (t == 14) {   // 빌려주기
    narr(S.same_class ? "중간고사가 끝난 날, 쉬는 시간." : "중간고사가 끝난 날, 복도에서.");
    say(W_HER, X_SMILE, fmt("%s, 혹시… 볼 만한 거 있어? 시험 끝났으니까 보고 싶어서.", S.name));
    int liked = 0, usb = 0;
    int w = give_from_collection("빌려줄 작품", "무엇을 구워 줄까?", &liked, &usb);
    if (w < 0) { love_add(1); say(W_HER, X_SAD, "괜찮아, 다음에 줘."); }
    else if (liked) { love_add(14); say(W_HER, X_BLUSH, fmt("%s! 이거 진짜 보고 싶었어. 어떻게 알았어?", WORKS[w].title)); narr("어떻게 알았냐면… 1년 동안 지켜봤으니까. 물론 그렇게 말하진 않았다."); }
    else { love_add(6); say(W_HER, X_SMILE, fmt("%s… 고마워. 잘 볼게.", WORKS[w].title)); narr("조금 어색한 웃음이었다. 취향이 아니었던 걸까."); }
  }

  if (t == 16) {   // 여름 도서관
    narr("중2 여름방학. 에어컨 바람을 찾아 구립 도서관에 갔다.");
    narr(fmt("%s 창가 자리에서 졸고 있었다. 이어폰 한쪽이 빠져 있었다.", josa(HER, "이/가")));
    Opt o[3] = { { "옆자리에 조용히 앉는다", 0, 0 }, { "깨워서 인사한다", 0, 0 }, { "방해하지 않고 돌아간다", 0, 0 } };
    int c = choose(W_NARR, 0, 0, o, 3);
    if (c == 0) { love_add(7); narr("한 시간쯤 지났을까. 그 애가 눈을 떴다."); say(W_HER, X_BLUSH, "…언제부터 있었어?"); narr("\"방금.\" 거짓말이었다."); }
    else if (c == 1) { love_add(3); say(W_HER, X_SURPRISE, "어? 아… 침 흘렸어? 안 흘렸지?"); }
    else narr("조용히 돌아 나왔다. 그날 저녁은 왠지 다운로드가 손에 잡히지 않았다.");
  }

  if (t == 19) {   // 구하기 힘든 파일
    u8 cands[NWORKS]; int nc = 0;
    for (int w = 0; w < NWORKS; w++) if (taste_likes(w) && WORKS[w].appear <= t) cands[nc++] = (u8)w;
    int want = nc ? cands[rnd(nc)] : (int)rnd(NWORKS);
    int have = owned(want) > 0;
    say(W_HER, X_SAD, fmt("있잖아… 혹시 「%s」 구할 수 있어? 아무리 찾아도 없더라.", WORKS[want].title));
    Opt o[4]; int map[4], n = 0;
    if (have) { o[n] = (Opt){ "\"그거 나한테 있어!\"", 0, 0 }; map[n++] = 0; }
    o[n] = (Opt){ "오늘 밤새 받아 준다", S.net ? "엄마 인내심 하락" : "모뎀이라 오래 걸린다", 0 }; map[n++] = 1;
    o[n] = (Opt){ "동호회 형에게 부탁한다", S.st_club ? 0 : "동호회 활동을 해 본 적이 없다", 0 }; map[n++] = 2;
    o[n] = (Opt){ "\"같이 찾아보자.\"", 0, 0 }; map[n++] = 3;
    int c = map[choose(W_NARR, 0, 0, o, n)];
    if (c == 0) { love_add(15); say(W_HER, X_BLUSH, "진짜?! 역시 너는…"); narr("\"역시 너는\" 다음 말은 끝내 듣지 못했다. 그래도 충분했다."); }
    else if (c == 1) {
      mom_add(-12);
      int k = receive_disc(want, M_DOWNLOAD);
      love_add(S.net ? 13 : 8);
      narr(fmt("그날 밤, 엄마 눈을 피해 「%s」 받았다. (%d%s)", josa(WORKS[want].title, "을/를"), k, TYPES[WORKS[want].type].unit));
      say(W_HER, X_BLUSH, "밤샜어? 눈이 빨개. …고마워.");
    } else if (c == 2) {
      if (S.st_club) { love_add(11); receive_disc(want, M_TRADE); say(W_HYUNG, X_SMILE, "그거? 내 하드에 있지. 구워서 보내 줄게. 여자애 주는 거냐? ㅋㅋ"); say(W_HER, X_SMILE, "와… 너 아는 사람 많구나."); }
      else { love_add(2); narr("동호회에 가입만 해 두고 글을 쓴 적이 없어서… 아무도 답해 주지 않았다."); say(W_HER, X_SAD, "괜찮아. 없으면 말지 뭐."); }
    } else { love_add(10); narr("방과 후, 둘이서 PC방 구석에 나란히 앉아 검색창에 제목을 쳤다."); narr("결국 찾지는 못했지만, 그날 PC방 요금은 하나도 아깝지 않았다."); }
  }

  if (t == 21) {   // 2001 크리스마스
    if (S.love >= 35) {
      say(W_HER, X_BLUSH, "이번에 해리 포터 영화 개봉했대. …보러 갈 사람?");
      Opt o[2] = { { "\"내가 표 살게. 같이 가자.\"", fmt("영화표 두 장과 팝콘 %w", 14000), S.money < 14000 }, { "\"비디오 나오면 같이 보자.\"", 0, 0 } };
      if (choose(W_NARR, 0, 0, o, 2) == 0) { S.money -= 14000; love_add(10); sfx(SFX_COIN); narr("종로의 극장. 팝콘 한 통을 나눠 먹었다."); say(W_HER, X_SMILE, "나중에 이거 파일 생기면 나도 줘. 또 볼래."); }
      else { love_add(2); say(W_HER, X_SAD, "…응, 그래."); }
    }
    if (S.love >= 40) {
      narr("12월 24일. 사물함에 작은 봉투가 들어 있었다.");
      say(W_HER, X_BLUSH, fmt("(카드) 올해도 고마웠어. 내년에도 재밌는 거 알려 줘. -%s", HG));
      Opt o[3] = { { "작은 선물과 함께 답장을 쓴다", fmt("문구점 선물 %w", 8000), S.money < 8000 }, { "답장을 쓴다", 0, 0 }, { "고맙다고 말로 한다", 0, 0 } };
      int c = choose(W_NARR, 0, 0, o, 3);
      if (c == 0) S.money -= 8000;
      love_add(c == 0 ? 10 : c == 1 ? 7 : 4);
    } else narr("12월 24일. 거리마다 캐럴이 흘렀다. 나는 방에서 받은 파일들을 정리했다.");
  }

  if (t == 23) {   // 밸런타인
    if (S.love >= 50) {
      narr(fmt("2월 14일. 하굣길에 %s 무언가를 내밀었다.", josa(HER, "이/가")));
      say(W_HER, X_BLUSH, "의, 의리야. 반 애들 다 줬어.");
      narr("반 애들 다 줬다는 초콜릿은, 내 것만 리본이 달려 있었다.");
      love_add(5);
    } else narr("2월 14일. 초콜릿은 없었다. 대신 하드디스크에 새 파일이 하나 늘었다.");
  }

  if (t == 26) {   // 생일
    say(W_FRIEND, X_SURPRISE, fmt("야, 다음 주 %s 생일이래. 너 알고 있었냐?", HG));
    narr("알고 있었다. 달력에 몰래 표시까지 해 두었으니까.");
    Opt o[3] = { { "내 컬렉션에서 골라 담아 준다", 0, 0 }, { "선물 가게에서 산다", fmt("오르골 %w", 15000), S.money < 15000 }, { "둘 다", fmt("오르골 %w + 컬렉션", 15000), S.money < 15000 } };
    int how = choose(W_NARR, 0, "생일 선물은…", o, 3);
    if (how >= 1) { S.money -= 15000; love_add(8); sfx(SFX_COIN); narr("오르골을 샀다. 태엽을 감으면 그 시절 유행하던 노래가 흘러나왔다."); say(W_HER, X_BLUSH, "…예쁘다. 고마워."); }
    int liked = 0, usb = 0, w = -1;
    if (how != 1) w = give_from_collection("생일 선물", "무엇을 담아서 선물할까?", &liked, &usb);
    if (how == 1 || (w < 0 && how == 2)) { /* 오르골만 */ }
    else if (w < 0) { love_add(2); narr("결국 문방구에서 산 볼펜 한 자루를 건넸다."); say(W_HER, X_SMILE, "고마워. 잘 쓸게."); }
    else {
      love_add((liked ? 15 : 7) + (usb ? 4 : 0));
      narr(usb ? fmt("USB 메모리에 「%s」 담고, 메모장 파일로 짧은 생일 축하 글을 넣었다.", josa(WORKS[w].title, "을/를")) : fmt("CD에 「%s」 굽고, 매직으로 정성껏 제목을 썼다.", josa(WORKS[w].title, "을/를")));
      if (liked) say(W_HER, X_BLUSH, "…이거, 내가 제일 좋아하는 거야. 말한 적 없는데.");
      else say(W_HER, X_SMILE, "와, 직접 만든 거야? 고마워.");
    }
  }

  if (t == 27) {   // 월드컵
    narr("2002년 6월. 온 나라가 붉게 물들었다. 대~한민국!");
    say(W_FRIEND, X_SMILE, "야, 오늘 광장 가자! 다들 간대!");
    Opt o[3] = { { fmt("%s에게 같이 가자고 한다", HG), 0, 0 }, { "친구들과 간다", 0, 0 }, { "집에서 경기 보면서 다운로드를 돌린다", 0, 0 } };
    int c = choose(W_NARR, 0, 0, o, 3);
    if (c == 0) {
      int ok = S.love >= 40;
      love_add(ok ? 13 : 6);
      if (ok && S.money >= 10000) {
        Opt o2[2] = { { "\"Be the Reds\" 티셔츠를 두 장 산다", wonstr(10000), 0 }, { "각자 집에 있는 빨간 옷을 입는다", 0, 0 } };
        if (choose(W_HER, X_BLUSH, "…좋아. 빨간 티 있어?", o2, 2) == 0) { S.money -= 10000; love_add(4); sfx(SFX_COIN); }
      }
      if (ok) { say(W_HER, X_BLUSH, "…좋아. 빨간 티 입고 갈게."); narr("골이 들어간 순간, 모르는 사람들과 얼싸안았다. 그리고 그 애와도."); narr("그해 여름은, 평생 잊지 못할 것이다."); }
      else { say(W_HER, X_SAD, "아… 나 친구들이랑 약속이 있어서. 미안."); narr("광장에서 멀리, 그 애의 빨간 티를 본 것 같았다."); }
    } else if (c == 1) { friend_add(15); narr("목이 쉬도록 소리 질렀다. 태식이는 집에 가는 버스에서 울었다."); }
    else {
      mom_add(5); S.flags |= F_WCHOME;
      u8 av[NWORKS]; int na = 0;
      for (int w = 0; w < NWORKS; w++) if (WORKS[w].appear <= t && remaining(w) > 0 && WORKS[w].type == T_ANIME) av[na++] = (u8)w;
      if (na) {
        int ids[1] = { av[rnd(na)] }; DlRes r[1]; int full; s32 left;
        do_download(ids, 1, S.net ? 2000 : 150, r, &full, &left);
        narr(fmt("창밖의 함성을 들으며 「%s」 받았다. (%d화)", josa(WORKS[ids[0]].title, "을/를"), r[0].got));
        narr("…조금 외로웠다.");
      }
    }
  }

  if (t == 32) {   // 고입 준비
    narr("11월. 고입 연합고사가 코앞이었다. 다들 말수가 줄었다.");
    narr(fmt("%s 요즘 쉬는 시간에도 문제집만 풀었다.", josa(HER, "은/는")));
    Opt o[3] = { { "힘내라고 무언가를 담아 준다", 0, 0 }, { "쪽지만 건넨다: \"시험 끝나면 같이 보자\"", 0, 0 }, { "방해하지 않는다", 0, 0 } };
    int c = choose(W_NARR, 0, 0, o, 3);
    if (c == 0) {
      int liked = 0, usb = 0;
      int w = give_from_collection("응원 선물", "시험 끝나고 볼 만한 걸 골라 주자.", &liked, &usb);
      if (w >= 0) { love_add(liked ? 12 : 6); say(W_HER, X_SMILE, "시험 끝나면 이것부터 볼게. 약속."); }
    } else if (c == 1) { love_add(8); say(W_HER, X_BLUSH, "(쪽지 뒷면) 응. 꼭."); }
    else love_add(1);
  }

  if (t == 17 || t == 29) {   // 친구의 한마디
    int L = S.love;
    if (L >= 60) say(W_FRIEND, X_SMILE, fmt("야, %s 너 얘기할 때 표정이 다르더라. 알고 있냐?", josa(HG, "이/가")));
    else if (L >= 35) say(W_FRIEND, X_SMILE, fmt("%s 너 뭐 모으는지 물어보더라. 신기하지?", josa(HG, "이/가")));
    else say(W_FRIEND, X_NORMAL, fmt("%s? 걔 요즘 너랑 말 별로 안 하던데. 무슨 일 있었냐?", HG));
  }
}

// ---------- 졸업식 ----------
static int school_round(void) { u32 h = 7; for (const char *p = S.school; *p;) h = h * 31 + utf8_next(&p); return 20 + h % 31; }

void story_graduation(void) {
  S.screen = SCR_OFF;
  refresh();
  narr("2003년 2월. 졸업식 날 아침. 학교 가는 길에 꽃집이 문을 열고 있었다.");
  if (S.money >= 10000) {
    Opt o[2] = { { "작은 꽃다발을 산다", wonstr(10000), 0 }, { "그냥 간다", 0, 0 } };
    if (choose(W_NARR, 0, fmt("가진 돈 %w", S.money), o, 2) == 0) { S.money -= 10000; love_add(4); sfx(SFX_COIN); narr("프리지어 한 다발. 노란색이 그 애한테 어울릴 것 같았다."); }
  }
  narr(fmt("2003년 2월. %s 제%d회 졸업식.", SCH, school_round()));
  narr("교복 위로 밀가루가 날렸고, 누군가는 울었고, 누군가는 웃었다.");
  narr(fmt("3년 동안 내 방에는 CD가 %d장 쌓였다.", S.st_discs));
  narr(fmt("운동장 끝, 플라타너스 아래에 %s 서 있었다.", josa(HER, "이/가")));
  say(W_HER, S.love >= 50 ? X_BLUSH : X_SMILE, fmt("%s. …졸업 축하해.", S.name));
  Opt o[3] = { { "\"좋아해. 처음 짝꿍이었을 때부터.\"", 0, 0 }, { "밤새 쓴 편지를 건넨다", 0, 0 }, { "\"…고등학교 가서도 잘 지내.\"", 0, 0 } };
  int c = choose(W_NARR, 0, "마지막으로 할 말은…", o, 3);
  S.grad = (u8)(c + 1);
  if (c == 0) {
    if (S.love >= 90) { say(W_HER, X_BLUSH, "…바보. 나도야. 그걸 이제 말하면 어떡해."); narr("그 애가 웃으면서 울었다. 나도 그랬던 것 같다."); }
    else if (S.love >= 60) say(W_HER, X_SAD, "…고마워. 근데 나, 아직 잘 모르겠어. 우리 계속 연락하자. 응?");
    else { say(W_HER, X_SAD, "아… 미안. 나는 너를 좋은 친구라고 생각했어."); narr("봄바람이 차가웠다."); }
  } else if (c == 1) {
    if (S.love >= 60) say(W_HER, X_BLUSH, "편지? …집에 가서 읽을게. 나도 답장 쓸게. 꼭.");
    else say(W_HER, X_SMILE, "아, 고마워. 잘 읽을게.");
  } else { say(W_HER, S.love >= 50 ? X_SAD : X_SMILE, "응. 너도."); narr("그 말이 마지막이었다. 그때는 몰랐다."); }
}

// ---------- 엔딩 ----------
static const char *const ROM_TITLE[4] = { "함께", "그 시절의 편지", "다시 만남", "기억 속의 그 아이" };

static void ending_title(const char *label, const char *title) {
  const char *L[1] = { title };
  notice(label, L, 1);
}

void story_ending(void) {
  int e = decide_ending(), r = decide_romance();
  Traits t; compute_traits(&t);
  META.endings |= 1u << e; META.romance |= 1u << r; META.plays++;
  meta_save();
  fade(1);
  scene_future();
  fade(-1);
  bgm(BGM_ENDING);
  narr("그리고 시간이 흘렀다.");
  narr(fmt("%s 졸업한 지 스물세 해. 교문 앞 문방구는 편의점이 되었다고 한다.", josa(SCH, "을/를")));
  const char *L[] = {
    fmt("최종 성향: %s", trait_label(&t)),
    fmt("가장 많이 모은 매체: %s", t.media_top >= 0 ? MEDIA_NAMES[t.media_top] : "없음"),
    fmt("1위 장르 점수: %d (기준 110) · 수집 점수 %d", (int)(t.top_score / 10), (int)(t.total / 10)),
    fmt("모은 작품: %d개 (완결 %d개)", t.owned_count, t.completed),
    fmt("구운 CD: %d장 · 포맷 %d번", S.st_discs, S.st_formats),
  };
  notice("성향 결과", L, 5);
  ending_title("엔딩", ENDINGS[e].title);
  for (int i = 0; i < ENDINGS[e].nlines; i++) narr(ENDINGS[e].lines[i]);
  ending_title("에필로그", ROM_TITLE[r]);
  const char *h = HER;
  if (r == 0) {
    narr(fmt("그리고 지금, 내 옆에는 %s 있다.", josa(h, "이/가")));
    narr("우리는 결혼하면서 서로의 하드디스크를 합쳤다. 겹치는 파일이 생각보다 많아서 한참 웃었다.");
    narr(fmt("\"이거 기억나? 네가 처음 구워 준 CD.\" %s 낡은 CD 케이스를 흔든다.", josa(h, "이/가")));
    narr("그 CD는 아직도 잘 읽힌다.");
  } else if (r == 1) {
    narr(fmt("%s는 다른 고등학교에 갔고, 그 뒤로는 자주 보지 못했다.", josa(h, "와/과")));
    narr("그래도 우리는 연락을 끊지 않았다. 이메일에서 메신저로, 메신저에서 단톡방으로.");
    narr(fmt("해마다 생일이면 %s 노래 한 곡을 보낸다. 그 시절 우리가 함께 듣던.", josa(h, "은/는")));
    narr("사랑이었는지 우정이었는지, 이제는 중요하지 않다.");
  } else if (r == 2) {
    narr("졸업 후 20년 동안, 그 애를 잊고 살았다.");
    narr("그러다 작년, 중고 거래 앱에서 옛날 CD를 사려고 약속 장소에 나갔다.");
    narr(fmt("거기 %s 서 있었다. \"…%s? 설마 이 CD, 너야?\"", josa(h, "이/가"), S.name));
    narr("우리는 카페에 앉아 세 시간 동안 그 시절 이야기를 했다. 다음 주에 또 만나기로 했다.");
  } else {
    narr(fmt("%s 어떻게 지내는지 나는 모른다.", josa(h, "이/가")));
    narr("가끔 필통에 스티커를 붙인 아이들을 보면, 그 시절 짝꿍이 생각난다.");
    narr("\"…그거, 애니 맞지?\"");
    narr("그 질문에, 나는 제대로 대답했던가.");
  }
  const char *E[] = {
    fmt("%s의 %s 3년은 이렇게 끝났습니다.", S.name, SCH),
    fmt("엔딩: %s / %s", ENDINGS[e].title, ROM_TITLE[r]),
    "다른 것을 모으면, 다른 사람이 됩니다. 히로인의 이름과 취향도 매번 바뀝니다.",
  };
  notice("THE END", E, 3);
}
