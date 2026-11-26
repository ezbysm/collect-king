// 자동 생성: gba/tools/make-data.js — 웹판 데이터에서 만든 표
#pragma once

#define NWORKS 113
#define NTYPES 6
#define NGENRES 6
#define NENDINGS 32

enum { T_ANIME, T_MANGA, T_NOVEL, T_MOVIE, T_MUSIC, T_GAME };
enum { G_HOT, G_LOVE, G_SF, G_MYS, G_FAN, G_LIFE };
enum { E_PROGAMER, E_STUNT, E_PE_TEACHER, E_ROCKER, E_VN_WRITER, E_DRAMA_PD, E_WEBNOVEL, E_LYRICIST, E_VFX, E_EDM, E_ESCAPE, E_DOCU, E_FORENSIC, E_PLANNER, E_ANIMATOR, E_COMPOSER, E_STREAMER, E_VLOGGER, E_BOOKSHOP, E_CAFE, E_LEGEND, E_IT, E_REPORTER, E_WEBTOON, E_GAMEDEV, E_YONGSAN, E_TEACHER, E_RADIO, E_FANCLUB, E_NOVELIST, E_EDITOR, E_ORDINARY };

typedef struct { const char *title; unsigned char type, g0, g1, appear, units; } Work;   // g1=255: 장르 하나
typedef struct { const char *name, *ext, *unit, *folder; unsigned short size, weight10; } TypeInfo;   // weight10: 성향 가중치 x10
typedef struct { const char *name, *trait; unsigned short color; } GenreInfo;
typedef struct { const char *title; unsigned char nlines; const char *lines[6]; } EndingInfo;
typedef struct { const char *name, *hint_item, *hint_line; } TasteInfo;

extern const Work WORKS[NWORKS];
extern const TypeInfo TYPES[NTYPES];
extern const GenreInfo GENRES[NGENRES];
extern const EndingInfo ENDINGS[NENDINGS];
extern const unsigned char ENDING_TABLE[NGENRES][4];   // [장르][매체: 0게임 1영상 2책 3음악]
extern const char *const HER_SURNAMES[]; extern const int N_HER_SURNAMES;
extern const char *const HER_GIVEN[]; extern const int N_HER_GIVEN;
extern const TasteInfo TASTES[3];   // 0 순정 만화, 1 판타지 소설, 2 음악
