#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cssg.h"
#include "node.h"
#include "buffer.h"
#include "utf8.h"
#include "scanners.h"
#include "render.h"

#define OUT(s, wrap, escaping) renderer->out(renderer, s, wrap, escaping)
#define LIT(s) renderer->out(renderer, s, false, LITERAL)
#define CR() renderer->cr(renderer)
#define BLANKLINE() renderer->blankline(renderer)
#define LIST_NUMBER_STRING_SIZE 20

static inline void outc(cssg_renderer *renderer, cssg_escaping escape,
                        int32_t c, unsigned char nextc) {
  if (escape == LITERAL) {
    cssg_render_code_point(renderer, c);
    return;
  }

  switch (c) {
  case 123: // '{'
  case 125: // '}'
  case 35:  // '#'
  case 37:  // '%'
  case 38:  // '&'
    cssg_render_ascii(renderer, "\\");
    cssg_render_code_point(renderer, c);
    break;
  case 36: // '$'
  case 95: // '_'
    if (escape == NORMAL) {
      cssg_render_ascii(renderer, "\\");
    }
    cssg_render_code_point(renderer, c);
    break;
  case 45:             // '-'
    if (nextc == 45) { // prevent ligature
      cssg_render_ascii(renderer, "-{}");
    } else {
      cssg_render_ascii(renderer, "-");
    }
    break;
  case 126: // '~'
    if (escape == NORMAL) {
      cssg_render_ascii(renderer, "\\textasciitilde{}");
    } else {
      cssg_render_code_point(renderer, c);
    }
    break;
  case 94: // '^'
    cssg_render_ascii(renderer, "\\^{}");
    break;
  case 92: // '\\'
    if (escape == URL) {
      // / acts as path sep even on windows:
      cssg_render_ascii(renderer, "/");
    } else {
      cssg_render_ascii(renderer, "\\textbackslash{}");
    }
    break;
  case 124: // '|'
    cssg_render_ascii(renderer, "\\textbar{}");
    break;
  case 60: // '<'
    cssg_render_ascii(renderer, "\\textless{}");
    break;
  case 62: // '>'
    cssg_render_ascii(renderer, "\\textgreater{}");
    break;
  case 91: // '['
  case 93: // ']'
    cssg_render_ascii(renderer, "{");
    cssg_render_code_point(renderer, c);
    cssg_render_ascii(renderer, "}");
    break;
  case 34: // '"'
    cssg_render_ascii(renderer, "\\textquotedbl{}");
    // requires \usepackage[T1]{fontenc}
    break;
  case 39: // '\''
    cssg_render_ascii(renderer, "\\textquotesingle{}");
    // requires \usepackage{textcomp}
    break;
  case 160: // nbsp
    cssg_render_ascii(renderer, "~");
    break;
  case 8230: // hellip
    cssg_render_ascii(renderer, "\\ldots{}");
    break;
  case 8216: // lsquo
    if (escape == NORMAL) {
      cssg_render_ascii(renderer, "`");
    } else {
      cssg_render_code_point(renderer, c);
    }
    break;
  case 8217: // rsquo
    if (escape == NORMAL) {
      cssg_render_ascii(renderer, "\'");
    } else {
      cssg_render_code_point(renderer, c);
    }
    break;
  case 8220: // ldquo
    if (escape == NORMAL) {
      cssg_render_ascii(renderer, "``");
    } else {
      cssg_render_code_point(renderer, c);
    }
    break;
  case 8221: // rdquo
    if (escape == NORMAL) {
      cssg_render_ascii(renderer, "''");
    } else {
      cssg_render_code_point(renderer, c);
    }
    break;
  case 8212: // emdash
    if (escape == NORMAL) {
      cssg_render_ascii(renderer, "---");
    } else {
      cssg_render_code_point(renderer, c);
    }
    break;
  case 8211: // endash
    if (escape == NORMAL) {
      cssg_render_ascii(renderer, "--");
    } else {
      cssg_render_code_point(renderer, c);
    }
    break;
  default:
    cssg_render_code_point(renderer, c);
  }
}

typedef enum {
  NO_LINK,
  URL_AUTOLINK,
  EMAIL_AUTOLINK,
  NORMAL_LINK,
  INTERNAL_LINK
} link_type;

static link_type get_link_type(cssg_node *node) {
  size_t title_len, url_len;
  cssg_node *link_text;
  char *realurl;
  int realurllen;
  bool isemail = false;

  if (node->type != CSSG_NODE_LINK) {
    return NO_LINK;
  }

  const char *url = cssg_node_get_url(node);
  cssg_chunk url_chunk = cssg_chunk_literal(url);

  if (url && *url == '#') {
    return INTERNAL_LINK;
  }

  url_len = strlen(url);
  if (url_len == 0 || scan_scheme(&url_chunk, 0) == 0) {
    return NO_LINK;
  }

  const char *title = cssg_node_get_title(node);
  title_len = strlen(title);
  // if it has a title, we can't treat it as an autolink:
  if (title_len == 0) {

    link_text = node->first_child;
    cssg_consolidate_text_nodes(link_text);

    if (!link_text)
      return NO_LINK;

    realurl = (char *)url;
    realurllen = (int)url_len;
    if (strncmp(realurl, "mailto:", 7) == 0) {
      realurl += 7;
      realurllen -= 7;
      isemail = true;
    }
    if (realurllen == link_text->len &&
        strncmp(realurl, (char *)link_text->data,
                link_text->len) == 0) {
      if (isemail) {
        return EMAIL_AUTOLINK;
      } else {
        return URL_AUTOLINK;
      }
    }
  }

  return NORMAL_LINK;
}

static int S_get_enumlevel(cssg_node *node) {
  int enumlevel = 0;
  cssg_node *tmp = node;
  while (tmp) {
    if (tmp->type == CSSG_NODE_LIST &&
        cssg_node_get_list_type(node) == CSSG_ORDERED_LIST) {
      enumlevel++;
    }
    tmp = tmp->parent;
  }
  return enumlevel;
}

static int S_render_node(cssg_renderer *renderer, cssg_node *node,
                         cssg_event_type ev_type, int options) {
  int list_number;
  int enumlevel;
  char list_number_string[LIST_NUMBER_STRING_SIZE];
  bool entering = (ev_type == CSSG_EVENT_ENTER);
  cssg_list_type list_type;
  bool allow_wrap = renderer->width > 0 && !(CSSG_OPT_NOBREAKS & options);

  // avoid warning about unused parameter:
  (void)(options);

  switch (node->type) {
  case CSSG_NODE_DOCUMENT:
    break;

  case CSSG_NODE_BLOCK_QUOTE:
    if (entering) {
      LIT("\\begin{quote}");
      CR();
    } else {
      LIT("\\end{quote}");
      BLANKLINE();
    }
    break;

  case CSSG_NODE_LIST:
    list_type = cssg_node_get_list_type(node);
    if (entering) {
      LIT("\\begin{");
      LIT(list_type == CSSG_ORDERED_LIST ? "enumerate" : "itemize");
      LIT("}");
      CR();
      list_number = cssg_node_get_list_start(node);
      if (list_number > 1) {
        enumlevel = S_get_enumlevel(node);
        // latex normally supports only five levels
        if (enumlevel >= 1 && enumlevel <= 5) {
          snprintf(list_number_string, LIST_NUMBER_STRING_SIZE, "%d",
                   list_number - 1); // the next item will increment this
          LIT("\\setcounter{enum");
          switch (enumlevel) {
          case 1: LIT("i"); break;
          case 2: LIT("ii"); break;
          case 3: LIT("iii"); break;
          case 4: LIT("iv"); break;
          case 5: LIT("v"); break;
          default: LIT("i"); break;
          }
          LIT("}{");
          OUT(list_number_string, false, NORMAL);
          LIT("}");
        }
        CR();
      }
    } else {
      LIT("\\end{");
      LIT(list_type == CSSG_ORDERED_LIST ? "enumerate" : "itemize");
      LIT("}");
      BLANKLINE();
    }
    break;

  case CSSG_NODE_ITEM:
    if (entering) {
      LIT("\\item ");
    } else {
      CR();
    }
    break;

  case CSSG_NODE_HEADING:
    if (entering) {
      switch (cssg_node_get_heading_level(node)) {
      case 1:
        LIT("\\section");
        break;
      case 2:
        LIT("\\subsection");
        break;
      case 3:
        LIT("\\subsubsection");
        break;
      case 4:
        LIT("\\paragraph");
        break;
      case 5:
        LIT("\\subparagraph");
        break;
      }
      LIT("{");
    } else {
      LIT("}");
      BLANKLINE();
    }
    break;

  case CSSG_NODE_CODE_BLOCK:
    CR();
    LIT("\\begin{verbatim}");
    CR();
    OUT(cssg_node_get_literal(node), false, LITERAL);
    CR();
    LIT("\\end{verbatim}");
    BLANKLINE();
    break;

  case CSSG_NODE_HTML_BLOCK:
    break;

  case CSSG_NODE_CUSTOM_BLOCK:
    CR();
    OUT(entering ? cssg_node_get_on_enter(node) : cssg_node_get_on_exit(node),
        false, LITERAL);
    CR();
    break;

  case CSSG_NODE_THEMATIC_BREAK:
    BLANKLINE();
    LIT("\\begin{center}\\rule{0.5\\linewidth}{\\linethickness}\\end{center}");
    BLANKLINE();
    break;

  case CSSG_NODE_PARAGRAPH:
    if (!entering) {
      BLANKLINE();
    }
    break;

  case CSSG_NODE_TEXT:
    OUT(cssg_node_get_literal(node), allow_wrap, NORMAL);
    break;

  case CSSG_NODE_LINEBREAK:
    LIT("\\\\");
    CR();
    break;

  case CSSG_NODE_SOFTBREAK:
    if (options & CSSG_OPT_HARDBREAKS) {
      LIT("\\\\");
      CR();
    } else if (renderer->width == 0 && !(CSSG_OPT_NOBREAKS & options)) {
      CR();
    } else {
      OUT(" ", allow_wrap, NORMAL);
    }
    break;

  case CSSG_NODE_CODE:
    LIT("\\texttt{");
    OUT(cssg_node_get_literal(node), false, NORMAL);
    LIT("}");
    break;

  case CSSG_NODE_HTML_INLINE:
    break;

  case CSSG_NODE_CUSTOM_INLINE:
    OUT(entering ? cssg_node_get_on_enter(node) : cssg_node_get_on_exit(node),
        false, LITERAL);
    break;

  case CSSG_NODE_STRONG:
    if (entering) {
      LIT("\\textbf{");
    } else {
      LIT("}");
    }
    break;

  case CSSG_NODE_EMPH:
    if (entering) {
      LIT("\\emph{");
    } else {
      LIT("}");
    }
    break;

  case CSSG_NODE_LINK:
    if (entering) {
      const char *url = cssg_node_get_url(node);
      // requires \usepackage{hyperref}
      switch (get_link_type(node)) {
      case URL_AUTOLINK:
        LIT("\\url{");
        OUT(url, false, URL);
        LIT("}");
        return 0; // Don't process further nodes to avoid double-rendering artefacts
      case EMAIL_AUTOLINK:
        LIT("\\href{");
        OUT(url, false, URL);
        LIT("}\\nolinkurl{");
        break;
      case NORMAL_LINK:
        LIT("\\href{");
        OUT(url, false, URL);
        LIT("}{");
        break;
      case INTERNAL_LINK:
        LIT("\\protect\\hyperlink{");
        OUT(url + 1, false, URL);
        LIT("}{");
        break;
      case NO_LINK:
        LIT("{"); // error?
      }
    } else {
      LIT("}");
    }

    break;

  case CSSG_NODE_IMAGE:
    if (entering) {
      LIT("\\protect\\includegraphics{");
      // requires \include{graphicx}
      OUT(cssg_node_get_url(node), false, URL);
      LIT("}");
      return 0;
    }
    break;

  default:
    assert(false);
    break;
  }

  return 1;
}

char *cssg_render_latex(cssg_node *root, int options, int width) {
  return cssg_render(root, options, width, outc, S_render_node);
}
