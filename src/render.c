#include <stdlib.h>
#include "buffer.h"
#include "cssg.h"
#include "utf8.h"
#include "render.h"
#include "node.h"
#include "cssg_ctype.h"

static inline void S_cr(cssg_renderer *renderer) {
  if (renderer->need_cr < 1) {
    renderer->need_cr = 1;
  }
}

static inline void S_blankline(cssg_renderer *renderer) {
  if (renderer->need_cr < 2) {
    renderer->need_cr = 2;
  }
}

static void S_out(cssg_renderer *renderer, const char *source, bool wrap,
                  cssg_escaping escape) {
  int length = (int)strlen(source);
  unsigned char nextc;
  int32_t c;
  int i = 0;
  int last_nonspace;
  int len;
  int k = renderer->buffer->size - 1;

  wrap = wrap && !renderer->no_linebreaks;

  if (renderer->in_tight_list_item && renderer->need_cr > 1) {
    renderer->need_cr = 1;
  }
  while (renderer->need_cr) {
    if (k < 0 || renderer->buffer->ptr[k] == '\n') {
      k -= 1;
    } else {
      cssg_strbuf_putc(renderer->buffer, '\n');
      if (renderer->need_cr > 1) {
        cssg_strbuf_put(renderer->buffer, renderer->prefix->ptr,
                         renderer->prefix->size);
      }
    }
    renderer->column = 0;
    renderer->last_breakable = 0;
    renderer->begin_line = true;
    renderer->begin_content = true;
    renderer->need_cr -= 1;
  }

  while (i < length) {
    if (renderer->begin_line) {
      cssg_strbuf_put(renderer->buffer, renderer->prefix->ptr,
                       renderer->prefix->size);
      // note: this assumes prefix is ascii:
      renderer->column = renderer->prefix->size;
    }

    len = cssg_utf8proc_iterate((const uint8_t *)source + i, length - i, &c);
    if (len == -1) { // error condition
      return;        // return without rendering rest of string
    }
    nextc = source[i + len];
    if (c == 32 && wrap) {
      if (!renderer->begin_line) {
        last_nonspace = renderer->buffer->size;
        cssg_strbuf_putc(renderer->buffer, ' ');
        renderer->column += 1;
        renderer->begin_line = false;
        renderer->begin_content = false;
        // skip following spaces
        while (source[i + 1] == ' ') {
          i++;
        }
        // We don't allow breaks that make a digit the first character
        // because this causes problems with commonmark output.
        if (!cssg_isdigit(source[i + 1])) {
          renderer->last_breakable = last_nonspace;
        }
      }

    } else if (escape == LITERAL) {
      if (c == 10) {
        cssg_strbuf_putc(renderer->buffer, '\n');
        renderer->column = 0;
        renderer->begin_line = true;
        renderer->begin_content = true;
        renderer->last_breakable = 0;
      } else {
        cssg_render_code_point(renderer, c);
        renderer->begin_line = false;
        // we don't set 'begin_content' to false til we've
        // finished parsing a digit.  Reason:  in commonmark
        // we need to escape a potential list marker after
        // a digit:
        renderer->begin_content =
            renderer->begin_content && cssg_isdigit(c) == 1;
      }
    } else {
      (renderer->outc)(renderer, escape, c, nextc);
      renderer->begin_line = false;
      renderer->begin_content =
          renderer->begin_content && cssg_isdigit(c) == 1;
    }

    // If adding the character went beyond width, look for an
    // earlier place where the line could be broken:
    if (renderer->width > 0 && renderer->column > renderer->width &&
        !renderer->begin_line && renderer->last_breakable > 0) {

      // copy from last_breakable to remainder
      unsigned char *src = renderer->buffer->ptr +
                           renderer->last_breakable + 1;
      bufsize_t remainder_len = renderer->buffer->size -
                                renderer->last_breakable - 1;
      unsigned char *remainder =
          (unsigned char *)renderer->mem->realloc(NULL, remainder_len);
      memcpy(remainder, src, remainder_len);
      // truncate at last_breakable
      cssg_strbuf_truncate(renderer->buffer, renderer->last_breakable);
      // add newline, prefix, and remainder
      cssg_strbuf_putc(renderer->buffer, '\n');
      cssg_strbuf_put(renderer->buffer, renderer->prefix->ptr,
                       renderer->prefix->size);
      cssg_strbuf_put(renderer->buffer, remainder, remainder_len);
      renderer->column = renderer->prefix->size + remainder_len;
      renderer->mem->free(remainder);
      renderer->last_breakable = 0;
      renderer->begin_line = false;
      renderer->begin_content = false;
    }

    i += len;
  }
}

// Assumes no newlines, assumes ascii content:
void cssg_render_ascii(cssg_renderer *renderer, const char *s) {
  int origsize = renderer->buffer->size;
  cssg_strbuf_puts(renderer->buffer, s);
  renderer->column += renderer->buffer->size - origsize;
}

void cssg_render_code_point(cssg_renderer *renderer, uint32_t c) {
  cssg_utf8proc_encode_char(c, renderer->buffer);
  renderer->column += 1;
}

char *cssg_render(cssg_node *root, int options, int width,
                   void (*outc)(cssg_renderer *, cssg_escaping, int32_t,
                                unsigned char),
                   int (*render_node)(cssg_renderer *renderer,
                                      cssg_node *node,
                                      cssg_event_type ev_type, int options)) {
  cssg_mem *mem = root->mem;
  cssg_strbuf pref = CSSG_BUF_INIT(mem);
  cssg_strbuf buf = CSSG_BUF_INIT(mem);
  cssg_node *cur;
  cssg_event_type ev_type;
  char *result;
  cssg_iter *iter = cssg_iter_new(root);

  cssg_renderer renderer = {options,
                             mem,    &buf,    &pref,      0,      width,
                             0,      0,       true,       true,   false,
                             false,  NULL,
                             outc,   S_cr,    S_blankline, S_out};

  while ((ev_type = cssg_iter_next(iter)) != CSSG_EVENT_DONE) {
    cur = cssg_iter_get_node(iter);
    if (!render_node(&renderer, cur, ev_type, options)) {
      // a false value causes us to skip processing
      // the node's contents.  this is used for
      // autolinks.
      cssg_iter_reset(iter, cur, CSSG_EVENT_EXIT);
    }
  }

  // ensure final newline
  if (renderer.buffer->size == 0 || renderer.buffer->ptr[renderer.buffer->size - 1] != '\n') {
    cssg_strbuf_putc(renderer.buffer, '\n');
  }

  result = (char *)cssg_strbuf_detach(renderer.buffer);

  cssg_iter_free(iter);
  cssg_strbuf_free(renderer.prefix);
  cssg_strbuf_free(renderer.buffer);

  return result;
}
