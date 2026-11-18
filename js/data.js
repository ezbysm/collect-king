// 수집왕 — 게임 데이터 (작품, 장르, 파일 종류, 인물)
'use strict';

const GENRES = {
  hot:  { name: '열혈',    trait: '열혈파',   color: '#d8483c' },
  love: { name: '순정',    trait: '순정파',   color: '#e889b0' },
  sf:   { name: '메카·SF', trait: '메카·SF광', color: '#4f8fd8' },
  mys:  { name: '미스터리', trait: '탐정파',   color: '#7a5bb0' },
  fan:  { name: '판타지',  trait: '몽상가',   color: '#4caf6a' },
  life: { name: '일상',    trait: '일상파',   color: '#e8b64a' },
};

// size: 수집 단위 1개의 용량(MB), weight: 성향 점수 가중치(단위당)
const TYPES = {
  anime: { name: '애니',  ext: 'AVI', unit: '화', size: 120, weight: 1.5, folder: '애니' },
  manga: { name: '만화',  ext: 'JPG', unit: '권', size: 30,  weight: 1.0, folder: '만화' },
  novel: { name: '소설',  ext: 'TXT', unit: '권', size: 1,   weight: 1.5, folder: '소설' },
  movie: { name: '영화',  ext: 'AVI', unit: 'CD', size: 700, weight: 8.0, folder: '영화' },
  music: { name: '음악',  ext: 'MP3', unit: '장', size: 60,  weight: 4.0, folder: '음악' },
  game:  { name: '게임',  ext: 'ISO', unit: 'CD', size: 650, weight: 7.0, folder: '게임' },
};

// [id, type, 제목, 장르들, 등장(YYYYMM), 단위 수]
const WORK_ROWS = [
  // 애니메이션
  ['a_onepiece', 'anime', '원피스', ['hot', 'fan'], 200003, 40],
  ['a_hunter', 'anime', '헌터×헌터', ['hot', 'fan'], 200003, 30],
  ['a_bebop', 'anime', '카우보이 비밥', ['sf', 'hot'], 200003, 26],
  ['a_eva', 'anime', '신세기 에반게리온', ['sf'], 200003, 26],
  ['a_sakura', 'anime', '카드캡터 체리', ['love', 'fan'], 200003, 35],
  ['a_conan', 'anime', '명탐정 코난', ['mys'], 200003, 40],
  ['a_kindaichi', 'anime', '소년탐정 김전일', ['mys'], 200003, 30],
  ['a_initiald', 'anime', '이니셜 D', ['hot'], 200003, 26],
  ['a_kareshi', 'anime', '그 남자 그 여자의 사정', ['love', 'life'], 200003, 26],
  ['a_shinchan', 'anime', '짱구는 못말려', ['life'], 200003, 30],
  ['a_lovehina', 'anime', '러브히나', ['love', 'life'], 200005, 25],
  ['a_digimon', 'anime', '디지몬 어드벤처 02', ['fan', 'hot'], 200005, 30],
  ['a_yugioh', 'anime', '유희왕 듀얼몬스터즈', ['hot'], 200005, 30],
  ['a_inuyasha', 'anime', '이누야샤', ['fan', 'love'], 200011, 35],
  ['a_shaman', 'anime', '샤먼킹', ['hot', 'fan'], 200107, 30],
  ['a_fruits', 'anime', '후르츠 바스켓', ['love'], 200107, 26],
  ['a_hikaru', 'anime', '히카루의 바둑', ['hot', 'life'], 200110, 30],
  ['a_tennis', 'anime', '테니스의 왕자', ['hot'], 200110, 30],
  ['a_fmp', 'anime', '풀 메탈 패닉!', ['sf', 'life'], 200201, 24],
  ['a_hoshi', 'anime', '별의 목소리', ['sf', 'love'], 200202, 1],
  ['a_azumanga', 'anime', '아즈망가 대왕', ['life'], 200204, 26],
  ['a_12kingdoms', 'anime', '십이국기', ['fan'], 200204, 26],
  ['a_seed', 'anime', '기동전사 건담 SEED', ['sf'], 200210, 20],
  ['a_gits', 'anime', '공각기동대 S.A.C.', ['sf', 'mys'], 200210, 20],
  ['a_naruto', 'anime', '나루토', ['hot'], 200210, 20],

  // 만화 (스캔본)
  ['m_slamdunk', 'manga', '슬램덩크', ['hot'], 200003, 31],
  ['m_dragonball', 'manga', '드래곤볼', ['hot', 'fan'], 200003, 42],
  ['m_onepiece', 'manga', '원피스', ['hot', 'fan'], 200003, 25],
  ['m_hunter', 'manga', '헌터×헌터', ['hot', 'fan'], 200003, 14],
  ['m_conan', 'manga', '명탐정 코난', ['mys'], 200003, 35],
  ['m_hanadan', 'manga', '꽃보다 남자', ['love'], 200003, 36],
  ['m_kenshin', 'manga', '바람의 검심', ['hot'], 200003, 28],
  ['m_berserk', 'manga', '베르세르크', ['fan'], 200003, 20],
  ['m_monster', 'manga', '몬스터', ['mys'], 200003, 18],
  ['m_vagabond', 'manga', '배가본드', ['hot'], 200003, 12],
  ['m_lovehina', 'manga', '러브히나', ['love', 'life'], 200003, 14],
  ['m_yeolhyeol', 'manga', '열혈강호', ['hot', 'fan'], 200003, 25],
  ['m_fullhouse', 'manga', '풀하우스', ['love', 'life'], 200003, 16],
  ['m_priest', 'manga', '프리스트', ['fan'], 200003, 12],
  ['m_island', 'manga', '아일랜드', ['fan', 'mys'], 200003, 7],
  ['m_jjang', 'manga', '짱', ['hot', 'life'], 200003, 25],
  ['m_20century', 'manga', '20세기 소년', ['mys', 'sf'], 200006, 12],
  ['m_naruto', 'manga', '나루토', ['hot'], 200009, 15],
  ['m_hikaru', 'manga', '히카루의 바둑', ['hot', 'life'], 200009, 18],
  ['m_tennis', 'manga', '테니스의 왕자', ['hot'], 200103, 15],
  ['m_fruits', 'manga', '후르츠 바스켓', ['love'], 200103, 10],
  ['m_amhaeng', 'manga', '신암행어사', ['fan', 'hot'], 200109, 6],
  ['m_goong', 'manga', '궁', ['love'], 200209, 3],

  // 소설 (TXT)
  ['n_dragonraja', 'novel', '드래곤 라자', ['fan'], 200003, 12],
  ['n_futurewalker', 'novel', '퓨처 워커', ['fan'], 200003, 7],
  ['n_sewol', 'novel', '세월의 돌', ['fan', 'love'], 200003, 9],
  ['n_toemarok', 'novel', '퇴마록', ['fan', 'mys'], 200003, 19],
  ['n_mukhyang', 'novel', '묵향', ['hot', 'fan'], 200003, 15],
  ['n_harrypotter', 'novel', '해리 포터 시리즈', ['fan'], 200003, 4],
  ['n_lotr', 'novel', '반지의 제왕', ['fan'], 200003, 6],
  ['n_alchemist', 'novel', '연금술사', ['life', 'fan'], 200103, 1],
  ['n_winterer', 'novel', '룬의 아이들: 윈터러', ['fan'], 200106, 7],
  ['n_geunom', 'novel', '그놈은 멋있었다', ['love', 'life'], 200109, 2],
  ['n_wolf', 'novel', '늑대의 유혹', ['love'], 200206, 2],

  // 영화 (AVI)
  ['v_matrix', 'movie', '매트릭스', ['sf'], 200003, 2],
  ['v_gladiator', 'movie', '글래디에이터', ['hot'], 200006, 2],
  ['v_donggam', 'movie', '동감', ['love', 'fan'], 200006, 1],
  ['v_jsa', 'movie', '공동경비구역 JSA', ['mys'], 200010, 2],
  ['v_crouching', 'movie', '와호장룡', ['hot', 'love'], 200012, 2],
  ['v_chingu', 'movie', '친구', ['hot'], 200104, 2],
  ['v_shrek', 'movie', '슈렉', ['life', 'fan'], 200107, 1],
  ['v_sassy', 'movie', '엽기적인 그녀', ['love', 'life'], 200108, 2],
  ['v_hp1', 'movie', '해리 포터와 마법사의 돌', ['fan'], 200112, 2],
  ['v_lotr1', 'movie', '반지의 제왕: 반지 원정대', ['fan'], 200201, 2],
  ['v_monsters', 'movie', '몬스터 주식회사', ['life'], 200202, 1],
  ['v_jibeuro', 'movie', '집으로…', ['life'], 200204, 1],
  ['v_spiderman', 'movie', '스파이더맨', ['hot', 'sf'], 200205, 2],
  ['v_chihiro', 'movie', '센과 치히로의 행방불명', ['fan'], 200206, 2],
  ['v_minority', 'movie', '마이너리티 리포트', ['sf', 'mys'], 200207, 2],

  // 음악 (MP3 앨범)
  ['s_god', 'music', 'god 2집·3집', ['life', 'love'], 200003, 2],
  ['s_josungmo', 'music', '조성모 3집', ['love'], 200003, 1],
  ['s_finkl', 'music', '핑클', ['love', 'life'], 200003, 3],
  ['s_ses', 'music', 'S.E.S.', ['love', 'life'], 200003, 3],
  ['s_shinhwa', 'music', '신화', ['hot'], 200003, 2],
  ['s_xjapan', 'music', 'X JAPAN 베스트', ['hot'], 200003, 2],
  ['s_boa1', 'music', '보아 1집', ['love', 'life'], 200008, 1],
  ['s_seotaiji', 'music', '서태지 6집', ['hot'], 200009, 1],
  ['s_hot5', 'music', 'H.O.T. 5집', ['hot'], 200009, 1],
  ['s_tiger', 'music', '드렁큰 타이거', ['hot'], 200103, 2],
  ['s_utada', 'music', '우타다 히카루', ['love'], 200103, 2],
  ['s_ayu', 'music', '하마사키 아유미', ['love'], 200106, 2],
  ['s_ost', 'music', '애니메이션 OST 모음', ['fan'], 200109, 3],
  ['s_boa2', 'music', '보아 2집 No.1', ['love', 'life'], 200204, 1],

  // PC 게임 (ISO)
  ['g_starcraft', 'game', '스타크래프트: 브루드 워', ['sf'], 200003, 1],
  ['g_aoe2', 'game', '에이지 오브 엠파이어 2', ['hot'], 200003, 1],
  ['g_daehanghae', 'game', '대항해시대 4', ['hot', 'life'], 200003, 1],
  ['g_cs', 'game', '카운터 스트라이크', ['hot'], 200003, 1],
  ['g_samguk7', 'game', '삼국지 7', ['hot'], 200006, 1],
  ['g_diablo2', 'game', '디아블로 2', ['fan', 'hot'], 200007, 3],
  ['g_sims', 'game', '심즈', ['life'], 200009, 1],
  ['g_ra2', 'game', '레드 얼럿 2', ['sf'], 200011, 1],
  ['g_changse3', 'game', '창세기전 3 파트 2', ['fan', 'sf'], 200012, 2],
  ['g_whiteday', 'game', '화이트 데이', ['mys'], 200105, 1],
  ['g_lod', 'game', '디아블로 2: 파괴의 군주', ['fan', 'hot'], 200107, 1],
  ['g_maxpayne', 'game', '맥스 페인', ['mys', 'hot'], 200108, 1],
  ['g_magna', 'game', '마그나카르타', ['fan', 'love'], 200112, 2],
  ['g_arcturus', 'game', '악튜러스', ['fan', 'love'], 200203, 2],
  ['g_war3', 'game', '워크래프트 3', ['fan'], 200207, 1],
];

const WORKS = WORK_ROWS.map(([id, type, title, genres, appear, units]) => ({
  id, type, title, genres, appear, units, size: TYPES[type].size,
}));
const WORK_BY_ID = Object.fromEntries(WORKS.map(w => [w.id, w]));

// 히로인 이름 풀 (1987년생 여자아이에게 흔했던 이름 느낌)
const HEROINE_SURNAMES = ['김', '이', '박', '최', '정', '강', '조', '윤', '장', '한', '임', '오', '서', '신'];
const HEROINE_GIVEN = ['지은', '수진', '혜진', '민지', '은지', '지혜', '유진', '소연', '지영', '보람', '현정', '다은', '수정', '은영', '미영', '선영', '하나', '슬기', '나영', '혜원'];

// 히로인의 숨은 취향 (회차마다 랜덤)
const HEROINE_TASTES = {
  love:  { name: '순정 만화', hintItem: '꽃보다 남자 책받침', hintLine: '…순정 만화 같은 건 유치하다고들 하잖아.', likes: w => w.genres.includes('love') },
  fan:   { name: '판타지 소설', hintItem: '드래곤 라자 문구가 적힌 책갈피', hintLine: '…긴 이야기를 읽고 있으면, 다른 세계에 있는 것 같아.', likes: w => w.genres.includes('fan') },
  music: { name: '음악', hintItem: '우타다 히카루 사진 스티커', hintLine: '…이어폰 끼고 있으면 아무도 말 안 걸거든.', likes: w => w.type === 'music' },
};

// 인물
const CAST = {
  me:     { name: '나' },
  mom:    { name: '엄마', face: 'mom' },
  dad:    { name: '아빠', face: 'dad' },
  friend: { name: '태식', face: 'friend' },
  hyung:  { name: '동호회 형 (닉: 밤의제왕)', face: 'hyung' },
  owner:  { name: '용산 사장님', face: 'owner' },
  her:    { name: '', face: 'her' },
  narr:   { name: '' },
};

// 상점 (용산)
const SHOP = [
  { id: 'cd10', name: '공CD 10장 묶음', price: 9000, desc: '700MB CD-R. 굽기에 필요.', avail: s => true },
  { id: 'cd30', name: '공CD 30장 스핀들', price: 24000, desc: '케이크 통에 든 공CD. 많이 굽는 사람용.', avail: s => true },
  { id: 'lens', name: '렌즈 클리너', price: 5000, desc: 'CD 드라이브 상태 +30', avail: s => s.pc.burner },
  { id: 'repair', name: 'CD 드라이브 수리', price: 15000, desc: '고장 난 드라이브를 고친다.', avail: s => s.pc.burner && s.pc.driveBroken },
  { id: 'scanner', name: '중고 평판 스캐너', price: 45000, desc: '만화책 스캔 활동 해금.', avail: s => !s.pc.scanner && s.turn >= 10 },
  { id: 'usb', name: 'USB 메모리 128MB', price: 35000, desc: '파일을 들고 다닐 수 있다. 귀중품.', avail: s => !s.pc.usb && s.turn >= 24 },
];
