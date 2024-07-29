#include <assert.h>
#include <limits.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cssg_ctype.h"
#include "buffer.h"

/* Used as default value for cssg_strbuf->ptr so that people can always
 * assume ptr is non-NULL and zero terminated even for new cssg_strbufs.
 */
unsigned char cssg_strbuf__initbuf[1];

#ifndef MIN
#define MIN(x, y) ((x < y) ? x : y)
#endif

void cssg_strbuf_init(cssg_mem *mem, cssg_strbuf *buf,
                       bufsize_t initial_size) {
  buf->mem = mem;
  buf->asize = 0;
  buf->size = 0;
  buf->ptr = cssg_strbuf__initbuf;

  if (initial_size > 0)
    cssg_strbuf_grow(buf, initial_size);
}

static inline void S_strbuf_grow_by(cssg_strbuf *buf, bufsize_t add) {
  cssg_strbuf_grow(buf, buf->size + add);
}

void cssg_strbuf_grow(cssg_strbuf *buf, bufsize_t target_size) {
  assert(target_size > 0);

  if (target_size < buf->asize)
    return;

  if (target_size > (bufsize_t)(INT32_MAX / 2)) {
    fprintf(stderr,
      "[cssg] cssg_strbuf_grow requests buffer with size > %d, aborting\n",
         (INT32_MAX / 2));
    abort();
  }

  /* Oversize the buffer by 50% to guarantee amortized linear time
   * complexity on append operations. */
  bufsize_t new_size = target_size + target_size / 2;
  new_size += 1;
  new_size = (new_size + 7) & ~7;

  buf->ptr = (unsigned char *)buf->mem->realloc(buf->asize ? buf->ptr : NULL,
                                                new_size);
  buf->asize = new_size;
}

void cssg_strbuf_free(cssg_strbuf *buf) {
  if (!buf)
    return;

  if (buf->ptr != cssg_strbuf__initbuf)
    buf->mem->free(buf->ptr);

  cssg_strbuf_init(buf->mem, buf, 0);
}

void cssg_strbuf_clear(cssg_strbuf *buf) {
  buf->size = 0;

  if (buf->asize > 0)
    buf->ptr[0] = '\0';
}

void cssg_strbuf_set(cssg_strbuf *buf, const unsigned char *data,
                      bufsize_t len) {
  if (len <= 0 || data == NULL) {
    cssg_strbuf_clear(buf);
  } else {
    if (data != buf->ptr) {
      if (len >= buf->asize)
        cssg_strbuf_grow(buf, len);
      memmove(buf->ptr, data, len);
    }
    buf->size = len;
    buf->ptr[buf->size] = '\0';
  }
}

void cssg_strbuf_putc(cssg_strbuf *buf, int c) {
  S_strbuf_grow_by(buf, 1);
  buf->ptr[buf->size++] = (unsigned char)(c & 0xFF);
  buf->ptr[buf->size] = '\0';
}

void cssg_strbuf_put(cssg_strbuf *buf, const unsigned char *data,
                      bufsize_t len) {
  if (len <= 0)
    return;

  S_strbuf_grow_by(buf, len);
  memmove(buf->ptr + buf->size, data, len);
  buf->size += len;
  buf->ptr[buf->size] = '\0';
}

void cssg_strbuf_puts(cssg_strbuf *buf, const char *string) {
  cssg_strbuf_put(buf, (const unsigned char *)string, (bufsize_t)strlen(string));
}

unsigned char *cssg_strbuf_detach(cssg_strbuf *buf) {
  unsigned char *data = buf->ptr;

  if (buf->asize == 0) {
    /* return an empty string */
    return (unsigned char *)buf->mem->calloc(1, 1);
  }

  cssg_strbuf_init(buf->mem, buf, 0);
  return data;
}

void cssg_strbuf_truncate(cssg_strbuf *buf, bufsize_t len) {
  if (len < 0)
    len = 0;

  if (len < buf->size) {
    buf->size = len;
    buf->ptr[buf->size] = '\0';
  }
}

void cssg_strbuf_drop(cssg_strbuf *buf, bufsize_t n) {
  if (n > 0) {
    if (n > buf->size)
      n = buf->size;
    buf->size = buf->size - n;
    if (buf->size)
      memmove(buf->ptr, buf->ptr + n, buf->size);

    buf->ptr[buf->size] = '\0';
  }
}

void cssg_strbuf_rtrim(cssg_strbuf *buf) {
  if (!buf->size)
    return;

  while (buf->size > 0) {
    if (!cssg_isspace(buf->ptr[buf->size - 1]))
      break;

    buf->size--;
  }

  buf->ptr[buf->size] = '\0';
}

void cssg_strbuf_trim(cssg_strbuf *buf) {
  bufsize_t i = 0;

  if (!buf->size)
    return;

  while (i < buf->size && cssg_isspace(buf->ptr[i]))
    i++;

  cssg_strbuf_drop(buf, i);

  cssg_strbuf_rtrim(buf);
}

// Destructively modify string, collapsing consecutive
// space and newline characters into a single space.
void cssg_strbuf_normalize_whitespace(cssg_strbuf *s) {
  bool last_char_was_space = false;
  bufsize_t r, w;

  for (r = 0, w = 0; r < s->size; ++r) {
    if (cssg_isspace(s->ptr[r])) {
      if (!last_char_was_space) {
        s->ptr[w++] = ' ';
        last_char_was_space = true;
      }
    } else {
      s->ptr[w++] = s->ptr[r];
      last_char_was_space = false;
    }
  }

  cssg_strbuf_truncate(s, w);
}

// Destructively unescape a string: remove backslashes before punctuation chars.
void cssg_strbuf_unescape(cssg_strbuf *buf) {
  bufsize_t r, w;

  for (r = 0, w = 0; r < buf->size; ++r) {
    if (buf->ptr[r] == '\\' && cssg_ispunct(buf->ptr[r + 1]))
      r++;

    buf->ptr[w++] = buf->ptr[r];
  }

  cssg_strbuf_truncate(buf, w);
}
