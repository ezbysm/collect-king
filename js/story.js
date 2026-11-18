// 수집왕 — 이야기와 이벤트
'use strict';

// 받침에 따라 조사 고르기: josa('지은', '이/가') → '지은이'
function josa(word, pair) {
  const [a, b] = pair.split('/');
  const c = word.charCodeAt(word.length - 1);
  const has = c >= 0xac00 && c <= 0xd7a3 && (c - 0xac00) % 28 !== 0;
  return word + (has ? a : b);
}
// 이름 부르기: 지은아 / 수지야
function callName(name) {
  const given = name.length >= 3 ? name.slice(1) : name;
  return josa(given, '아/야');
}

const Story = (() => {
  const s = () => G.s;
  const N = () => s().name;
  const HER = () => s().heroine.name;
  const HG = () => (HER().length >= 3 ? HER().slice(1) : HER()); // 성을 뺀 이름
  const taste = () => HEROINE_TASTES[s().heroine.taste];
  const say = (...a) => UI.say(...a);
  const talk = l => UI.talk(l);
  const love = n => { s().love = clamp(s().love + n); };
  const mom = n => { s().mom = clamp(s().mom + n); };
  const friend = n => { s().friend = clamp(s().friend + n); };

  // 내 컬렉션에서 하나 골라 구워 주기 / USB에 담아 주기
  async function giveFromCollection(title, prompt) {
    const st = s();
    const works = ownedWorks(st);
    const canUsb = st.pc.usb;
    if (!works.length) { await say('narr', '줄 만한 게 아직 하나도 없었다. 내 하드디스크는 너무 텅 비어 있었다.'); return null; }
    if (!canUsb && st.blank <= 0) { await say('narr', '구워 줄 공CD가 한 장도 없었다…'); return null; }
    await say('narr', prompt);
    const ids = await UI.pickWorks(title, works, {
      max: 1, allowCancel: true, confirmLabel: canUsb ? 'USB에 담기' : 'CD로 굽기',
      info: w => `${owned(st, w.id)}${TYPES[w.type].unit} 보유`,
      hint: canUsb ? 'USB 메모리에 담아 줍니다' : '공CD 1장을 씁니다',
    });
    if (!ids || !ids.length) return null;
    const w = WORK_BY_ID[ids[0]];
    if (!canUsb) st.blank--;
    st.method.share += 8;
    st.stats.gifts++;
    return { w, liked: taste().likes(w), usb: canUsb };
  }

  // ---------- 프롤로그 ----------
  async function prologue() {
    UI.setScene('room');
    s().screen = 'off';
    await talk([
      '2000년 3월. 새 천년이 밝았고, 세상은 생각보다 멀쩡했다.',
      `나는 ${N()}. 오늘부터 중학생이다.`,
      '작년 겨울, 아빠가 큰맘 먹고 컴퓨터를 사 왔다. 펜티엄 III, 하드디스크 10기가.',
      ['mom', '그 컴퓨터, 공부하라고 사 준 거다. 알지?', 'angry'],
      '물론 안다. 그래도 하드디스크에서 3기가쯤은… 내 몫이다.',
    ]);
    s().screen = 'desktop';
    Sound.sfx('startup');
    setTimeout(() => Sound.sfx('modem'), 1800);
    await talk([
      '삐- 삐리리리- 치이이익-.',
      '모뎀이 전화선을 타고 세상에 접속하는 소리. 그 소리를 들을 때마다 가슴이 뛰었다.',
      '저 너머에, 내가 모을 수 있는 모든 것이 있었다.',
    ]);
    await UI.notice('수집왕 안내', [
      '매달 세 번의 일정을 고르세요.',
      '무엇을 모으느냐에 따라 성향이 생기고, 성향이 3년 뒤의 엔딩을 정합니다.',
      '하드디스크 용량과 인터넷 속도를 잘 살피세요. CD로 구운 파일은 안전합니다.',
    ], '시작');
  }

  // ---------- 월초 고정 이벤트 ----------
  async function monthStart() {
    const st = s();
    const d = dateOf(st.turn);
    const t = st.turn;

    // 세뱃돈
    if (d.m === 1 && t > 0) {
      const gift = 30000 + (d.y - 2001) * 5000;
      st.money += gift;
      Sound.sfx('coin');
      await talk([`${d.y}년 설날. 친척들에게 받은 세뱃돈이 ${won(gift)}.`, ['mom', '반은 엄마가 맡아 둘게.', 'smile'], '…그 반은 어디로 가는 걸까. 영원한 수수께끼다.']);
    }

    // 전화요금 고지서 (모뎀 시절)
    if (t === 2 && st.stats.modemDownloads >= 2) {
      await talk([
        ['mom', `${N()}! 이번 달 전화요금이 얼만 줄 알아? 칠만 원이야, 칠만 원!`, 'angry'],
        '밤마다 모뎀이 전화선을 붙잡고 있었으니까. 할 말이 없었다.',
      ]);
      const c = await UI.choose('어떻게 할까?', [
        { label: '잘못했다고 싹싹 빈다', value: 'sorry', note: '엄마 인내심 회복, 용돈 일부 반납' },
        { label: '"밤에만 했는데…"라고 변명한다', value: 'excuse', note: '엄마 인내심 하락' },
      ]);
      if (c === 'sorry') { st.money = Math.max(0, st.money - 10000); mom(10); await say('mom', '…다음부턴 조심해. 용돈에서 만 원은 깐다.', 'sad'); }
      else { mom(-15); await say('mom', '밤에 하면 요금이 안 나오니? 당분간 지켜볼 거야.', 'angry'); }
    }

    // CD 버너
    if (t === 3) {
      await talk([
        ['dad', '회사 김 과장이 CD 굽는 기계를 싸게 넘긴다더라. 하나 달아 줄까?', 'smile'],
        'CD-RW 드라이브! 이제 하드에 쌓인 파일을 CD에 구워 보관할 수 있다.',
      ]);
      st.pc.burner = true; st.blank += 5;
      Sound.sfx('item');
      await UI.notice('새 장비', ['CD-RW 드라이브가 설치되었다.', '공CD 5장을 덤으로 받았다.', '이제 일정에서 "CD 굽기"를 고를 수 있습니다. 구운 파일은 하드에서 빠지고 안전하게 보관됩니다.']);
    }

    // ADSL 개통
    if (t === 7) {
      await talk([
        '10월의 토요일 오후. 초고속 인터넷 기사님이 다녀갔다.',
        '모뎀 대신 작은 상자가 깜빡였다. ADSL.',
        '하룻밤에 겨우 한 편 받던 것이, 이제는 열 편, 스무 편.',
        '더 이상 삐- 삐리리- 하는 소리는 들리지 않았다. …조금, 아주 조금 서운했다.',
      ]);
      st.net = 'adsl'; st.pc.line = 90;
      Sound.sfx('startup');
      await UI.notice('초고속 인터넷', ['인터넷이 ADSL로 바뀌었다.', `다운로드 한 번에 받을 수 있는 양: ${mb(SPEED.modem)} → ${mb(SPEED.adsl)}`]);
    }

    // 2학년, 3학년 반 배정
    if (t === 12 || t === 24) {
      const same = chance(0.4 + st.love / 200);
      st.heroine.sameClass = same;
      await talk([`${d.grade}학년 반 배정 날. 게시판 앞은 사람들로 북적였다.`]);
      if (same) {
        await talk([`${josa(HER(), '이/가')} 같은 반이었다. 나도 모르게 주먹을 쥐었다.`]);
        love(3);
      } else {
        await talk([`${josa(HER(), '은/는')} 옆 반이었다. 복도 끝 교실이 갑자기 멀게 느껴졌다.`]);
      }
    }

    // 스캐너
    if (t === 13 && !st.pc.scanner) {
      await talk([
        ['hyung', '만화책 직접 스캔해서 올리는 사람들 있잖아. 그게 진짜 고수야. 용산 가면 중고 스캐너 4만 5천 원이면 산다.', 'smile'],
        '내 손으로 만든 파일. …용돈을 모아야겠다.',
      ]);
    }

    // USB 메모리 등장
    if (t === 24) {
      await talk([['friend', '야, 너 USB 메모리라고 알아? 열쇠고리만 한 게 플로피 백 장이 들어간대. 용산에 나왔다더라.', 'surprise']]);
    }

    // PC 업그레이드
    if (t === 30) {
      if (st.pc.hdd40) {
        await talk([['dad', '하드 바꿔 주려고 했더니 벌써 네 돈으로 달았다며? …이건 용돈에 보태라.', 'smile']]);
        st.money += 20000; Sound.sfx('coin');
      } else if (st.mom >= 60) {
        await talk([
          ['dad', '요즘 엄마 말도 잘 듣는다며. 고입 준비도 해야 하니까… 40기가 하드 하나 달아 주마.', 'smile'],
          '40기가. 숫자를 듣는 것만으로 숨이 막혔다.',
        ]);
        st.pc.hdd40 = true; st.hddCap += 25000; st.pc.frag = 0;
        Sound.sfx('startup');
        await UI.notice('PC 업그레이드', ['40GB 하드디스크가 추가되었다. (내 몫 +25GB)', '디스크 조각화도 깨끗해졌다.']);
      } else {
        await talk([['dad', '하드를 바꿔 줄까 했는데… 엄마가 요즘 너 컴퓨터만 한다더라. 필요하면 네 돈으로 사.', 'angry'], '용산에서 40GB 하드가 11만 원이라던데.']);
      }
    }
  }

  // ---------- 월말 고정 이벤트 (히로인 중심) ----------
  async function monthEnd() {
    const st = s();
    const t = st.turn;
    const T = taste();

    if (t === 0) { // 중1 1학기 — 짝꿍
      await talk([
        '새 학기 첫 주. 담임 선생님이 번호순으로 짝을 정했다.',
        `내 옆자리는 ${HER()}. 조용하고, 필기를 예쁘게 하는 아이였다.`,
        ['her', '…그거, 필통에 붙인 스티커. 애니 맞지?', 'surprise'],
      ]);
      const c = await UI.choose('뭐라고 대답할까?', [
        { label: '"응, 나 그런 거 좋아해."', value: 'open' },
        { label: '"아, 이거? 동생 거야."', value: 'hide' },
        { label: '(말없이 고개만 끄덕인다)', value: 'nod' },
      ]);
      if (c === 'open') {
        love(8); st.flags.openedUp = true;
        await talk([['her', '그렇구나. …나도 사실.', 'smile'], `${josa(HER(), '은/는')} 말을 하다 말고 가방을 꼭 닫았다. 지퍼 틈으로 ${T.hintItem}이 언뜻 보였다.`]);
      } else if (c === 'hide') {
        love(2);
        await talk([['her', '아… 그렇구나.', 'sad'], `그 애가 서랍에 무언가를 급히 밀어 넣었다. ${T.hintItem}… 였던 것 같다.`]);
      } else {
        love(4);
        await talk([['her', '…역시.', 'smile'], `그 애는 작게 웃고 다시 필기를 시작했다. 필통 안에 ${T.hintItem}이 보였다.`]);
      }
    }

    if (t === 5) { // 여름방학 — 우연한 만남
      const place = { love: '동네 만화 대여점', fan: '도서 대여점의 판타지 소설 코너', music: '학교 앞 음반 가게' }[st.heroine.taste];
      await talk([`여름방학. 매미가 시끄럽던 오후, ${place}에서 ${josa(HER(), '을/를')} 마주쳤다.`, ['her', '어… 안녕. 여기서 보네.', 'blush']]);
      const c = await UI.choose('', [
        { label: '"뭐 찾는 거 있어? 같이 보자."', value: 'join' },
        { label: '인사만 하고 지나간다', value: 'pass' },
      ]);
      if (c === 'join') {
        love(st.flags.openedUp ? 9 : 6);
        await talk([['her', '…비밀로 해 줄 거지? 반 애들한텐.', 'blush'], '고개를 끄덕였다. 그날, 우리에게 작은 비밀이 하나 생겼다.']);
        st.flags.secret = true;
      } else {
        love(1);
        await talk(['괜히 쑥스러워서 고개만 까딱하고 나왔다. 밖은 너무 더웠다.']);
      }
    }

    if (t === 8) { // 중1 2학기 — 도서실
      const what = { love: '순정 만화 스캔본을 출력한 종이 묶음', fan: 'A4 용지에 빼곡히 출력한 판타지 소설', music: '가사를 손으로 옮겨 적은 노트와 공CD 한 장' }[st.heroine.taste];
      await talk([`점심시간의 도서실. 구석 자리에서 ${josa(HER(), '이/가')} ${what}을 몰래 보고 있었다.`, ['her', '앗…!', 'surprise']]);
      const have = ownedWorks(st).some(w => T.likes(w));
      const c = await UI.choose('', [
        { label: '모른 척 지나간다', value: 'ignore' },
        { label: have ? '"그거 나도 있어. 하드에 다 있어."' : '"그거 재밌지? 나도 좋아해."', value: 'share' },
        { label: '"너 이런 거 보는구나?" 하고 놀린다', value: 'tease' },
      ]);
      if (c === 'ignore') { love(4); st.flags.keptSecret = true; await talk(['못 본 척 책장 사이로 걸어갔다. 등 뒤에서 작게 숨을 내쉬는 소리가 들렸다.']); }
      else if (c === 'share') {
        love(have ? 12 : 6);
        await talk([['her', have ? '진짜? …나중에 나도 좀 보여 줘.' : '…응. 이거, 끝까지 다 읽으면 얘기해 줄게.', 'smile'], T.hintLine ? ['her', T.hintLine, 'blush'] : '']);
      } else { love(-8); await talk([['her', '…', 'sad'], '그 애는 종이를 접어 가방에 넣고 나가 버렸다. 괜한 말을 했다.']); }
    }

    if (t === 9) { // 크리스마스 카드
      await talk(['12월. 교실에 크리스마스 카드가 오가기 시작했다.']);
      const c = await UI.choose(`${HG()}에게 카드를 쓸까?`, [
        { label: '쓴다', value: 'yes', note: '반 아이들 전부에게 쓰는 척하면서' },
        { label: '쓰지 않는다', value: 'no' },
      ]);
      if (c === 'yes') {
        love(5);
        await talk([st.love >= 30 ? ['her', '고마워. …나도 너한테 쓰려고 했는데.', 'blush'] : ['her', '아, 고마워. 메리 크리스마스.', 'smile']]);
      }
    }

    if (t === 14) { // 중2 1학기 — 빌려주기
      await talk([
        st.heroine.sameClass ? '중간고사가 끝난 날, 쉬는 시간.' : '중간고사가 끝난 날, 복도에서.',
        ['her', `${N()}, 혹시… 볼 만한 거 있어? 시험 끝났으니까 보고 싶어서.`, 'smile'],
      ]);
      const g = await giveFromCollection('빌려줄 작품', '무엇을 구워 줄까?');
      if (!g) { love(1); await say('her', '괜찮아, 다음에 줘.', 'sad'); }
      else if (g.liked) { love(14); await talk([['her', `${g.w.title}! 이거 진짜 보고 싶었어. 어떻게 알았어?`, 'blush'], '어떻게 알았냐면… 1년 동안 지켜봤으니까. 물론 그렇게 말하진 않았다.']); }
      else { love(6); await talk([['her', `${g.w.title}… 고마워. 잘 볼게.`, 'smile'], '조금 어색한 웃음이었다. 취향이 아니었던 걸까.']); }
    }

    if (t === 16) { // 여름 도서관
      await talk(['중2 여름방학. 에어컨 바람을 찾아 구립 도서관에 갔다.', `${josa(HER(), '이/가')} 창가 자리에서 졸고 있었다. 이어폰 한쪽이 빠져 있었다.`]);
      const c = await UI.choose('', [
        { label: '옆자리에 조용히 앉는다', value: 'sit' },
        { label: '깨워서 인사한다', value: 'wake' },
        { label: '방해하지 않고 돌아간다', value: 'leave' },
      ]);
      if (c === 'sit') { love(7); await talk(['한 시간쯤 지났을까. 그 애가 눈을 떴다.', ['her', '…언제부터 있었어?', 'blush'], '"방금." 거짓말이었다.']); }
      else if (c === 'wake') { love(3); await talk([['her', '어? 아… 침 흘렸어? 안 흘렸지?', 'surprise']]); }
      else { await talk(['조용히 돌아 나왔다. 그날 저녁은 왠지 다운로드가 손에 잡히지 않았다.']); }
    }

    if (t === 19) { // 중2 2학기 — 구하기 힘든 파일
      const cands = WORKS.filter(w => T.likes(w) && w.appear <= dateOf(t).ym);
      const want = pick(cands.length ? cands : WORKS);
      const have = owned(st, want.id) > 0;
      await talk([['her', `있잖아… 혹시 「${want.title}」 구할 수 있어? 아무리 찾아도 없더라.`, 'sad']]);
      const opts = [];
      if (have) opts.push({ label: '"그거 나한테 있어!"', value: 'have' });
      opts.push({ label: '오늘 밤새 받아 준다', value: 'night', note: st.net === 'adsl' ? '엄마 인내심 하락' : '모뎀이라 오래 걸린다' });
      opts.push({ label: '동호회 형에게 부탁한다', value: 'club', note: st.stats.club ? '' : '동호회 활동을 해 본 적이 없다' });
      opts.push({ label: '"같이 찾아보자."', value: 'together' });
      const c = await UI.choose('', opts);
      if (c === 'have') { love(15); await talk([['her', '진짜?! 역시 너는…', 'blush'], '"역시 너는" 다음 말은 끝내 듣지 못했다. 그래도 충분했다.']); }
      else if (c === 'night') {
        mom(-12);
        const n = receiveDisc(st, want, 'download');
        love(st.net === 'adsl' ? 13 : 8);
        await talk([`그날 밤, 엄마 눈을 피해 「${want.title}」을 받았다. (${n}${TYPES[want.type].unit})`, ['her', '밤샜어? 눈이 빨개. …고마워.', 'blush']]);
      } else if (c === 'club') {
        if (st.stats.club) { love(11); receiveDisc(st, want, 'trade'); await talk([['hyung', '그거? 내 하드에 있지. 구워서 보내 줄게. 여자애 주는 거냐? ㅋㅋ', 'smile'], ['her', '와… 너 아는 사람 많구나.', 'smile']]); }
        else { love(2); await talk(['동호회에 가입만 해 두고 글을 쓴 적이 없어서… 아무도 답해 주지 않았다.', ['her', '괜찮아. 없으면 말지 뭐.', 'sad']]); }
      } else { love(10); await talk(['방과 후, 둘이서 PC방 구석에 나란히 앉아 검색창에 제목을 쳤다.', '결국 찾지는 못했지만, 그날 PC방 요금은 하나도 아깝지 않았다.']); }
    }

    if (t === 21) { // 2001 크리스마스
      if (st.love >= 35) {
        await talk([['her', '이번에 해리 포터 영화 개봉했대. …보러 갈 사람?', 'blush']]);
        const c = await UI.choose('', [
          { label: '"내가 표 살게. 같이 가자."', value: 'go', note: `영화표 두 장과 팝콘 ${won(14000)}`, disabled: st.money < 14000 },
          { label: '"비디오 나오면 같이 보자."', value: 'later' },
        ]);
        if (c === 'go') { st.money -= 14000; love(10); Sound.sfx('coin'); await talk(['종로의 극장. 팝콘 한 통을 나눠 먹었다.', ['her', '나중에 이거 파일 생기면 나도 줘. 또 볼래.', 'smile']]); }
        else { love(2); await talk([['her', '…응, 그래.', 'sad']]); }
      }
      if (st.love >= 40) {
        await talk(['12월 24일. 사물함에 작은 봉투가 들어 있었다.', ['her', '(카드) 올해도 고마웠어. 내년에도 재밌는 거 알려 줘. -' + HG(), 'blush']]);
        const c = await UI.choose('', [
          { label: '작은 선물과 함께 답장을 쓴다', value: 'gift', note: `문구점 선물 ${won(8000)}`, disabled: st.money < 8000 },
          { label: '답장을 쓴다', value: 'reply' },
          { label: '고맙다고 말로 한다', value: 'say' },
        ]);
        if (c === 'gift') st.money -= 8000;
        love(c === 'gift' ? 10 : c === 'reply' ? 7 : 4);
      } else {
        await talk(['12월 24일. 거리마다 캐럴이 흘렀다. 나는 방에서 받은 파일들을 정리했다.']);
      }
    }

    if (t === 23) { // 밸런타인
      if (st.love >= 50) {
        await talk(['2월 14일. 하굣길에 ' + josa(HER(), '이/가') + ' 무언가를 내밀었다.', ['her', '의, 의리야. 반 애들 다 줬어.', 'blush'], '반 애들 다 줬다는 초콜릿은, 내 것만 리본이 달려 있었다.']);
        love(5);
      } else {
        await talk(['2월 14일. 초콜릿은 없었다. 대신 하드디스크에 새 파일이 하나 늘었다.']);
      }
    }

    if (t === 26) { // 중3 1학기 — 생일
      await talk([['friend', `야, 다음 주 ${HG()} 생일이래. 너 알고 있었냐?`, 'surprise'], '알고 있었다. 달력에 몰래 표시까지 해 두었으니까.']);
      const how = await UI.choose('생일 선물은…', [
        { label: '내 컬렉션에서 골라 담아 준다', value: 'col' },
        { label: '선물 가게에서 산다', value: 'buy', note: `오르골 ${won(15000)}`, disabled: st.money < 15000 },
        { label: '둘 다', value: 'both', note: `오르골 ${won(15000)} + 컬렉션`, disabled: st.money < 15000 },
      ]);
      if (how === 'buy' || how === 'both') {
        st.money -= 15000; love(8); Sound.sfx('coin');
        await talk(['오르골을 샀다. 태엽을 감으면 그 시절 유행하던 노래가 흘러나왔다.', ['her', '…예쁘다. 고마워.', 'blush']]);
      }
      const g = how === 'buy' ? null : await giveFromCollection('생일 선물', '무엇을 담아서 선물할까?');
      if (how === 'buy') { /* 오르골만 */ }
      else if (!g && how === 'col') { love(2); await talk(['결국 문방구에서 산 볼펜 한 자루를 건넸다.', ['her', '고마워. 잘 쓸게.', 'smile']]); }
      else {
        love((g.liked ? 15 : 7) + (g.usb ? 4 : 0));
        await talk([
          g.usb ? `USB 메모리에 「${g.w.title}」을 담고, 메모장 파일로 짧은 생일 축하 글을 넣었다.` : `CD에 「${g.w.title}」을 굽고, 매직으로 정성껏 제목을 썼다.`,
          g.liked ? ['her', '…이거, 내가 제일 좋아하는 거야. 말한 적 없는데.', 'blush'] : ['her', '와, 직접 만든 거야? 고마워.', 'smile'],
        ]);
      }
    }

    if (t === 27) { // 2002 월드컵
      await talk(['2002년 6월. 온 나라가 붉게 물들었다. 대~한민국!', ['friend', '야, 오늘 광장 가자! 다들 간대!', 'smile']]);
      const c = await UI.choose('', [
        { label: `${HG()}에게 같이 가자고 한다`, value: 'her' },
        { label: '친구들과 간다', value: 'friends' },
        { label: '집에서 경기 보면서 다운로드를 돌린다', value: 'home' },
      ]);
      if (c === 'her') {
        const ok = st.love >= 40;
        love(ok ? 13 : 6);
        if (ok && st.money >= 10000) {
          const t2 = await UI.choose('…좋아. 빨간 티 있어?', [
            { label: '"Be the Reds" 티셔츠를 두 장 산다', value: 'buy', note: won(10000) },
            { label: '각자 집에 있는 빨간 옷을 입는다', value: 'no' },
          ], 'her', 'blush');
          if (t2 === 'buy') { st.money -= 10000; love(4); Sound.sfx('coin'); }
        }
        await talk(ok
          ? [['her', '…좋아. 빨간 티 입고 갈게.', 'blush'], '골이 들어간 순간, 모르는 사람들과 얼싸안았다. 그리고 그 애와도.', '그해 여름은, 평생 잊지 못할 것이다.']
          : [['her', '아… 나 친구들이랑 약속이 있어서. 미안.', 'sad'], '광장에서 멀리, 그 애의 빨간 티를 본 것 같았다.']);
      } else if (c === 'friends') { friend(15); await talk(['목이 쉬도록 소리 질렀다. 태식이는 집에 가는 버스에서 울었다.']); }
      else { mom(5); st.flags.worldcupHome = true; const av = availableWorks(st).filter(w => w.type === 'anime'); if (av.length) { const w = pick(av); const r = doDownload(st, [w.id], SPEED[st.net]); await talk([`창밖의 함성을 들으며 「${w.title}」을 받았다. (${r.res[0] ? r.res[0].got : 0}화)`, '…조금 외로웠다.']); } }
    }

    if (t === 32) { // 중3 2학기 — 고입 준비
      await talk(['11월. 고입 연합고사가 코앞이었다. 다들 말수가 줄었다.', `${josa(HER(), '은/는')} 요즘 쉬는 시간에도 문제집만 풀었다.`]);
      const c = await UI.choose('', [
        { label: '힘내라고 무언가를 담아 준다', value: 'gift' },
        { label: '쪽지만 건넨다: "시험 끝나면 같이 보자"', value: 'note' },
        { label: '방해하지 않는다', value: 'none' },
      ]);
      if (c === 'gift') {
        const g = await giveFromCollection('응원 선물', '시험 끝나고 볼 만한 걸 골라 주자.');
        if (g) { love(g.liked ? 12 : 6); await say('her', '시험 끝나면 이것부터 볼게. 약속.', 'smile'); }
      } else if (c === 'note') { love(8); await talk([['her', '(쪽지 뒷면) 응. 꼭.', 'blush']]); }
      else { love(1); }
    }

    // 친구의 한마디 (호감 힌트)
    if (t === 17 || t === 29) {
      const L = st.love;
      const line = L >= 60 ? `야, ${josa(HG(), '이/가')} 너 얘기할 때 표정이 다르더라. 알고 있냐?`
        : L >= 35 ? `${josa(HG(), '이/가')} 너 뭐 모으는지 물어보더라. 신기하지?`
          : `${HG()}? 걔 요즘 너랑 말 별로 안 하던데. 무슨 일 있었냐?`;
      await say('friend', line, L >= 35 ? 'smile' : 'normal');
    }
  }

  // ---------- 졸업식 ----------
  async function graduation() {
    const st = s();
    UI.setScene('room');
    st.screen = 'off';
    await talk(['2003년 2월. 졸업식 날 아침. 학교 가는 길에 꽃집이 문을 열고 있었다.']);
    if (st.money >= 10000) {
      const f = await UI.choose(`가진 돈 ${won(st.money)}`, [
        { label: '작은 꽃다발을 산다', value: 'y', note: won(10000) },
        { label: '그냥 간다', value: 'n' },
      ]);
      if (f === 'y') { st.money -= 10000; love(4); Sound.sfx('coin'); await talk(['프리지어 한 다발. 노란색이 그 애한테 어울릴 것 같았다.']); }
    }
    await talk([
      '2003년 2월. 졸업식.',
      '교복 위로 밀가루가 날렸고, 누군가는 울었고, 누군가는 웃었다.',
      `3년 동안 내 방에는 CD가 ${st.stats.discs}장 쌓였다.`,
      `운동장 끝, 플라타너스 아래에 ${josa(HER(), '이/가')} 서 있었다.`,
      ['her', `${N()}. …졸업 축하해.`, st.love >= 50 ? 'blush' : 'smile'],
    ]);
    const c = await UI.choose('마지막으로 할 말은…', [
      { label: '"좋아해. 처음 짝꿍이었을 때부터."', value: 'confess' },
      { label: '밤새 쓴 편지를 건넨다', value: 'letter' },
      { label: '"…고등학교 가서도 잘 지내."', value: 'silent' },
    ]);
    st.flags.gradChoice = c;
    if (c === 'confess') {
      if (st.love >= 90) await talk([['her', '…바보. 나도야. 그걸 이제 말하면 어떡해.', 'blush'], '그 애가 웃으면서 울었다. 나도 그랬던 것 같다.']);
      else if (st.love >= 60) await talk([['her', '…고마워. 근데 나, 아직 잘 모르겠어. 우리 계속 연락하자. 응?', 'sad']]);
      else await talk([['her', '아… 미안. 나는 너를 좋은 친구라고 생각했어.', 'sad'], '봄바람이 차가웠다.']);
    } else if (c === 'letter') {
      await talk(st.love >= 60
        ? [['her', '편지? …집에 가서 읽을게. 나도 답장 쓸게. 꼭.', 'blush']]
        : [['her', '아, 고마워. 잘 읽을게.', 'smile']]);
    } else {
      await talk([['her', '응. 너도.', st.love >= 50 ? 'sad' : 'smile'], '그 말이 마지막이었다. 그때는 몰랐다.']);
    }
  }

  // ---------- 엔딩 ----------
  const ENDINGS = {
    legend: { title: '전설의 수집왕', lines: [
      '2026년. 나는 서른아홉이 되었다.',
      '거실 한쪽 벽은 NAS와 하드디스크, 그리고 3년 동안 구운 CD들이 차지하고 있다.',
      '사람들은 내 개인 아카이브를 "인터넷 고고학 박물관"이라고 부른다.',
      '사라진 사이트, 끊긴 자막, 더는 구할 수 없는 파일들. 누군가 찾으면, 나는 꺼내 준다.',
      '그 시절 모뎀 소리를 들으며 결심했던 대로. 나는 수집왕이 되었다.'] },
    it: { title: 'IT 회사 개발자', lines: [
      '2026년. 나는 판교의 IT 회사에서 서버를 만든다.',
      '하루에 몇 페타바이트가 오가는 시스템을 설계하면서, 가끔 56k 모뎀을 생각한다.',
      '하룻밤에 150메가. 그 느림이, 나를 이 일로 데려왔다.',
      '책상 서랍에는 아직도 그때 구운 CD 한 장이 있다. 「메카·SF」라고 매직으로 쓴.'] },
    reporter: { title: '기자', lines: [
      '2026년. 나는 탐사 보도 기자가 되었다.',
      '흩어진 자료를 모으고, 날짜를 맞추고, 빠진 조각을 찾는 일.',
      '생각해 보면 열네 살 때부터 하던 일이었다. 김전일과 코난을 보면서.',
      '취재 파일 폴더 이름은 여전히 "C:\\내 컬렉션"이다.'] },
    webtoon: { title: '웹툰 작가', lines: [
      '2026년. 나는 웹툰 작가가 되었다.',
      '중학생 때 만화책을 한 장 한 장 스캔하던 손이, 이제는 펜 태블릿을 쥔다.',
      '스캔 각도가 삐뚤어질 때마다 다시 하던 그 집요함이 내 연재를 지탱한다.',
      '첫 단행본이 나온 날, 나는 그것을 스캔하는 대신 책장에 꽂았다.'] },
    gamedev: { title: '게임 개발자', lines: [
      '2026년. 나는 작은 인디 게임 스튜디오를 운영한다.',
      '도트를 찍고, 코드를 짜고, 밤을 샌다. 중학생 때처럼.',
      '최근작은 2000년대 PC방을 배경으로 한 게임이다. 리뷰에 이런 말이 있었다.',
      '"만든 사람은 분명 그 시절 CD를 구웠던 사람이다." …정답.'] },
    yongsan: { title: '용산 가게 사장', lines: [
      '2026년. 나는 용산 전자상가에서 작은 가게를 한다.',
      '레트로 PC와 부품을 고치고, 중고 CD를 사고판다. 장사는 그럭저럭이다.',
      '하지만 가게는 늘 북적인다. 옛날 얘기를 하러 오는 사람들로.',
      '그때 나에게 공CD를 팔던 사장님처럼, 나도 누군가에게 "또 왔냐" 하고 웃는다.'] },
    teacher: { title: '방과후 교사', lines: [
      '2026년. 나는 초등학교 방과후 코딩 교실 선생님이다.',
      '아이들은 USB 메모리가 왜 귀중품이었는지 이해하지 못한다.',
      '"선생님 때는 친구한테 CD를 구워 줬어." 아이들이 웃는다.',
      '나눠 주는 기쁨을, 나는 그때 배웠다. 지금도 그 일을 하고 있다.'] },
    radio: { title: '라디오 PD', lines: [
      '2026년. 나는 심야 라디오 PD가 되었다.',
      '사연과 노래를 고르는 일. 누군가의 밤에 무언가를 건네는 일.',
      '가끔 그 시절 노래를 튼다. 그러면 꼭 문자가 온다. "중학교 때 이 노래 CD로 구워 들었어요."',
      '나도요. 속으로 대답한다.'] },
    fanclub: { title: '공식 팬클럽 회장', lines: [
      '2026년. 나는 한 작품의 공식 팬클럽 회장이다.',
      '20년 넘게 한 우물만 팠다. 설정집 오탈자까지 외운다.',
      '작가님 사인회에서 내 이름을 기억해 주셨을 때, 열네 살의 내가 울었다.',
      '끝까지 모은다는 것. 그건 끝까지 사랑한다는 뜻이었다.'] },
    novelist: { title: '판타지 소설가', lines: [
      '2026년. 나는 판타지 소설을 쓴다.',
      '중학생 때 TXT 파일로 밤새 읽던 이야기들이, 내 안에서 다른 세계가 되었다.',
      '완결까지 쓰는 작가로 유명하다. 연재를 멈춘 적이 한 번도 없다.',
      '완결되지 않는 이야기가 얼마나 슬픈지, 나는 너무 잘 아니까.'] },
    editor: { title: '출판 편집자', lines: [
      '2026년. 나는 출판사 편집자가 되었다.',
      '원고를 끝까지 읽고, 시리즈를 끝까지 책임지는 사람.',
      '책장에는 내가 만든 책과, 그 시절 끝까지 모은 파일 목록이 나란히 있다.',
      '완결 도장을 찍을 때마다, 3년 동안 별표를 모으던 그 방이 떠오른다.'] },
    ordinary: { title: '평범한 직장인', lines: [
      '2026년. 나는 평범한 회사원이 되었다.',
      '출근하고, 일하고, 퇴근한다. 컬렉션 같은 건 이제 없다.',
      '그래도 가끔, 퇴근길 지하철에서 그 시절 노래를 찾아 듣는다.',
      '삐- 삐리리- 치이익-. 그 소리가 그리워질 때면.'] },
  };

  const ROMANCE = {
    together: { title: '함께', lines: h => [
      `그리고 지금, 내 옆에는 ${josa(h, '이/가')} 있다.`,
      '우리는 결혼하면서 서로의 하드디스크를 합쳤다. 겹치는 파일이 생각보다 많아서 한참 웃었다.',
      `"이거 기억나? 네가 처음 구워 준 CD." ${josa(h, '이/가')} 낡은 CD 케이스를 흔든다.`,
      '그 CD는 아직도 잘 읽힌다.'] },
    letter: { title: '그 시절의 편지', lines: h => [
      `${josa(h, '와/과')}는 다른 고등학교에 갔고, 그 뒤로는 자주 보지 못했다.`,
      '그래도 우리는 연락을 끊지 않았다. 이메일에서 메신저로, 메신저에서 단톡방으로.',
      `해마다 생일이면 ${josa(h, '은/는')} 노래 한 곡을 보낸다. 그 시절 우리가 함께 듣던.`,
      '사랑이었는지 우정이었는지, 이제는 중요하지 않다.'] },
    reunion: { title: '다시 만남', lines: h => [
      '졸업 후 20년 동안, 그 애를 잊고 살았다.',
      '그러다 작년, 중고 거래 앱에서 옛날 CD를 사려고 약속 장소에 나갔다.',
      `거기 ${josa(h, '이/가')} 서 있었다. "…${N()}? 설마 이 CD, 너야?"`,
      '우리는 카페에 앉아 세 시간 동안 그 시절 이야기를 했다. 다음 주에 또 만나기로 했다.'] },
    memory: { title: '기억 속의 그 아이', lines: h => [
      `${josa(h, '이/가')} 어떻게 지내는지 나는 모른다.`,
      '가끔 필통에 스티커를 붙인 아이들을 보면, 그 시절 짝꿍이 생각난다.',
      '"…그거, 애니 맞지?"',
      '그 질문에, 나는 제대로 대답했던가.'] },
  };

  async function ending() {
    const st = s();
    const eKey = decideEnding(st), rKey = decideRomance(st);
    const E = ENDINGS[eKey], Rm = ROMANCE[rKey];
    const tr = computeTraits(st);
    Meta.addEnding(eKey, rKey);
    UI.setScene('future');
    Sound.bgm('ending');
    st.screen = 'desktop';
    await talk(['그리고 시간이 흘렀다.']);
    await UI.notice('성향 결과', [
      `최종 성향: ${traitLabel(tr)}`,
      `모은 작품: ${tr.ownedCount}개 (완결 ${tr.completed}개)`,
      `구운 CD: ${st.stats.discs}장 · 포맷 ${st.stats.formats}번`,
    ], '2026년으로');
    await UI.notice('엔딩', [UI.h('h2', { class: 'ending-title' }, E.title)], '보기');
    await talk(E.lines);
    await UI.notice('에필로그', [UI.h('h2', { class: 'ending-title' }, Rm.title)], '보기');
    await talk(Rm.lines(HER()));
    return { eKey, rKey, E, Rm };
  }

  return { prologue, monthStart, monthEnd, graduation, ending, ENDINGS, ROMANCE, giveFromCollection };
})();
