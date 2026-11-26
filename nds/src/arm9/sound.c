// 수집왕 NDS — 소리 요청: 실제 연주는 ARM7(src/arm7/sound7.c)이 화면 갱신마다 한다
#include "common.h"

int sound_on = 1;
void sound_init(void) { SHARED->bgm = BGM_NONE; SHARED->sound_on = (u8)sound_on; }
void sound_update(void) {}
void bgm(int id) { SHARED->bgm = (u8)id; }
void sfx(int id) {
  if (!sound_on) return;
  Shared *sh = SHARED;
  sh->sfx_id = (u8)id;
  sh->sfx_seq++;
}
void sound_toggle(void) { sound_on = !sound_on; SHARED->sound_on = (u8)sound_on; }
