#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CSSG_NO_SHORT_NAMES
#include "cssg.h"
#include "node.h"

#include "harness.h"
#include "cplusplus.h"

#define UTF8_REPL "\xEF\xBF\xBD"

static const cssg_node_type node_types[] = {
    CSSG_NODE_DOCUMENT,  CSSG_NODE_BLOCK_QUOTE, CSSG_NODE_LIST,
    CSSG_NODE_ITEM,      CSSG_NODE_CODE_BLOCK,  CSSG_NODE_HTML_BLOCK,
    CSSG_NODE_PARAGRAPH, CSSG_NODE_HEADING,     CSSG_NODE_THEMATIC_BREAK,
    CSSG_NODE_TEXT,      CSSG_NODE_SOFTBREAK,   CSSG_NODE_LINEBREAK,
    CSSG_NODE_CODE,      CSSG_NODE_HTML_INLINE, CSSG_NODE_EMPH,
    CSSG_NODE_STRONG,    CSSG_NODE_LINK,        CSSG_NODE_IMAGE};
static const int num_node_types = sizeof(node_types) / sizeof(*node_types);

static void test_md_to_html(test_batch_runner *runner, const char *markdown,
                            const char *expected_html, const char *msg);

static void test_content(test_batch_runner *runner, cssg_node_type type,
                         int allowed_content);

static void test_char(test_batch_runner *runner, int valid, const char *utf8,
                      const char *msg);

static void test_incomplete_char(test_batch_runner *runner, const char *utf8,
                                 const char *msg);

static void test_continuation_byte(test_batch_runner *runner, const char *utf8);

static void version(test_batch_runner *runner) {
  INT_EQ(runner, cssg_version(), CSSG_VERSION, "cssg_version");
  STR_EQ(runner, cssg_version_string(), CSSG_VERSION_STRING,
         "cssg_version_string");
}

static void constructor(test_batch_runner *runner) {
  for (int i = 0; i < num_node_types; ++i) {
    cssg_node_type type = node_types[i];
    cssg_node *node = cssg_node_new(type);
    OK(runner, node != NULL, "new type %d", type);
    INT_EQ(runner, cssg_node_get_type(node), type, "get_type %d", type);

    switch (node->type) {
    case CSSG_NODE_HEADING:
      INT_EQ(runner, cssg_node_get_heading_level(node), 1,
             "default heading level is 1");
      node->as.heading.level = 1;
      break;

    case CSSG_NODE_LIST:
      INT_EQ(runner, cssg_node_get_list_type(node), CSSG_BULLET_LIST,
             "default is list type is bullet");
      INT_EQ(runner, cssg_node_get_list_delim(node), CSSG_NO_DELIM,
             "default is list delim is NO_DELIM");
      INT_EQ(runner, cssg_node_get_list_start(node), 0,
             "default is list start is 0");
      INT_EQ(runner, cssg_node_get_list_tight(node), 0,
             "default is list is loose");
      break;

    default:
      break;
    }

    cssg_node_free(node);
  }
}

static void accessors(test_batch_runner *runner) {
  static const char markdown[] = "## Header\n"
                                 "\n"
                                 "* Item 1\n"
                                 "* Item 2\n"
                                 "\n"
                                 "2. Item 1\n"
                                 "\n"
                                 "3. Item 2\n"
                                 "\n"
                                 "``` lang\n"
                                 "fenced\n"
                                 "```\n"
                                 "    code\n"
                                 "\n"
                                 "<div>html</div>\n"
                                 "\n"
                                 "[link](url 'title')\n";

  cssg_node *doc =
      cssg_parse_document(markdown, sizeof(markdown) - 1, CSSG_OPT_DEFAULT);

  // Getters

  cssg_node *heading = cssg_node_first_child(doc);
  INT_EQ(runner, cssg_node_get_heading_level(heading), 2, "get_heading_level");

  cssg_node *bullet_list = cssg_node_next(heading);
  INT_EQ(runner, cssg_node_get_list_type(bullet_list), CSSG_BULLET_LIST,
         "get_list_type bullet");
  INT_EQ(runner, cssg_node_get_list_tight(bullet_list), 1,
         "get_list_tight tight");

  cssg_node *ordered_list = cssg_node_next(bullet_list);
  INT_EQ(runner, cssg_node_get_list_type(ordered_list), CSSG_ORDERED_LIST,
         "get_list_type ordered");
  INT_EQ(runner, cssg_node_get_list_delim(ordered_list), CSSG_PERIOD_DELIM,
         "get_list_delim ordered");
  INT_EQ(runner, cssg_node_get_list_start(ordered_list), 2, "get_list_start");
  INT_EQ(runner, cssg_node_get_list_tight(ordered_list), 0,
         "get_list_tight loose");

  cssg_node *fenced = cssg_node_next(ordered_list);
  STR_EQ(runner, cssg_node_get_literal(fenced), "fenced\n",
         "get_literal fenced code");
  STR_EQ(runner, cssg_node_get_fence_info(fenced), "lang", "get_fence_info");

  cssg_node *code = cssg_node_next(fenced);
  STR_EQ(runner, cssg_node_get_literal(code), "code\n",
         "get_literal indented code");

  cssg_node *html = cssg_node_next(code);
  STR_EQ(runner, cssg_node_get_literal(html), "<div>html</div>\n",
         "get_literal html");

  cssg_node *paragraph = cssg_node_next(html);
  INT_EQ(runner, cssg_node_get_start_line(paragraph), 17, "get_start_line");
  INT_EQ(runner, cssg_node_get_start_column(paragraph), 1, "get_start_column");
  INT_EQ(runner, cssg_node_get_end_line(paragraph), 17, "get_end_line");

  cssg_node *link = cssg_node_first_child(paragraph);
  STR_EQ(runner, cssg_node_get_url(link), "url", "get_url");
  STR_EQ(runner, cssg_node_get_title(link), "title", "get_title");

  cssg_node *string = cssg_node_first_child(link);
  STR_EQ(runner, cssg_node_get_literal(string), "link", "get_literal string");

  // Setters

  OK(runner, cssg_node_set_heading_level(heading, 3), "set_heading_level");

  OK(runner, cssg_node_set_list_type(bullet_list, CSSG_ORDERED_LIST),
     "set_list_type ordered");
  OK(runner, cssg_node_set_list_delim(bullet_list, CSSG_PAREN_DELIM),
     "set_list_delim paren");
  OK(runner, cssg_node_set_list_start(bullet_list, 3), "set_list_start");
  OK(runner, cssg_node_set_list_tight(bullet_list, 0), "set_list_tight loose");

  OK(runner, cssg_node_set_list_type(ordered_list, CSSG_BULLET_LIST),
     "set_list_type bullet");
  OK(runner, cssg_node_set_list_tight(ordered_list, 1),
     "set_list_tight tight");

  OK(runner, cssg_node_set_literal(code, "CODE\n"),
     "set_literal indented code");

  OK(runner, cssg_node_set_literal(fenced, "FENCED\n"),
     "set_literal fenced code");
  OK(runner, cssg_node_set_fence_info(fenced, "LANG"), "set_fence_info");

  OK(runner, cssg_node_set_literal(html, "<div>HTML</div>\n"),
     "set_literal html");

  OK(runner, cssg_node_set_url(link, "URL"), "set_url");
  OK(runner, cssg_node_set_title(link, "TITLE"), "set_title");

  OK(runner, cssg_node_set_literal(string, "prefix-LINK"),
     "set_literal string");

  // Set literal to suffix of itself (issue #139).
  const char *literal = cssg_node_get_literal(string);
  OK(runner, cssg_node_set_literal(string, literal + sizeof("prefix")),
     "set_literal suffix");

  char *rendered_html = cssg_render_html(doc,
                          CSSG_OPT_DEFAULT | CSSG_OPT_UNSAFE);
  static const char expected_html[] =
      "<h3>Header</h3>\n"
      "<ol start=\"3\">\n"
      "<li>\n"
      "<p>Item 1</p>\n"
      "</li>\n"
      "<li>\n"
      "<p>Item 2</p>\n"
      "</li>\n"
      "</ol>\n"
      "<ul>\n"
      "<li>Item 1</li>\n"
      "<li>Item 2</li>\n"
      "</ul>\n"
      "<pre><code class=\"language-LANG\">FENCED\n"
      "</code></pre>\n"
      "<pre><code>CODE\n"
      "</code></pre>\n"
      "<div>HTML</div>\n"
      "<p><a href=\"URL\" title=\"TITLE\">LINK</a></p>\n";
  STR_EQ(runner, rendered_html, expected_html, "setters work");
  free(rendered_html);

  // Getter errors

  INT_EQ(runner, cssg_node_get_heading_level(bullet_list), 0,
         "get_heading_level error");
  INT_EQ(runner, cssg_node_get_list_type(heading), CSSG_NO_LIST,
         "get_list_type error");
  INT_EQ(runner, cssg_node_get_list_start(code), 0, "get_list_start error");
  INT_EQ(runner, cssg_node_get_list_tight(fenced), 0, "get_list_tight error");
  OK(runner, cssg_node_get_literal(ordered_list) == NULL, "get_literal error");
  OK(runner, cssg_node_get_fence_info(paragraph) == NULL,
     "get_fence_info error");
  OK(runner, cssg_node_get_url(html) == NULL, "get_url error");
  OK(runner, cssg_node_get_title(heading) == NULL, "get_title error");

  // Setter errors

  OK(runner, !cssg_node_set_heading_level(bullet_list, 3),
     "set_heading_level error");
  OK(runner, !cssg_node_set_list_type(heading, CSSG_ORDERED_LIST),
     "set_list_type error");
  OK(runner, !cssg_node_set_list_start(code, 3), "set_list_start error");
  OK(runner, !cssg_node_set_list_tight(fenced, 0), "set_list_tight error");
  OK(runner, !cssg_node_set_literal(ordered_list, "content\n"),
     "set_literal error");
  OK(runner, !cssg_node_set_fence_info(paragraph, "lang"),
     "set_fence_info error");
  OK(runner, !cssg_node_set_url(html, "url"), "set_url error");
  OK(runner, !cssg_node_set_title(heading, "title"), "set_title error");

  OK(runner, !cssg_node_set_heading_level(heading, 0),
     "set_heading_level too small");
  OK(runner, !cssg_node_set_heading_level(heading, 7),
     "set_heading_level too large");
  OK(runner, !cssg_node_set_list_type(bullet_list, CSSG_NO_LIST),
     "set_list_type invalid");
  OK(runner, !cssg_node_set_list_start(bullet_list, -1),
     "set_list_start negative");

  cssg_node_free(doc);
}

static void free_parent(test_batch_runner *runner) {
  static const char markdown[] = "text\n";

  cssg_node *doc =
      cssg_parse_document(markdown, sizeof(markdown) - 1, CSSG_OPT_DEFAULT);

  cssg_node *para = cssg_node_first_child(doc);
  cssg_node *text = cssg_node_first_child(para);
  cssg_node_unlink(text);
  cssg_node_free(doc);
  STR_EQ(runner, cssg_node_get_literal(text), "text",
         "inline content after freeing parent block");
  cssg_node_free(text);
}

static void node_check(test_batch_runner *runner) {
  // Construct an incomplete tree.
  cssg_node *doc = cssg_node_new(CSSG_NODE_DOCUMENT);
  cssg_node *p1 = cssg_node_new(CSSG_NODE_PARAGRAPH);
  cssg_node *p2 = cssg_node_new(CSSG_NODE_PARAGRAPH);
  doc->first_child = p1;
  p1->next = p2;

  INT_EQ(runner, cssg_node_check(doc, NULL), 4, "node_check works");
  INT_EQ(runner, cssg_node_check(doc, NULL), 0, "node_check fixes tree");

  cssg_node_free(doc);
}

static void iterator(test_batch_runner *runner) {
  cssg_node *doc = cssg_parse_document("> a *b*\n\nc", 10, CSSG_OPT_DEFAULT);
  int parnodes = 0;
  cssg_event_type ev_type;
  cssg_iter *iter = cssg_iter_new(doc);
  cssg_node *cur;

  while ((ev_type = cssg_iter_next(iter)) != CSSG_EVENT_DONE) {
    cur = cssg_iter_get_node(iter);
    if (cur->type == CSSG_NODE_PARAGRAPH && ev_type == CSSG_EVENT_ENTER) {
      parnodes += 1;
    }
  }
  INT_EQ(runner, parnodes, 2, "iterate correctly counts paragraphs");

  cssg_iter_free(iter);
  cssg_node_free(doc);
}

static void iterator_delete(test_batch_runner *runner) {
  static const char md[] = "a *b* c\n"
                           "\n"
                           "* item1\n"
                           "* item2\n"
                           "\n"
                           "a `b` c\n"
                           "\n"
                           "* item1\n"
                           "* item2\n";
  cssg_node *doc = cssg_parse_document(md, sizeof(md) - 1, CSSG_OPT_DEFAULT);
  cssg_iter *iter = cssg_iter_new(doc);
  cssg_event_type ev_type;

  while ((ev_type = cssg_iter_next(iter)) != CSSG_EVENT_DONE) {
    cssg_node *node = cssg_iter_get_node(iter);
    // Delete list, emph, and code nodes.
    if ((ev_type == CSSG_EVENT_EXIT && node->type == CSSG_NODE_LIST) ||
        (ev_type == CSSG_EVENT_EXIT && node->type == CSSG_NODE_EMPH) ||
        (ev_type == CSSG_EVENT_ENTER && node->type == CSSG_NODE_CODE)) {
      cssg_node_free(node);
    }
  }

  char *html = cssg_render_html(doc, CSSG_OPT_DEFAULT);
  static const char expected[] = "<p>a  c</p>\n"
                                 "<p>a  c</p>\n";
  STR_EQ(runner, html, expected, "iterate and delete nodes");

  cssg_mem *allocator = cssg_get_default_mem_allocator();

  allocator->free(html);
  cssg_iter_free(iter);
  cssg_node_free(doc);
}

static void create_tree(test_batch_runner *runner) {
  char *html;
  cssg_node *doc = cssg_node_new(CSSG_NODE_DOCUMENT);

  cssg_node *p = cssg_node_new(CSSG_NODE_PARAGRAPH);
  OK(runner, !cssg_node_insert_before(doc, p), "insert before root fails");
  OK(runner, !cssg_node_insert_after(doc, p), "insert after root fails");
  OK(runner, cssg_node_append_child(doc, p), "append1");
  INT_EQ(runner, cssg_node_check(doc, NULL), 0, "append1 consistent");
  OK(runner, cssg_node_parent(p) == doc, "node_parent");

  cssg_node *emph = cssg_node_new(CSSG_NODE_EMPH);
  OK(runner, cssg_node_prepend_child(p, emph), "prepend1");
  INT_EQ(runner, cssg_node_check(doc, NULL), 0, "prepend1 consistent");

  cssg_node *str1 = cssg_node_new(CSSG_NODE_TEXT);
  cssg_node_set_literal(str1, "Hello, ");
  OK(runner, cssg_node_prepend_child(p, str1), "prepend2");
  INT_EQ(runner, cssg_node_check(doc, NULL), 0, "prepend2 consistent");

  cssg_node *str3 = cssg_node_new(CSSG_NODE_TEXT);
  cssg_node_set_literal(str3, "!");
  OK(runner, cssg_node_append_child(p, str3), "append2");
  INT_EQ(runner, cssg_node_check(doc, NULL), 0, "append2 consistent");

  cssg_node *str2 = cssg_node_new(CSSG_NODE_TEXT);
  cssg_node_set_literal(str2, "world");
  OK(runner, cssg_node_append_child(emph, str2), "append3");
  INT_EQ(runner, cssg_node_check(doc, NULL), 0, "append3 consistent");

  html = cssg_render_html(doc, CSSG_OPT_DEFAULT);
  STR_EQ(runner, html, "<p>Hello, <em>world</em>!</p>\n", "render_html");
  free(html);

  OK(runner, cssg_node_insert_before(str1, str3), "ins before1");
  INT_EQ(runner, cssg_node_check(doc, NULL), 0, "ins before1 consistent");
  // 31e
  OK(runner, cssg_node_first_child(p) == str3, "ins before1 works");

  OK(runner, cssg_node_insert_before(str1, emph), "ins before2");
  INT_EQ(runner, cssg_node_check(doc, NULL), 0, "ins before2 consistent");
  // 3e1
  OK(runner, cssg_node_last_child(p) == str1, "ins before2 works");

  OK(runner, cssg_node_insert_after(str1, str3), "ins after1");
  INT_EQ(runner, cssg_node_check(doc, NULL), 0, "ins after1 consistent");
  // e13
  OK(runner, cssg_node_next(str1) == str3, "ins after1 works");

  OK(runner, cssg_node_insert_after(str1, emph), "ins after2");
  INT_EQ(runner, cssg_node_check(doc, NULL), 0, "ins after2 consistent");
  // 1e3
  OK(runner, cssg_node_previous(emph) == str1, "ins after2 works");

  cssg_node *str4 = cssg_node_new(CSSG_NODE_TEXT);
  cssg_node_set_literal(str4, "brzz");
  OK(runner, cssg_node_replace(str1, str4), "replace");
  // The replaced node is not freed
  cssg_node_free(str1);

  INT_EQ(runner, cssg_node_check(doc, NULL), 0, "replace consistent");
  OK(runner, cssg_node_previous(emph) == str4, "replace works");
  INT_EQ(runner, cssg_node_replace(p, str4), 0, "replace str for p fails");

  cssg_node_unlink(emph);

  html = cssg_render_html(doc, CSSG_OPT_DEFAULT);
  STR_EQ(runner, html, "<p>brzz!</p>\n", "render_html after shuffling");
  free(html);

  cssg_node_free(doc);
  cssg_node_free(emph);
}

static void custom_nodes(test_batch_runner *runner) {
  char *html;
  char *man;
  cssg_node *doc = cssg_node_new(CSSG_NODE_DOCUMENT);
  cssg_node *p = cssg_node_new(CSSG_NODE_PARAGRAPH);
  cssg_node_append_child(doc, p);
  cssg_node *ci = cssg_node_new(CSSG_NODE_CUSTOM_INLINE);
  cssg_node *str1 = cssg_node_new(CSSG_NODE_TEXT);
  cssg_node_set_literal(str1, "Hello");
  OK(runner, cssg_node_append_child(ci, str1), "append1");
  OK(runner, cssg_node_set_on_enter(ci, "<ON ENTER|"), "set_on_enter");
  OK(runner, cssg_node_set_on_exit(ci, "|ON EXIT>"), "set_on_exit");
  STR_EQ(runner, cssg_node_get_on_enter(ci), "<ON ENTER|", "get_on_enter");
  STR_EQ(runner, cssg_node_get_on_exit(ci), "|ON EXIT>", "get_on_exit");
  cssg_node_append_child(p, ci);
  cssg_node *cb = cssg_node_new(CSSG_NODE_CUSTOM_BLOCK);
  cssg_node_set_on_enter(cb, "<on enter|");
  // leave on_exit unset
  STR_EQ(runner, cssg_node_get_on_exit(cb), "", "get_on_exit (empty)");
  cssg_node_append_child(doc, cb);

  html = cssg_render_html(doc, CSSG_OPT_DEFAULT);
  STR_EQ(runner, html, "<p><ON ENTER|Hello|ON EXIT></p>\n<on enter|\n",
         "render_html");
  free(html);

  man = cssg_render_man(doc, CSSG_OPT_DEFAULT, 0);
  STR_EQ(runner, man, ".PP\n<ON ENTER|Hello|ON EXIT>\n<on enter|\n",
         "render_man");
  free(man);

  cssg_node_free(doc);
}

void hierarchy(test_batch_runner *runner) {
  cssg_node *bquote1 = cssg_node_new(CSSG_NODE_BLOCK_QUOTE);
  cssg_node *bquote2 = cssg_node_new(CSSG_NODE_BLOCK_QUOTE);
  cssg_node *bquote3 = cssg_node_new(CSSG_NODE_BLOCK_QUOTE);

  OK(runner, cssg_node_append_child(bquote1, bquote2), "append bquote2");
  OK(runner, cssg_node_append_child(bquote2, bquote3), "append bquote3");
  OK(runner, !cssg_node_append_child(bquote3, bquote3),
     "adding a node as child of itself fails");
  OK(runner, !cssg_node_append_child(bquote3, bquote1),
     "adding a parent as child fails");

  cssg_node_free(bquote1);

  int max_node_type = CSSG_NODE_LAST_BLOCK > CSSG_NODE_LAST_INLINE
                          ? CSSG_NODE_LAST_BLOCK
                          : CSSG_NODE_LAST_INLINE;
  OK(runner, max_node_type < 32, "all node types < 32");

  int list_item_flag = 1 << CSSG_NODE_ITEM;
  int top_level_blocks =
      (1 << CSSG_NODE_BLOCK_QUOTE) | (1 << CSSG_NODE_LIST) |
      (1 << CSSG_NODE_CODE_BLOCK) | (1 << CSSG_NODE_HTML_BLOCK) |
      (1 << CSSG_NODE_PARAGRAPH) | (1 << CSSG_NODE_HEADING) |
      (1 << CSSG_NODE_THEMATIC_BREAK);
  int all_inlines = (1 << CSSG_NODE_TEXT) | (1 << CSSG_NODE_SOFTBREAK) |
                    (1 << CSSG_NODE_LINEBREAK) | (1 << CSSG_NODE_CODE) |
                    (1 << CSSG_NODE_HTML_INLINE) | (1 << CSSG_NODE_EMPH) |
                    (1 << CSSG_NODE_STRONG) | (1 << CSSG_NODE_LINK) |
                    (1 << CSSG_NODE_IMAGE);

  test_content(runner, CSSG_NODE_DOCUMENT, top_level_blocks);
  test_content(runner, CSSG_NODE_BLOCK_QUOTE, top_level_blocks);
  test_content(runner, CSSG_NODE_LIST, list_item_flag);
  test_content(runner, CSSG_NODE_ITEM, top_level_blocks);
  test_content(runner, CSSG_NODE_CODE_BLOCK, 0);
  test_content(runner, CSSG_NODE_HTML_BLOCK, 0);
  test_content(runner, CSSG_NODE_PARAGRAPH, all_inlines);
  test_content(runner, CSSG_NODE_HEADING, all_inlines);
  test_content(runner, CSSG_NODE_THEMATIC_BREAK, 0);
  test_content(runner, CSSG_NODE_TEXT, 0);
  test_content(runner, CSSG_NODE_SOFTBREAK, 0);
  test_content(runner, CSSG_NODE_LINEBREAK, 0);
  test_content(runner, CSSG_NODE_CODE, 0);
  test_content(runner, CSSG_NODE_HTML_INLINE, 0);
  test_content(runner, CSSG_NODE_EMPH, all_inlines);
  test_content(runner, CSSG_NODE_STRONG, all_inlines);
  test_content(runner, CSSG_NODE_LINK, all_inlines);
  test_content(runner, CSSG_NODE_IMAGE, all_inlines);
}

static void test_content(test_batch_runner *runner, cssg_node_type type,
                         int allowed_content) {
  cssg_node *node = cssg_node_new(type);

  for (int i = 0; i < num_node_types; ++i) {
    cssg_node_type child_type = node_types[i];
    cssg_node *child = cssg_node_new(child_type);

    int got = cssg_node_append_child(node, child);
    int expected = (allowed_content >> child_type) & 1;

    INT_EQ(runner, got, expected, "add %d as child of %d", child_type, type);

    cssg_node_free(child);
  }

  cssg_node_free(node);
}

static void parser(test_batch_runner *runner) {
  test_md_to_html(runner, "No newline", "<p>No newline</p>\n",
                  "document without trailing newline");
}

static void render_html(test_batch_runner *runner) {
  char *html;

  static const char markdown[] = "foo *bar*\n"
                                 "\n"
                                 "paragraph 2\n";
  cssg_node *doc =
      cssg_parse_document(markdown, sizeof(markdown) - 1, CSSG_OPT_DEFAULT);

  cssg_node *paragraph = cssg_node_first_child(doc);
  html = cssg_render_html(paragraph, CSSG_OPT_DEFAULT);
  STR_EQ(runner, html, "<p>foo <em>bar</em></p>\n", "render single paragraph");
  free(html);

  cssg_node *string = cssg_node_first_child(paragraph);
  html = cssg_render_html(string, CSSG_OPT_DEFAULT);
  STR_EQ(runner, html, "foo ", "render single inline");
  free(html);

  cssg_node *emph = cssg_node_next(string);
  html = cssg_render_html(emph, CSSG_OPT_DEFAULT);
  STR_EQ(runner, html, "<em>bar</em>", "render inline with children");
  free(html);

  cssg_node_free(doc);
}

static void render_xml(test_batch_runner *runner) {
  char *xml;

  static const char markdown[] = "foo *bar*\n"
                                 "\n"
                                 "control -\x0C-\n"
                                 "fffe -\xEF\xBF\xBE-\n"
                                 "ffff -\xEF\xBF\xBF-\n"
                                 "escape <>&\"\n"
                                 "\n"
                                 "```\ncode\n```\n";
  cssg_node *doc =
      cssg_parse_document(markdown, sizeof(markdown) - 1, CSSG_OPT_DEFAULT);

  xml = cssg_render_xml(doc, CSSG_OPT_DEFAULT);
  STR_EQ(runner, xml, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
                      "<!DOCTYPE document SYSTEM \"CommonMark.dtd\">\n"
                      "<document xmlns=\"http://commonmark.org/xml/1.0\">\n"
                      "  <paragraph>\n"
                      "    <text xml:space=\"preserve\">foo </text>\n"
                      "    <emph>\n"
                      "      <text xml:space=\"preserve\">bar</text>\n"
                      "    </emph>\n"
                      "  </paragraph>\n"
                      "  <paragraph>\n"
                      "    <text xml:space=\"preserve\">control -" UTF8_REPL "-</text>\n"
                      "    <softbreak />\n"
                      "    <text xml:space=\"preserve\">fffe -" UTF8_REPL "-</text>\n"
                      "    <softbreak />\n"
                      "    <text xml:space=\"preserve\">ffff -" UTF8_REPL "-</text>\n"
                      "    <softbreak />\n"
                      "    <text xml:space=\"preserve\">escape &lt;&gt;&amp;&quot;</text>\n"
                      "  </paragraph>\n"
                      "  <code_block xml:space=\"preserve\">code\n"
                      "</code_block>\n"
                      "</document>\n",
         "render document");
  free(xml);
  cssg_node *paragraph = cssg_node_first_child(doc);
  xml = cssg_render_xml(paragraph, CSSG_OPT_DEFAULT | CSSG_OPT_SOURCEPOS);
  STR_EQ(runner, xml, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
                      "<!DOCTYPE document SYSTEM \"CommonMark.dtd\">\n"
                      "<paragraph sourcepos=\"1:1-1:9\">\n"
                      "  <text sourcepos=\"1:1-1:4\" xml:space=\"preserve\">foo </text>\n"
                      "  <emph sourcepos=\"1:5-1:9\">\n"
                      "    <text sourcepos=\"1:6-1:8\" xml:space=\"preserve\">bar</text>\n"
                      "  </emph>\n"
                      "</paragraph>\n",
         "render first paragraph with source pos");
  free(xml);
  cssg_node_free(doc);
}

static void render_man(test_batch_runner *runner) {
  char *man;

  static const char markdown[] = "foo *bar*\n"
                                 "\n"
                                 "- Lorem ipsum dolor sit amet,\n"
                                 "  consectetur adipiscing elit,\n"
                                 "- sed do eiusmod tempor incididunt\n"
                                 "  ut labore et dolore magna aliqua.\n";
  cssg_node *doc =
      cssg_parse_document(markdown, sizeof(markdown) - 1, CSSG_OPT_DEFAULT);

  man = cssg_render_man(doc, CSSG_OPT_DEFAULT, 20);
  STR_EQ(runner, man, ".PP\n"
                      "foo \\f[I]bar\\f[]\n"
                      ".IP \\[bu] 2\n"
                      "Lorem ipsum dolor\n"
                      "sit amet,\n"
                      "consectetur\n"
                      "adipiscing elit,\n"
                      ".IP \\[bu] 2\n"
                      "sed do eiusmod\n"
                      "tempor incididunt ut\n"
                      "labore et dolore\n"
                      "magna aliqua.\n",
         "render document with wrapping");
  free(man);
  man = cssg_render_man(doc, CSSG_OPT_DEFAULT, 0);
  STR_EQ(runner, man, ".PP\n"
                      "foo \\f[I]bar\\f[]\n"
                      ".IP \\[bu] 2\n"
                      "Lorem ipsum dolor sit amet,\n"
                      "consectetur adipiscing elit,\n"
                      ".IP \\[bu] 2\n"
                      "sed do eiusmod tempor incididunt\n"
                      "ut labore et dolore magna aliqua.\n",
         "render document without wrapping");
  free(man);
  cssg_node_free(doc);
}

static void render_commonmark(test_batch_runner *runner) {
  char *commonmark;

  static const char markdown[] = "> \\- foo *bar* \\*bar\\*\n"
                                 "\n"
                                 "- Lorem ipsum dolor sit amet,\n"
                                 "  consectetur adipiscing elit,\n"
                                 "- sed do eiusmod tempor incididunt\n"
                                 "  ut labore et dolore magna aliqua.\n";
  cssg_node *doc =
      cssg_parse_document(markdown, sizeof(markdown) - 1, CSSG_OPT_DEFAULT);

  commonmark = cssg_render_commonmark(doc, CSSG_OPT_DEFAULT, 26);
  STR_EQ(runner, commonmark, "> \\- foo *bar* \\*bar\\*\n"
                             "\n"
                             "  - Lorem ipsum dolor sit\n"
                             "    amet, consectetur\n"
                             "    adipiscing elit,\n"
                             "  - sed do eiusmod tempor\n"
                             "    incididunt ut labore\n"
                             "    et dolore magna\n"
                             "    aliqua.\n",
         "render document with wrapping");
  free(commonmark);
  commonmark = cssg_render_commonmark(doc, CSSG_OPT_DEFAULT, 0);
  STR_EQ(runner, commonmark, "> \\- foo *bar* \\*bar\\*\n"
                             "\n"
                             "  - Lorem ipsum dolor sit amet,\n"
                             "    consectetur adipiscing elit,\n"
                             "  - sed do eiusmod tempor incididunt\n"
                             "    ut labore et dolore magna aliqua.\n",
         "render document without wrapping");
  free(commonmark);

  cssg_node *text = cssg_node_new(CSSG_NODE_TEXT);
  cssg_node_set_literal(text, "Hi");
  commonmark = cssg_render_commonmark(text, CSSG_OPT_DEFAULT, 0);
  STR_EQ(runner, commonmark, "Hi\n", "render single inline node");
  free(commonmark);

  cssg_node_free(text);
  cssg_node_free(doc);
}

static void utf8(test_batch_runner *runner) {
  // Ranges
  test_char(runner, 1, "\x01", "valid utf8 01");
  test_char(runner, 1, "\x7F", "valid utf8 7F");
  test_char(runner, 0, "\x80", "invalid utf8 80");
  test_char(runner, 0, "\xBF", "invalid utf8 BF");
  test_char(runner, 0, "\xC0\x80", "invalid utf8 C080");
  test_char(runner, 0, "\xC1\xBF", "invalid utf8 C1BF");
  test_char(runner, 1, "\xC2\x80", "valid utf8 C280");
  test_char(runner, 1, "\xDF\xBF", "valid utf8 DFBF");
  test_char(runner, 0, "\xE0\x80\x80", "invalid utf8 E08080");
  test_char(runner, 0, "\xE0\x9F\xBF", "invalid utf8 E09FBF");
  test_char(runner, 1, "\xE0\xA0\x80", "valid utf8 E0A080");
  test_char(runner, 1, "\xED\x9F\xBF", "valid utf8 ED9FBF");
  test_char(runner, 0, "\xED\xA0\x80", "invalid utf8 EDA080");
  test_char(runner, 0, "\xED\xBF\xBF", "invalid utf8 EDBFBF");
  test_char(runner, 0, "\xF0\x80\x80\x80", "invalid utf8 F0808080");
  test_char(runner, 0, "\xF0\x8F\xBF\xBF", "invalid utf8 F08FBFBF");
  test_char(runner, 1, "\xF0\x90\x80\x80", "valid utf8 F0908080");
  test_char(runner, 1, "\xF4\x8F\xBF\xBF", "valid utf8 F48FBFBF");
  test_char(runner, 0, "\xF4\x90\x80\x80", "invalid utf8 F4908080");
  test_char(runner, 0, "\xF7\xBF\xBF\xBF", "invalid utf8 F7BFBFBF");
  test_char(runner, 0, "\xF8", "invalid utf8 F8");
  test_char(runner, 0, "\xFF", "invalid utf8 FF");

  // Incomplete byte sequences at end of input
  test_incomplete_char(runner, "\xE0\xA0", "invalid utf8 E0A0");
  test_incomplete_char(runner, "\xF0\x90\x80", "invalid utf8 F09080");

  // Invalid continuation bytes
  test_continuation_byte(runner, "\xC2\x80");
  test_continuation_byte(runner, "\xE0\xA0\x80");
  test_continuation_byte(runner, "\xF0\x90\x80\x80");

  // Test string containing null character
  static const char string_with_null[] = "((((\0))))";
  char *html = cssg_markdown_to_html(
      string_with_null, sizeof(string_with_null) - 1, CSSG_OPT_DEFAULT);
  STR_EQ(runner, html, "<p>((((" UTF8_REPL "))))</p>\n", "utf8 with U+0000");
  free(html);

  // Test NUL followed by newline
  static const char string_with_nul_lf[] = "```\n\0\n```\n";
  html = cssg_markdown_to_html(
      string_with_nul_lf, sizeof(string_with_nul_lf) - 1, CSSG_OPT_DEFAULT);
  STR_EQ(runner, html, "<pre><code>\xef\xbf\xbd\n</code></pre>\n",
         "utf8 with \\0\\n");
  free(html);
}

static void test_char(test_batch_runner *runner, int valid, const char *utf8,
                      const char *msg) {
  char buf[20];
  sprintf(buf, "((((%s))))", utf8);

  if (valid) {
    char expected[30];
    sprintf(expected, "<p>((((%s))))</p>\n", utf8);
    test_md_to_html(runner, buf, expected, msg);
  } else {
    test_md_to_html(runner, buf, "<p>((((" UTF8_REPL "))))</p>\n", msg);
  }
}

static void test_incomplete_char(test_batch_runner *runner, const char *utf8,
                                 const char *msg) {
  char buf[20];
  sprintf(buf, "----%s", utf8);
  test_md_to_html(runner, buf, "<p>----" UTF8_REPL "</p>\n", msg);
}

static void test_continuation_byte(test_batch_runner *runner,
                                   const char *utf8) {
  size_t len = strlen(utf8);

  for (size_t pos = 1; pos < len; ++pos) {
    char buf[20];
    sprintf(buf, "((((%s))))", utf8);
    buf[4 + pos] = '\x20';

    char expected[50];
    strcpy(expected, "<p>((((" UTF8_REPL "\x20");
    for (size_t i = pos + 1; i < len; ++i) {
      strcat(expected, UTF8_REPL);
    }
    strcat(expected, "))))</p>\n");

    char *html =
        cssg_markdown_to_html(buf, strlen(buf), CSSG_OPT_VALIDATE_UTF8);
    STR_EQ(runner, html, expected, "invalid utf8 continuation byte %d/%d", pos,
           len);
    free(html);
  }
}

static void line_endings(test_batch_runner *runner) {
  // Test list with different line endings
  static const char list_with_endings[] = "- a\n- b\r\n- c\r- d";
  char *html = cssg_markdown_to_html(
      list_with_endings, sizeof(list_with_endings) - 1, CSSG_OPT_DEFAULT);
  STR_EQ(runner, html,
         "<ul>\n<li>a</li>\n<li>b</li>\n<li>c</li>\n<li>d</li>\n</ul>\n",
         "list with different line endings");
  free(html);

  static const char crlf_lines[] = "line\r\nline\r\n";
  html = cssg_markdown_to_html(crlf_lines, sizeof(crlf_lines) - 1,
                                CSSG_OPT_DEFAULT | CSSG_OPT_HARDBREAKS);
  STR_EQ(runner, html, "<p>line<br />\nline</p>\n",
         "crlf endings with CSSG_OPT_HARDBREAKS");
  free(html);
  html = cssg_markdown_to_html(crlf_lines, sizeof(crlf_lines) - 1,
                                CSSG_OPT_DEFAULT | CSSG_OPT_NOBREAKS);
  STR_EQ(runner, html, "<p>line line</p>\n",
         "crlf endings with CSSG_OPT_NOBREAKS");
  free(html);

  static const char no_line_ending[] = "```\nline\n```";
  html = cssg_markdown_to_html(no_line_ending, sizeof(no_line_ending) - 1,
                                CSSG_OPT_DEFAULT);
  STR_EQ(runner, html, "<pre><code>line\n</code></pre>\n",
         "fenced code block with no final newline");
  free(html);
}

static void numeric_entities(test_batch_runner *runner) {
  test_md_to_html(runner, "&#0;", "<p>" UTF8_REPL "</p>\n",
                  "Invalid numeric entity 0");
  test_md_to_html(runner, "&#55295;", "<p>\xED\x9F\xBF</p>\n",
                  "Valid numeric entity 0xD7FF");
  test_md_to_html(runner, "&#xD800;", "<p>" UTF8_REPL "</p>\n",
                  "Invalid numeric entity 0xD800");
  test_md_to_html(runner, "&#xDFFF;", "<p>" UTF8_REPL "</p>\n",
                  "Invalid numeric entity 0xDFFF");
  test_md_to_html(runner, "&#57344;", "<p>\xEE\x80\x80</p>\n",
                  "Valid numeric entity 0xE000");
  test_md_to_html(runner, "&#x10FFFF;", "<p>\xF4\x8F\xBF\xBF</p>\n",
                  "Valid numeric entity 0x10FFFF");
  test_md_to_html(runner, "&#x110000;", "<p>" UTF8_REPL "</p>\n",
                  "Invalid numeric entity 0x110000");
  test_md_to_html(runner, "&#x80000000;", "<p>&amp;#x80000000;</p>\n",
                  "Invalid numeric entity 0x80000000");
  test_md_to_html(runner, "&#xFFFFFFFF;", "<p>&amp;#xFFFFFFFF;</p>\n",
                  "Invalid numeric entity 0xFFFFFFFF");
  test_md_to_html(runner, "&#99999999;", "<p>&amp;#99999999;</p>\n",
                  "Invalid numeric entity 99999999");

  test_md_to_html(runner, "&#;", "<p>&amp;#;</p>\n",
                  "Min decimal entity length");
  test_md_to_html(runner, "&#x;", "<p>&amp;#x;</p>\n",
                  "Min hexadecimal entity length");
  test_md_to_html(runner, "&#999999999;", "<p>&amp;#999999999;</p>\n",
                  "Max decimal entity length");
  test_md_to_html(runner, "&#x000000041;", "<p>&amp;#x000000041;</p>\n",
                  "Max hexadecimal entity length");
}

static void test_safe(test_batch_runner *runner) {
  // Test safe mode
  static const char raw_html[] = "<div>\nhi\n</div>\n\n<a>hi</"
                                 "a>\n[link](JAVAscript:alert('hi'))\n![image]("
                                 "file:my.js)\n";
  char *html = cssg_markdown_to_html(raw_html, sizeof(raw_html) - 1,
                                      CSSG_OPT_DEFAULT);
  STR_EQ(runner, html, "<!-- raw HTML omitted -->\n<p><!-- raw HTML omitted "
                       "-->hi<!-- raw HTML omitted -->\n<a "
                       "href=\"\">link</a>\n<img src=\"\" alt=\"image\" "
                       "/></p>\n",
         "input with raw HTML and dangerous links");
  free(html);
}

static void test_md_to_html(test_batch_runner *runner, const char *markdown,
                            const char *expected_html, const char *msg) {
  char *html = cssg_markdown_to_html(markdown, strlen(markdown),
                                      CSSG_OPT_VALIDATE_UTF8);
  STR_EQ(runner, html, expected_html, msg);
  free(html);
}

static void test_feed_across_line_ending(test_batch_runner *runner) {
  // See #117
  cssg_parser *parser = cssg_parser_new(CSSG_OPT_DEFAULT);
  cssg_parser_feed(parser, "line1\r", 6);
  cssg_parser_feed(parser, "\nline2\r\n", 8);
  cssg_node *document = cssg_parser_finish(parser);
  OK(runner, document->first_child->next == NULL, "document has one paragraph");
  cssg_parser_free(parser);
  cssg_node_free(document);
}

static void sub_document(test_batch_runner *runner) {
  cssg_node *doc = cssg_node_new(CSSG_NODE_DOCUMENT);
  cssg_node *list = cssg_node_new(CSSG_NODE_LIST);
  OK(runner, cssg_node_append_child(doc, list), "list");

  {
    cssg_node *item = cssg_node_new(CSSG_NODE_ITEM);
    OK(runner, cssg_node_append_child(list, item), "append_0");
    static const char markdown[] =
      "Hello &ldquo; <http://www.google.com>\n";
    cssg_parser *parser = cssg_parser_new_with_mem_into_root(
        CSSG_OPT_DEFAULT,
        cssg_get_default_mem_allocator(),
        item);
    cssg_parser_feed(parser, markdown, sizeof(markdown) - 1);
    OK(runner, cssg_parser_finish(parser) != NULL, "parser_finish_0");
    cssg_parser_free(parser);
  }

  {
    cssg_node *item = cssg_node_new(CSSG_NODE_ITEM);
    OK(runner, cssg_node_append_child(list, item), "append_0");
    static const char markdown[] =
      "Bye &ldquo; <http://www.geocities.com>\n";
    cssg_parser *parser = cssg_parser_new_with_mem_into_root(
        CSSG_OPT_DEFAULT,
        cssg_get_default_mem_allocator(),
        item);
    cssg_parser_feed(parser, markdown, sizeof(markdown) - 1);
    OK(runner, cssg_parser_finish(parser) != NULL, "parser_finish_0");
    cssg_parser_free(parser);
  }

  char *xml = cssg_render_xml(doc, CSSG_OPT_DEFAULT);
  STR_EQ(runner, xml, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
                      "<!DOCTYPE document SYSTEM \"CommonMark.dtd\">\n"
                      "<document xmlns=\"http://commonmark.org/xml/1.0\">\n"
                      "  <list type=\"bullet\" tight=\"false\">\n"
                      "    <item>\n"
                      "      <paragraph>\n"
                      "        <text xml:space=\"preserve\">Hello “ </text>\n"
                      "        <link destination=\"http://www.google.com\">\n"
                      "          <text xml:space=\"preserve\">http://www.google.com</text>\n"
                      "        </link>\n"
                      "      </paragraph>\n"
                      "    </item>\n"
                      "    <item>\n"
                      "      <paragraph>\n"
                      "        <text xml:space=\"preserve\">Bye “ </text>\n"
                      "        <link destination=\"http://www.geocities.com\">\n"
                      "          <text xml:space=\"preserve\">http://www.geocities.com</text>\n"
                      "        </link>\n"
                      "      </paragraph>\n"
                      "    </item>\n"
                      "  </list>\n"
                      "</document>\n",
         "nested document XML is as expected");
  free(xml);

  char *cssg = cssg_render_commonmark(doc, CSSG_OPT_DEFAULT, 0);
  STR_EQ(runner, cssg, "  - Hello “ <http://www.google.com>\n"
                        "\n"
                        "  - Bye “ <http://www.geocities.com>\n",
         "nested document CommonMark is as expected");
  free(cssg);

  cssg_node_free(doc);
}

static void source_pos(test_batch_runner *runner) {
  static const char markdown[] =
    "# Hi *there*.\n"
    "\n"
    "Hello &ldquo; <http://www.google.com>\n"
    "there `hi` -- [okay](www.google.com (ok)).\n"
    "\n"
    "> 1. Okay.\n"
    ">    Sure.\n"
    ">\n"
    "> 2. Yes, okay.\n"
    ">    ![ok](hi \"yes\")\n";

  cssg_node *doc = cssg_parse_document(markdown, sizeof(markdown) - 1, CSSG_OPT_DEFAULT);
  char *xml = cssg_render_xml(doc, CSSG_OPT_DEFAULT | CSSG_OPT_SOURCEPOS);
  STR_EQ(runner, xml, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
                      "<!DOCTYPE document SYSTEM \"CommonMark.dtd\">\n"
                      "<document sourcepos=\"1:1-10:20\" xmlns=\"http://commonmark.org/xml/1.0\">\n"
                      "  <heading sourcepos=\"1:1-1:13\" level=\"1\">\n"
                      "    <text sourcepos=\"1:3-1:5\" xml:space=\"preserve\">Hi </text>\n"
                      "    <emph sourcepos=\"1:6-1:12\">\n"
                      "      <text sourcepos=\"1:7-1:11\" xml:space=\"preserve\">there</text>\n"
                      "    </emph>\n"
                      "    <text sourcepos=\"1:13-1:13\" xml:space=\"preserve\">.</text>\n"
                      "  </heading>\n"
                      "  <paragraph sourcepos=\"3:1-4:42\">\n"
                      "    <text sourcepos=\"3:1-3:14\" xml:space=\"preserve\">Hello “ </text>\n"
                      "    <link sourcepos=\"3:15-3:37\" destination=\"http://www.google.com\">\n"
                      "      <text sourcepos=\"3:16-3:36\" xml:space=\"preserve\">http://www.google.com</text>\n"
                      "    </link>\n"
                      "    <softbreak />\n"
                      "    <text sourcepos=\"4:1-4:6\" xml:space=\"preserve\">there </text>\n"
                      "    <code sourcepos=\"4:8-4:9\" xml:space=\"preserve\">hi</code>\n"
                      "    <text sourcepos=\"4:11-4:14\" xml:space=\"preserve\"> -- </text>\n"
                      "    <link sourcepos=\"4:15-4:41\" destination=\"www.google.com\" title=\"ok\">\n"
                      "      <text sourcepos=\"4:16-4:19\" xml:space=\"preserve\">okay</text>\n"
                      "    </link>\n"
                      "    <text sourcepos=\"4:42-4:42\" xml:space=\"preserve\">.</text>\n"
                      "  </paragraph>\n"
                      "  <block_quote sourcepos=\"6:1-10:20\">\n"
                      "    <list sourcepos=\"6:3-10:20\" type=\"ordered\" start=\"1\" delim=\"period\" tight=\"false\">\n"
                      "      <item sourcepos=\"6:3-8:1\">\n"
                      "        <paragraph sourcepos=\"6:6-7:10\">\n"
                      "          <text sourcepos=\"6:6-6:10\" xml:space=\"preserve\">Okay.</text>\n"
                      "          <softbreak />\n"
                      "          <text sourcepos=\"7:6-7:10\" xml:space=\"preserve\">Sure.</text>\n"
                      "        </paragraph>\n"
                      "      </item>\n"
                      "      <item sourcepos=\"9:3-10:20\">\n"
                      "        <paragraph sourcepos=\"9:6-10:20\">\n"
                      "          <text sourcepos=\"9:6-9:15\" xml:space=\"preserve\">Yes, okay.</text>\n"
                      "          <softbreak />\n"
                      "          <image sourcepos=\"10:6-10:20\" destination=\"hi\" title=\"yes\">\n"
                      "            <text sourcepos=\"10:8-10:9\" xml:space=\"preserve\">ok</text>\n"
                      "          </image>\n"
                      "        </paragraph>\n"
                      "      </item>\n"
                      "    </list>\n"
                      "  </block_quote>\n"
                      "</document>\n",
         "sourcepos are as expected");
  free(xml);
  cssg_node_free(doc);
}

static void source_pos_inlines(test_batch_runner *runner) {
  {
    static const char markdown[] =
      "*first*\n"
      "second\n";

    cssg_node *doc = cssg_parse_document(markdown, sizeof(markdown) - 1, CSSG_OPT_DEFAULT);
    char *xml = cssg_render_xml(doc, CSSG_OPT_DEFAULT | CSSG_OPT_SOURCEPOS);
    STR_EQ(runner, xml, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
                        "<!DOCTYPE document SYSTEM \"CommonMark.dtd\">\n"
                        "<document sourcepos=\"1:1-2:6\" xmlns=\"http://commonmark.org/xml/1.0\">\n"
                        "  <paragraph sourcepos=\"1:1-2:6\">\n"
                        "    <emph sourcepos=\"1:1-1:7\">\n"
                        "      <text sourcepos=\"1:2-1:6\" xml:space=\"preserve\">first</text>\n"
                        "    </emph>\n"
                        "    <softbreak />\n"
                        "    <text sourcepos=\"2:1-2:6\" xml:space=\"preserve\">second</text>\n"
                        "  </paragraph>\n"
                        "</document>\n",
                        "sourcepos are as expected");
    free(xml);
    cssg_node_free(doc);
  }
  {
    static const char markdown[] =
      "*first\n"
      "second*\n";

    cssg_node *doc = cssg_parse_document(markdown, sizeof(markdown) - 1, CSSG_OPT_DEFAULT);
    char *xml = cssg_render_xml(doc, CSSG_OPT_DEFAULT | CSSG_OPT_SOURCEPOS);
    STR_EQ(runner, xml, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
                        "<!DOCTYPE document SYSTEM \"CommonMark.dtd\">\n"
                        "<document sourcepos=\"1:1-2:7\" xmlns=\"http://commonmark.org/xml/1.0\">\n"
                        "  <paragraph sourcepos=\"1:1-2:7\">\n"
                        "    <emph sourcepos=\"1:1-2:7\">\n"
                        "      <text sourcepos=\"1:2-1:6\" xml:space=\"preserve\">first</text>\n"
                        "      <softbreak />\n"
                        "      <text sourcepos=\"2:1-2:6\" xml:space=\"preserve\">second</text>\n"
                        "    </emph>\n"
                        "  </paragraph>\n"
                        "</document>\n",
                        "sourcepos are as expected");
    free(xml);
    cssg_node_free(doc);
  }
  {
    static const char markdown[] =
      "` It is one backtick\n"
      "`` They are two backticks\n";

    cssg_node *doc = cssg_parse_document(markdown, sizeof(markdown) - 1, CSSG_OPT_DEFAULT);
    char *xml = cssg_render_xml(doc, CSSG_OPT_DEFAULT | CSSG_OPT_SOURCEPOS);
    STR_EQ(runner, xml, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
                        "<!DOCTYPE document SYSTEM \"CommonMark.dtd\">\n"
                        "<document sourcepos=\"1:1-2:25\" xmlns=\"http://commonmark.org/xml/1.0\">\n"
                        "  <paragraph sourcepos=\"1:1-2:25\">\n"
                        "    <text sourcepos=\"1:1-1:20\" xml:space=\"preserve\">` It is one backtick</text>\n"
                        "    <softbreak />\n"
                        "    <text sourcepos=\"2:1-2:25\" xml:space=\"preserve\">`` They are two backticks</text>\n"
                        "  </paragraph>\n"
                        "</document>\n",
                        "sourcepos are as expected");
    free(xml);
    cssg_node_free(doc);
  }
}

static void ref_source_pos(test_batch_runner *runner) {
  static const char markdown[] =
    "Let's try [reference] links.\n"
    "\n"
    "[reference]: https://github.com (GitHub)\n";

  cssg_node *doc = cssg_parse_document(markdown, sizeof(markdown) - 1, CSSG_OPT_DEFAULT);
  char *xml = cssg_render_xml(doc, CSSG_OPT_DEFAULT | CSSG_OPT_SOURCEPOS);
  STR_EQ(runner, xml, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
                      "<!DOCTYPE document SYSTEM \"CommonMark.dtd\">\n"
                      "<document sourcepos=\"1:1-3:40\" xmlns=\"http://commonmark.org/xml/1.0\">\n"
                      "  <paragraph sourcepos=\"1:1-1:28\">\n"
                      "    <text sourcepos=\"1:1-1:10\" xml:space=\"preserve\">Let's try </text>\n"
                      "    <link sourcepos=\"1:11-1:21\" destination=\"https://github.com\" title=\"GitHub\">\n"
                      "      <text sourcepos=\"1:12-1:20\" xml:space=\"preserve\">reference</text>\n"
                      "    </link>\n"
                      "    <text sourcepos=\"1:22-1:28\" xml:space=\"preserve\"> links.</text>\n"
                      "  </paragraph>\n"
                      "</document>\n",
         "sourcepos are as expected");
  free(xml);
  cssg_node_free(doc);
}

int main(void) {
  int retval;
  test_batch_runner *runner = test_batch_runner_new();

  version(runner);
  constructor(runner);
  accessors(runner);
  free_parent(runner);
  node_check(runner);
  iterator(runner);
  iterator_delete(runner);
  create_tree(runner);
  custom_nodes(runner);
  hierarchy(runner);
  parser(runner);
  render_html(runner);
  render_xml(runner);
  render_man(runner);
  render_commonmark(runner);
  utf8(runner);
  line_endings(runner);
  numeric_entities(runner);
  test_cplusplus(runner);
  test_safe(runner);
  test_feed_across_line_ending(runner);
  sub_document(runner);
  source_pos(runner);
  source_pos_inlines(runner);
  ref_source_pos(runner);

  test_print_summary(runner);
  retval = test_ok(runner) ? 0 : 1;
  free(runner);

  return retval;
}
