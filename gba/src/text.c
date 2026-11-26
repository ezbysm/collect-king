// 수집왕 GBA — 문장 만들기: 조사, 숫자, 돈, 용량
#include <stdarg.h>
#include "common.h"

// 돌려쓰는 버퍼 (동시에 여러 개를 쓸 수 있게)
static char pool[24][256];
static int pool_i;
static char *next_buf(void) { pool_i = (pool_i + 1) % 24; return pool[pool_i]; }

static int has_batchim(const char *word) {
  const char *p = word, *last = word;
  u32 cp = 0;
  while (*p) { last = p; cp = utf8_next(&p); }
  (void)last;
  return cp >= 0xAC00 && cp <= 0xD7A3 && (cp - 0xAC00) % 28 != 0;
}

static int put(char *dst, int at, const char *s) { while (*s && at < 254) dst[at++] = *s++; dst[at] = 0; return at; }

const char *josa(const char *word, const char *pair) {
  char *out = next_buf();
  int at = put(out, 0, word);
  const char *slash = pair;
  while (*slash && *slash != '/') slash++;
  if (has_batchim(word)) { int n = slash - pair; for (int i = 0; i < n && at < 254; i++) out[at++] = pair[i]; out[at] = 0; }
  else put(out, at, *slash ? slash + 1 : pair);
  return out;
}

const char *given_name(const char *full) {
  // 세 글자 이상이면 첫 글자(성)를 뺀다
  int n = 0; const char *p = full;
  while (*p) { utf8_next(&p); n++; }
  if (n < 3) return full;
  p = full; utf8_next(&p);
  char *out = next_buf(); put(out, 0, p); return out;
}

static int put_int(char *dst, int at, s32 v, int commas) {
  char tmp[16]; int n = 0, neg = v < 0;
  u32 u = neg ? -v : v;
  do { if (commas && n % 4 == 3) tmp[n++] = ','; tmp[n++] = '0' + u % 10; u /= 10; } while (u && n < 15);
  if (neg) dst[at++] = '-';
  while (n) dst[at++] = tmp[--n];
  dst[at] = 0;
  return at;
}

const char *wonstr(s32 n) { char *o = next_buf(); int at = put(o, 0, "₩"); put_int(o, at, n, 1); return o; }
const char *mbstr(s32 mb) {
  char *o = next_buf();
  if (mb >= 1000) { int at = put_int(o, 0, mb / 1000, 0); o[at++] = '.'; o[at++] = '0' + (mb % 1000) / 100; o[at] = 0; put(o, at, "GB"); }
  else { int at = put_int(o, 0, mb, 0); put(o, at, "MB"); }
  return o;
}

// %s 문자열, %d 정수, %w 원(₩1,000), %m 용량(MB/GB), %% 퍼센트
const char *fmt(const char *f, ...) {
  char *o = next_buf();
  int at = 0;
  va_list ap;
  va_start(ap, f);
  while (*f && at < 250) {
    if (*f != '%') { o[at++] = *f++; continue; }
    f++;
    switch (*f++) {
      case 's': at = put(o, at, va_arg(ap, const char *)); break;
      case 'd': at = put_int(o, at, va_arg(ap, int), 0); break;
      case 'w': at = put(o, at, wonstr(va_arg(ap, int))); break;
      case 'm': at = put(o, at, mbstr(va_arg(ap, int))); break;
      case '%': o[at++] = '%'; break;
    }
  }
  o[at] = 0;
  va_end(ap);
  return o;
}
