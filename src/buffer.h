#ifndef CSSG_BUFFER_H
#define CSSG_BUFFER_H

#include <limits.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "cssg.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef int32_t bufsize_t;

typedef struct {
  cssg_mem *mem;
  unsigned char *ptr;
  bufsize_t asize, size;
} cssg_strbuf;

extern unsigned char cssg_strbuf__initbuf[];

#define CSSG_BUF_INIT(mem)                                                    \
  { mem, cssg_strbuf__initbuf, 0, 0 }

/**
 * Initialize a cssg_strbuf structure.
 *
 * For the cases where CSSG_BUF_INIT cannot be used to do static
 * initialization.
 */
void cssg_strbuf_init(cssg_mem *mem, cssg_strbuf *buf,
                       bufsize_t initial_size);

/**
 * Grow the buffer to hold at least `target_size` bytes.
 */
void cssg_strbuf_grow(cssg_strbuf *buf, bufsize_t target_size);

void cssg_strbuf_free(cssg_strbuf *buf);

unsigned char *cssg_strbuf_detach(cssg_strbuf *buf);

/*
static inline const char *cssg_strbuf_cstr(const cssg_strbuf *buf) {
 return (char *)buf->ptr;
}
*/

#define cssg_strbuf_at(buf, n) ((buf)->ptr[n])

void cssg_strbuf_set(cssg_strbuf *buf, const unsigned char *data,
                      bufsize_t len);
void cssg_strbuf_putc(cssg_strbuf *buf, int c);
void cssg_strbuf_put(cssg_strbuf *buf, const unsigned char *data,
                      bufsize_t len);
void cssg_strbuf_puts(cssg_strbuf *buf, const char *string);
void cssg_strbuf_clear(cssg_strbuf *buf);

void cssg_strbuf_drop(cssg_strbuf *buf, bufsize_t n);
void cssg_strbuf_truncate(cssg_strbuf *buf, bufsize_t len);
void cssg_strbuf_rtrim(cssg_strbuf *buf);
void cssg_strbuf_trim(cssg_strbuf *buf);
void cssg_strbuf_normalize_whitespace(cssg_strbuf *s);
void cssg_strbuf_unescape(cssg_strbuf *s);

#ifdef __cplusplus
}
#endif

#endif
