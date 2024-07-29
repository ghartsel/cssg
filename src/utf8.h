#ifndef CSSG_UTF8_H
#define CSSG_UTF8_H

#include <stdint.h>
#include "buffer.h"

#ifdef __cplusplus
extern "C" {
#endif

void cssg_utf8proc_case_fold(cssg_strbuf *dest, const uint8_t *str,
                              bufsize_t len);
void cssg_utf8proc_encode_char(int32_t uc, cssg_strbuf *buf);
int cssg_utf8proc_iterate(const uint8_t *str, bufsize_t str_len, int32_t *dst);
void cssg_utf8proc_check(cssg_strbuf *dest, const uint8_t *line,
                          bufsize_t size);
int cssg_utf8proc_is_space(int32_t uc);
int cssg_utf8proc_is_punctuation_or_symbol(int32_t uc);

#ifdef __cplusplus
}
#endif

#endif
