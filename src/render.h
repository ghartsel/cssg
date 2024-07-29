#ifndef CSSG_RENDER_H
#define CSSG_RENDER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdlib.h>

#include "buffer.h"

typedef enum { LITERAL, NORMAL, TITLE, URL } cssg_escaping;

struct block_number {
  int number;
  struct block_number *parent;
};

struct cssg_renderer {
  int options;
  cssg_mem *mem;
  cssg_strbuf *buffer;
  cssg_strbuf *prefix;
  int column;
  int width;
  int need_cr;
  bufsize_t last_breakable;
  bool begin_line;
  bool begin_content;
  bool no_linebreaks;
  bool in_tight_list_item;
  struct block_number *block_number_in_list_item;
  void (*outc)(struct cssg_renderer *, cssg_escaping, int32_t, unsigned char);
  void (*cr)(struct cssg_renderer *);
  void (*blankline)(struct cssg_renderer *);
  void (*out)(struct cssg_renderer *, const char *, bool, cssg_escaping);
};

typedef struct cssg_renderer cssg_renderer;

void cssg_render_ascii(cssg_renderer *renderer, const char *s);

void cssg_render_code_point(cssg_renderer *renderer, uint32_t c);

char *cssg_render(cssg_node *root, int options, int width,
                   void (*outc)(cssg_renderer *, cssg_escaping, int32_t,
                                unsigned char),
                   int (*render_node)(cssg_renderer *renderer,
                                      cssg_node *node,
                                      cssg_event_type ev_type, int options));

#ifdef __cplusplus
}
#endif

#endif
