// 한글 도트 글꼴 (Galmuri11에서 게임에 쓰는 글자만 뽑은 것)
#pragma once

extern const int FONT_COUNT;
extern const int FONT_CELL_H;
extern const unsigned short FONT_CODES[];   // 유니코드, 오름차순
extern const unsigned char FONT_WIDTHS[];   // 글자 폭(다음 글자까지 거리)
extern const unsigned short FONT_BITS[];    // 글자마다 FONT_CELL_H줄, 줄당 16비트(왼쪽 비트가 왼쪽 픽셀)
