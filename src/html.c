#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cssg_ctype.h"
#include "cssg.h"
#include "node.h"
#include "buffer.h"
#include "houdini.h"
#include "scanners.h"

#include "html.h"

#define BUFFER_SIZE 100

// Functions to convert cssg_nodes to HTML strings.

static void escape_html(cssg_strbuf *dest, const unsigned char *source,
                        bufsize_t length) {
  houdini_escape_html(dest, source, length, 0);
}

static inline void cr(cssg_strbuf *html) {
  if (html->size && html->ptr[html->size - 1] != '\n')
    cssg_strbuf_putc(html, '\n');
}

struct render_state {
  cssg_strbuf *html;
  cssg_node *plain;
};

static void S_render_sourcepos(cssg_node *node, cssg_strbuf *html,
                               int options) {
  char buffer[BUFFER_SIZE];
  if (CSSG_OPT_SOURCEPOS & options) {
    snprintf(buffer, BUFFER_SIZE, " data-sourcepos=\"%d:%d-%d:%d\"",
             cssg_node_get_start_line(node), cssg_node_get_start_column(node),
             cssg_node_get_end_line(node), cssg_node_get_end_column(node));
    cssg_strbuf_puts(html, buffer);
  }
}

static int S_render_node(cssg_node *node, cssg_event_type ev_type,
                         struct render_state *state, int options) {
  cssg_node *parent;
  cssg_node *grandparent;
  cssg_strbuf *html = state->html;
  char start_heading[] = "<h0";
  char end_heading[] = "</h0";
  bool tight;
  char buffer[BUFFER_SIZE];

  bool entering = (ev_type == CSSG_EVENT_ENTER);

  if (state->plain == node) { // back at original node
    state->plain = NULL;
  }

  if (state->plain != NULL) {
    switch (node->type) {
    case CSSG_NODE_TEXT:
    case CSSG_NODE_CODE:
    case CSSG_NODE_HTML_INLINE:
      escape_html(html, node->data, node->len);
      break;

    case CSSG_NODE_LINEBREAK:
    case CSSG_NODE_SOFTBREAK:
      cssg_strbuf_putc(html, ' ');
      break;

    default:
      break;
    }
    return 1;
  }

  switch (node->type) {
  case CSSG_NODE_DOCUMENT:
    if (entering) {
      cssg_strbuf_puts(html, htmlOpen);
      cssg_strbuf_puts(html, htmlHead);
      cssg_strbuf_puts(html, bodyOpen);
      cssg_strbuf_puts(html, nav);
      cssg_strbuf_puts(html, mainHTML);
      cssg_strbuf_puts(html, article);
      cssg_strbuf_puts(html, headerArticle);
      cssg_strbuf_puts(html, asideArticle);
      cssg_strbuf_puts(html, mainArticle);
    }
    else {
      cssg_strbuf_puts(html, htmlFooter);
      cssg_strbuf_puts(html, htmlTerminal);
    }
    break;

  case CSSG_NODE_BLOCK_QUOTE:
    if (entering) {
      cr(html);
      cssg_strbuf_puts(html, "<blockquote");
      S_render_sourcepos(node, html, options);
      cssg_strbuf_puts(html, ">\n");
    } else {
      cr(html);
      cssg_strbuf_puts(html, "</blockquote>\n");
    }
    break;

  case CSSG_NODE_LIST: {
    cssg_list_type list_type = (cssg_list_type)node->as.list.list_type;
    int start = node->as.list.start;

    if (entering) {
      cr(html);
      if (list_type == CSSG_BULLET_LIST) {
        cssg_strbuf_puts(html, "<ul");
        S_render_sourcepos(node, html, options);
        cssg_strbuf_puts(html, ">\n");
      } else if (start == 1) {
        cssg_strbuf_puts(html, "<ol");
        S_render_sourcepos(node, html, options);
        cssg_strbuf_puts(html, ">\n");
      } else {
        snprintf(buffer, BUFFER_SIZE, "<ol start=\"%d\"", start);
        cssg_strbuf_puts(html, buffer);
        S_render_sourcepos(node, html, options);
        cssg_strbuf_puts(html, ">\n");
      }
    } else {
      cssg_strbuf_puts(html,
                        list_type == CSSG_BULLET_LIST ? "</ul>\n" : "</ol>\n");
    }
    break;
  }

  case CSSG_NODE_ITEM:
    if (entering) {
      cr(html);
      cssg_strbuf_puts(html, "<li");
      S_render_sourcepos(node, html, options);
      cssg_strbuf_putc(html, '>');
    } else {
      cssg_strbuf_puts(html, "</li>\n");
    }
    break;

  case CSSG_NODE_HEADING:
    if (entering) {
      cr(html);
      start_heading[2] = (char)('0' + node->as.heading.level);
      cssg_strbuf_puts(html, start_heading);
      S_render_sourcepos(node, html, options);
      cssg_strbuf_putc(html, '>');
    } else {
      end_heading[3] = (char)('0' + node->as.heading.level);
      cssg_strbuf_puts(html, end_heading);
      cssg_strbuf_puts(html, ">\n");
    }
    break;

  case CSSG_NODE_CODE_BLOCK:
    cr(html);

    if (node->as.code.info == NULL || node->as.code.info[0] == 0) {
      cssg_strbuf_puts(html, "<pre");
      S_render_sourcepos(node, html, options);
      cssg_strbuf_puts(html, "><code>");
    } else {
      bufsize_t first_tag = 0;
      while (node->as.code.info[first_tag] &&
             !cssg_isspace(node->as.code.info[first_tag])) {
        first_tag += 1;
      }

      cssg_strbuf_puts(html, "<pre");
      S_render_sourcepos(node, html, options);
      cssg_strbuf_puts(html, "><code class=\"");
      if (strncmp((char *)node->as.code.info, "language-", 9) != 0) {
        cssg_strbuf_puts(html, "language-");
      }
      escape_html(html, node->as.code.info, first_tag);
      cssg_strbuf_puts(html, "\">");
    }

    escape_html(html, node->data, node->len);
    cssg_strbuf_puts(html, "</code></pre>\n");
    break;

  case CSSG_NODE_HTML_BLOCK:
    cr(html);
    if (!(options & CSSG_OPT_UNSAFE)) {
      cssg_strbuf_puts(html, "<!-- raw HTML omitted -->");
    } else {
      cssg_strbuf_put(html, node->data, node->len);
    }
    cr(html);
    break;

  case CSSG_NODE_CUSTOM_BLOCK: {
    unsigned char *block = entering ? node->as.custom.on_enter :
                                      node->as.custom.on_exit;
    cr(html);
    if (block) {
      cssg_strbuf_puts(html, (char *)block);
    }
    cr(html);
    break;
  }

  case CSSG_NODE_THEMATIC_BREAK:
    cr(html);
    cssg_strbuf_puts(html, "<hr");
    S_render_sourcepos(node, html, options);
    cssg_strbuf_puts(html, " />\n");
    break;

  case CSSG_NODE_PARAGRAPH:
    parent = cssg_node_parent(node);
    grandparent = cssg_node_parent(parent);
    if (grandparent != NULL && grandparent->type == CSSG_NODE_LIST) {
      tight = grandparent->as.list.tight;
    } else {
      tight = false;
    }
    if (!tight) {
      if (entering) {
        cr(html);
        cssg_strbuf_puts(html, "<p");
        S_render_sourcepos(node, html, options);
        cssg_strbuf_putc(html, '>');
      } else {
        cssg_strbuf_puts(html, "</p>\n");
      }
    }
    break;

  case CSSG_NODE_TEXT:
    escape_html(html, node->data, node->len);
    break;

  case CSSG_NODE_LINEBREAK:
    cssg_strbuf_puts(html, "<br />\n");
    break;

  case CSSG_NODE_SOFTBREAK:
    if (options & CSSG_OPT_HARDBREAKS) {
      cssg_strbuf_puts(html, "<br />\n");
    } else if (options & CSSG_OPT_NOBREAKS) {
      cssg_strbuf_putc(html, ' ');
    } else {
      cssg_strbuf_putc(html, '\n');
    }
    break;

  case CSSG_NODE_CODE:
    cssg_strbuf_puts(html, "<code>");
    escape_html(html, node->data, node->len);
    cssg_strbuf_puts(html, "</code>");
    break;

  case CSSG_NODE_HTML_INLINE:
    if (!(options & CSSG_OPT_UNSAFE)) {
      cssg_strbuf_puts(html, "<!-- raw HTML omitted -->");
    } else {
      cssg_strbuf_put(html, node->data, node->len);
    }
    break;

  case CSSG_NODE_CUSTOM_INLINE: {
    unsigned char *block = entering ? node->as.custom.on_enter :
                                      node->as.custom.on_exit;
    if (block) {
      cssg_strbuf_puts(html, (char *)block);
    }
    break;
  }

  case CSSG_NODE_STRONG:
    if (entering) {
      cssg_strbuf_puts(html, "<strong>");
    } else {
      cssg_strbuf_puts(html, "</strong>");
    }
    break;

  case CSSG_NODE_EMPH:
    if (entering) {
      cssg_strbuf_puts(html, "<em>");
    } else {
      cssg_strbuf_puts(html, "</em>");
    }
    break;

  case CSSG_NODE_LINK:
    if (entering) {
      cssg_strbuf_puts(html, "<a href=\"");
      if (node->as.link.url && ((options & CSSG_OPT_UNSAFE) ||
                                !(_scan_dangerous_url(node->as.link.url)))) {
        houdini_escape_href(html, node->as.link.url,
                            (bufsize_t)strlen((char *)node->as.link.url));
      }
      if (node->as.link.title) {
        cssg_strbuf_puts(html, "\" title=\"");
        escape_html(html, node->as.link.title,
                    (bufsize_t)strlen((char *)node->as.link.title));
      }
      cssg_strbuf_puts(html, "\">");
    } else {
      cssg_strbuf_puts(html, "</a>");
    }
    break;

  case CSSG_NODE_IMAGE:
    if (entering) {
      cssg_strbuf_puts(html, "<img src=\"");
      if (node->as.link.url && ((options & CSSG_OPT_UNSAFE) ||
                                !(_scan_dangerous_url(node->as.link.url)))) {
        houdini_escape_href(html, node->as.link.url,
                            (bufsize_t)strlen((char *)node->as.link.url));
      }
      cssg_strbuf_puts(html, "\" alt=\"");
      state->plain = node;
    } else {
      if (node->as.link.title) {
        cssg_strbuf_puts(html, "\" title=\"");
        escape_html(html, node->as.link.title,
                    (bufsize_t)strlen((char *)node->as.link.title));
      }

      cssg_strbuf_puts(html, "\" />");
    }
    break;

  default:
    assert(false);
    break;
  }

  // cssg_strbuf_putc(html, 'x');
  return 1;
}

char *cssg_render_html(cssg_node *root, int options) {
  char *result;
  cssg_strbuf html = CSSG_BUF_INIT(root->mem);
  cssg_event_type ev_type;
  cssg_node *cur;
  struct render_state state = {&html, NULL};
  cssg_iter *iter = cssg_iter_new(root);

  while ((ev_type = cssg_iter_next(iter)) != CSSG_EVENT_DONE) {
    cur = cssg_iter_get_node(iter);
    S_render_node(cur, ev_type, &state, options);
  }
  result = (char *)cssg_strbuf_detach(&html);

  cssg_iter_free(iter);
  return result;
}
