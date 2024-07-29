#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cssg.h"
#include "node.h"
#include "buffer.h"

#define BUFFER_SIZE 100
#define MAX_INDENT 40

// Functions to convert cssg_nodes to XML strings.

// C0 control characters, U+FFFE and U+FFF aren't allowed in XML.
static const char XML_ESCAPE_TABLE[256] = {
    /* 0x00 */ 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 0, 1, 1,
    /* 0x10 */ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    /* 0x20 */ 0, 0, 2, 0, 0, 0, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    /* 0x30 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4, 0, 5, 0,
    /* 0x40 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    /* 0x50 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    /* 0x60 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    /* 0x70 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    /* 0x80 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    /* 0x90 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    /* 0xA0 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    /* 0xB0 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 9, 9,
    /* 0xC0 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    /* 0xD0 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    /* 0xE0 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    /* 0xF0 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};

// U+FFFD Replacement Character encoded in UTF-8
#define UTF8_REPL "\xEF\xBF\xBD"

static const char *XML_ESCAPES[] = {
  "", UTF8_REPL, "&quot;", "&amp;", "&lt;", "&gt;"
};

static void escape_xml(cssg_strbuf *ob, const unsigned char *src,
                       bufsize_t size) {
  bufsize_t i = 0, org, esc = 0;

  while (i < size) {
    org = i;
    while (i < size && (esc = XML_ESCAPE_TABLE[src[i]]) == 0)
      i++;

    if (i > org)
      cssg_strbuf_put(ob, src + org, i - org);

    if (i >= size)
      break;

    if (esc == 9) {
      // To replace U+FFFE and U+FFFF with U+FFFD, only the last byte has to
      // be changed.
      // We know that src[i] is 0xBE or 0xBF.
      if (i >= 2 && src[i-2] == 0xEF && src[i-1] == 0xBF) {
        cssg_strbuf_putc(ob, 0xBD);
      } else {
        cssg_strbuf_putc(ob, src[i]);
      }
    } else {
      cssg_strbuf_puts(ob, XML_ESCAPES[esc]);
    }

    i++;
  }
}

static void escape_xml_str(cssg_strbuf *dest, const unsigned char *source) {
  if (source)
    escape_xml(dest, source, (bufsize_t)strlen((char *)source));
}

struct render_state {
  cssg_strbuf *xml;
  int indent;
};

static inline void indent(struct render_state *state) {
  int i;
  for (i = 0; i < state->indent && i < MAX_INDENT; i++) {
    cssg_strbuf_putc(state->xml, ' ');
  }
}

static int S_render_node(cssg_node *node, cssg_event_type ev_type,
                         struct render_state *state, int options) {
  cssg_strbuf *xml = state->xml;
  bool literal = false;
  cssg_delim_type delim;
  bool entering = (ev_type == CSSG_EVENT_ENTER);
  char buffer[BUFFER_SIZE];

  if (entering) {
    indent(state);
    cssg_strbuf_putc(xml, '<');
    cssg_strbuf_puts(xml, cssg_node_get_type_string(node));

    if (options & CSSG_OPT_SOURCEPOS && node->start_line != 0) {
      snprintf(buffer, BUFFER_SIZE, " sourcepos=\"%d:%d-%d:%d\"",
               node->start_line, node->start_column, node->end_line,
               node->end_column);
      cssg_strbuf_puts(xml, buffer);
    }

    literal = false;

    switch (node->type) {
    case CSSG_NODE_DOCUMENT:
      cssg_strbuf_puts(xml, " xmlns=\"http://commonmark.org/xml/1.0\"");
      break;
    case CSSG_NODE_TEXT:
    case CSSG_NODE_CODE:
    case CSSG_NODE_HTML_BLOCK:
    case CSSG_NODE_HTML_INLINE:
      cssg_strbuf_puts(xml, " xml:space=\"preserve\">");
      escape_xml(xml, node->data, node->len);
      cssg_strbuf_puts(xml, "</");
      cssg_strbuf_puts(xml, cssg_node_get_type_string(node));
      literal = true;
      break;
    case CSSG_NODE_LIST:
      switch (cssg_node_get_list_type(node)) {
      case CSSG_ORDERED_LIST:
        cssg_strbuf_puts(xml, " type=\"ordered\"");
        snprintf(buffer, BUFFER_SIZE, " start=\"%d\"",
                 cssg_node_get_list_start(node));
        cssg_strbuf_puts(xml, buffer);
        delim = cssg_node_get_list_delim(node);
        if (delim == CSSG_PAREN_DELIM) {
          cssg_strbuf_puts(xml, " delim=\"paren\"");
        } else if (delim == CSSG_PERIOD_DELIM) {
          cssg_strbuf_puts(xml, " delim=\"period\"");
        }
        break;
      case CSSG_BULLET_LIST:
        cssg_strbuf_puts(xml, " type=\"bullet\"");
        break;
      default:
        break;
      }
      snprintf(buffer, BUFFER_SIZE, " tight=\"%s\"",
               (cssg_node_get_list_tight(node) ? "true" : "false"));
      cssg_strbuf_puts(xml, buffer);
      break;
    case CSSG_NODE_HEADING:
      snprintf(buffer, BUFFER_SIZE, " level=\"%d\"", node->as.heading.level);
      cssg_strbuf_puts(xml, buffer);
      break;
    case CSSG_NODE_CODE_BLOCK:
      if (node->as.code.info) {
        cssg_strbuf_puts(xml, " info=\"");
        escape_xml_str(xml, node->as.code.info);
        cssg_strbuf_putc(xml, '"');
      }
      cssg_strbuf_puts(xml, " xml:space=\"preserve\">");
      escape_xml(xml, node->data, node->len);
      cssg_strbuf_puts(xml, "</");
      cssg_strbuf_puts(xml, cssg_node_get_type_string(node));
      literal = true;
      break;
    case CSSG_NODE_CUSTOM_BLOCK:
    case CSSG_NODE_CUSTOM_INLINE:
      cssg_strbuf_puts(xml, " on_enter=\"");
      escape_xml_str(xml, node->as.custom.on_enter);
      cssg_strbuf_putc(xml, '"');
      cssg_strbuf_puts(xml, " on_exit=\"");
      escape_xml_str(xml, node->as.custom.on_exit);
      cssg_strbuf_putc(xml, '"');
      break;
    case CSSG_NODE_LINK:
    case CSSG_NODE_IMAGE:
      cssg_strbuf_puts(xml, " destination=\"");
      escape_xml_str(xml, node->as.link.url);
      cssg_strbuf_putc(xml, '"');
      if (node->as.link.title) {
        cssg_strbuf_puts(xml, " title=\"");
        escape_xml_str(xml, node->as.link.title);
        cssg_strbuf_putc(xml, '"');
      }
      break;
    default:
      break;
    }
    if (node->first_child) {
      state->indent += 2;
    } else if (!literal) {
      cssg_strbuf_puts(xml, " /");
    }
    cssg_strbuf_puts(xml, ">\n");

  } else if (node->first_child) {
    state->indent -= 2;
    indent(state);
    cssg_strbuf_puts(xml, "</");
    cssg_strbuf_puts(xml, cssg_node_get_type_string(node));
    cssg_strbuf_puts(xml, ">\n");
  }

  return 1;
}

char *cssg_render_xml(cssg_node *root, int options) {
  char *result;
  cssg_strbuf xml = CSSG_BUF_INIT(root->mem);
  cssg_event_type ev_type;
  cssg_node *cur;
  struct render_state state = {&xml, 0};

  cssg_iter *iter = cssg_iter_new(root);

  cssg_strbuf_puts(state.xml, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
  cssg_strbuf_puts(state.xml,
                    "<!DOCTYPE document SYSTEM \"CommonMark.dtd\">\n");
  while ((ev_type = cssg_iter_next(iter)) != CSSG_EVENT_DONE) {
    cur = cssg_iter_get_node(iter);
    S_render_node(cur, ev_type, &state, options);
  }
  result = (char *)cssg_strbuf_detach(&xml);

  cssg_iter_free(iter);
  return result;
}
