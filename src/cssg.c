#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include "node.h"
#include "houdini.h"
#include "cssg.h"
#include "buffer.h"

int cssg_version(void) { return CSSG_VERSION; }

const char *cssg_version_string(void) { return CSSG_VERSION_STRING; }

static void *xcalloc(size_t nmem, size_t size) {
  void *ptr = calloc(nmem, size);
  if (!ptr) {
    fprintf(stderr, "[cssg] calloc returned null pointer, aborting\n");
    abort();
  }
  return ptr;
}

static void *xrealloc(void *ptr, size_t size) {
  void *new_ptr = realloc(ptr, size);
  if (!new_ptr) {
    fprintf(stderr, "[cssg] realloc returned null pointer, aborting\n");
    abort();
  }
  return new_ptr;
}

cssg_mem DEFAULT_MEM_ALLOCATOR = {xcalloc, xrealloc, free};

cssg_mem *cssg_get_default_mem_allocator(void) {
  return &DEFAULT_MEM_ALLOCATOR;
}


char *cssg_markdown_to_html(const char *text, size_t len, int options) {
  cssg_node *doc;
  char *result;

  doc = cssg_parse_document(text, len, options);

  result = cssg_render_html(doc, options);
  cssg_node_free(doc);

  return result;
}
