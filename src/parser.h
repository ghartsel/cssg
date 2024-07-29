#ifndef CSSG_AST_H
#define CSSG_AST_H

#include <stdio.h>
#include "references.h"
#include "node.h"
#include "buffer.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_LINK_LABEL_LENGTH 1000

struct cssg_parser {
  struct cssg_mem *mem;
  struct cssg_reference_map *refmap;
  struct cssg_node *root;
  struct cssg_node *current;
  int line_number;
  bufsize_t offset;
  bufsize_t column;
  bufsize_t first_nonspace;
  bufsize_t first_nonspace_column;
  bufsize_t thematic_break_kill_pos;
  int indent;
  bool blank;
  bool partially_consumed_tab;
  cssg_strbuf curline;
  bufsize_t last_line_length;
  cssg_strbuf linebuf;
  cssg_strbuf content;
  int options;
  bool last_buffer_ended_with_cr;
  unsigned int total_size;
};

#ifdef __cplusplus
}
#endif

#endif
