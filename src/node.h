#ifndef CSSG_NODE_H
#define CSSG_NODE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "cssg.h"
#include "buffer.h"

typedef struct {
  int marker_offset;
  int padding;
  int start;
  unsigned char list_type;
  unsigned char delimiter;
  unsigned char bullet_char;
  bool tight;
} cssg_list;

typedef struct {
  unsigned char *info;
  uint8_t fence_length;
  uint8_t fence_offset;
  unsigned char fence_char;
  int8_t fenced;
} cssg_code;

typedef struct {
  int internal_offset;
  int8_t level;
  bool setext;
} cssg_heading;

typedef struct {
  unsigned char *url;
  unsigned char *title;
} cssg_link;

typedef struct {
  unsigned char *on_enter;
  unsigned char *on_exit;
} cssg_custom;

enum cssg_node__internal_flags {
  CSSG_NODE__OPEN = (1 << 0),
  CSSG_NODE__LAST_LINE_BLANK = (1 << 1),
  CSSG_NODE__LAST_LINE_CHECKED = (1 << 2),
  CSSG_NODE__LIST_LAST_LINE_BLANK = (1 << 3),
};

struct cssg_node {
  cssg_mem *mem;

  struct cssg_node *next;
  struct cssg_node *prev;
  struct cssg_node *parent;
  struct cssg_node *first_child;
  struct cssg_node *last_child;

  void *user_data;

  unsigned char *data;
  bufsize_t len;

  int start_line;
  int start_column;
  int end_line;
  int end_column;
  uint16_t type;
  uint16_t flags;

  union {
    cssg_list list;
    cssg_code code;
    cssg_heading heading;
    cssg_link link;
    cssg_custom custom;
    int html_block_type;
  } as;
};

CSSG_EXPORT int cssg_node_check(cssg_node *node, FILE *out);

#ifdef __cplusplus
}
#endif

#endif
