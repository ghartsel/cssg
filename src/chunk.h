#ifndef CSSG_CHUNK_H
#define CSSG_CHUNK_H

#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include "cssg.h"
#include "buffer.h"
#include "cssg_ctype.h"

#define CSSG_CHUNK_EMPTY                                                      \
  { NULL, 0 }

typedef struct {
  const unsigned char *data;
  bufsize_t len;
} cssg_chunk;

// NOLINTNEXTLINE(clang-diagnostic-unused-function)
static inline void cssg_chunk_free(cssg_chunk *c) {
  c->data = NULL;
  c->len = 0;
}

static inline void cssg_chunk_ltrim(cssg_chunk *c) {
  while (c->len && cssg_isspace(c->data[0])) {
    c->data++;
    c->len--;
  }
}

static inline void cssg_chunk_rtrim(cssg_chunk *c) {
  while (c->len > 0) {
    if (!cssg_isspace(c->data[c->len - 1]))
      break;

    c->len--;
  }
}

// NOLINTNEXTLINE(clang-diagnostic-unused-function)
static inline void cssg_chunk_trim(cssg_chunk *c) {
  cssg_chunk_ltrim(c);
  cssg_chunk_rtrim(c);
}

// NOLINTNEXTLINE(clang-diagnostic-unused-function)
static inline bufsize_t cssg_chunk_strchr(cssg_chunk *ch, int c,
                                           bufsize_t offset) {
  const unsigned char *p =
      (unsigned char *)memchr(ch->data + offset, c, ch->len - offset);
  return p ? (bufsize_t)(p - ch->data) : ch->len;
}

// NOLINTNEXTLINE(clang-diagnostic-unused-function)
static inline cssg_chunk cssg_chunk_literal(const char *data) {
  bufsize_t len = data ? (bufsize_t)strlen(data) : 0;
  cssg_chunk c = {(unsigned char *)data, len};
  return c;
}

// NOLINTNEXTLINE(clang-diagnostic-unused-function)
static inline cssg_chunk cssg_chunk_dup(const cssg_chunk *ch, bufsize_t pos,
                                          bufsize_t len) {
  cssg_chunk c = {ch->data + pos, len};
  return c;
}

#endif
