#ifndef CSSG_H
#define CSSG_H

#include <stdio.h>
#include <cssg_export.h>
#include <cssg_version.h>

#ifdef __cplusplus
extern "C" {
#endif

/** # NAME
 *
 * **cssg** - CommonMark parsing, manipulating, and rendering
 */

/** # DESCRIPTION
 *
 * ## Simple Interface
 */

/** Convert 'text' (assumed to be a UTF-8 encoded string with length
 * 'len') from CommonMark Markdown to HTML, returning a null-terminated,
 * UTF-8-encoded string. It is the caller's responsibility
 * to free the returned buffer.
 */
CSSG_EXPORT
char *cssg_markdown_to_html(const char *text, size_t len, int options);

/** ## Node Structure
 */

typedef enum {
  /* Error status */
  CSSG_NODE_NONE,

  /* Block */
  CSSG_NODE_DOCUMENT,
  CSSG_NODE_BLOCK_QUOTE,
  CSSG_NODE_LIST,
  CSSG_NODE_ITEM,
  CSSG_NODE_CODE_BLOCK,
  CSSG_NODE_HTML_BLOCK,
  CSSG_NODE_CUSTOM_BLOCK,
  CSSG_NODE_PARAGRAPH,
  CSSG_NODE_HEADING,
  CSSG_NODE_THEMATIC_BREAK,

  CSSG_NODE_FIRST_BLOCK = CSSG_NODE_DOCUMENT,
  CSSG_NODE_LAST_BLOCK = CSSG_NODE_THEMATIC_BREAK,

  /* Inline */
  CSSG_NODE_TEXT,
  CSSG_NODE_SOFTBREAK,
  CSSG_NODE_LINEBREAK,
  CSSG_NODE_CODE,
  CSSG_NODE_HTML_INLINE,
  CSSG_NODE_CUSTOM_INLINE,
  CSSG_NODE_EMPH,
  CSSG_NODE_STRONG,
  CSSG_NODE_LINK,
  CSSG_NODE_IMAGE,

  CSSG_NODE_FIRST_INLINE = CSSG_NODE_TEXT,
  CSSG_NODE_LAST_INLINE = CSSG_NODE_IMAGE
} cssg_node_type;

/* For backwards compatibility: */
#define CSSG_NODE_HEADER CSSG_NODE_HEADING
#define CSSG_NODE_HRULE CSSG_NODE_THEMATIC_BREAK
#define CSSG_NODE_HTML CSSG_NODE_HTML_BLOCK
#define CSSG_NODE_INLINE_HTML CSSG_NODE_HTML_INLINE

typedef enum {
  CSSG_NO_LIST,
  CSSG_BULLET_LIST,
  CSSG_ORDERED_LIST
} cssg_list_type;

typedef enum {
  CSSG_NO_DELIM,
  CSSG_PERIOD_DELIM,
  CSSG_PAREN_DELIM
} cssg_delim_type;

typedef struct cssg_node cssg_node;
typedef struct cssg_parser cssg_parser;
typedef struct cssg_iter cssg_iter;

/**
 * ## Custom memory allocator support
 */

/** Defines the memory allocation functions to be used by Cssg
 * when parsing and allocating a document tree
 */
typedef struct cssg_mem {
  void *(*calloc)(size_t, size_t);
  void *(*realloc)(void *, size_t);
  void (*free)(void *);
} cssg_mem;

/** Returns a pointer to the default memory allocator.
 */
CSSG_EXPORT cssg_mem *cssg_get_default_mem_allocator(void);

/**
 * ## Creating and Destroying Nodes
 */

/** Creates a new node of type 'type'.  Note that the node may have
 * other required properties, which it is the caller's responsibility
 * to assign.
 */
CSSG_EXPORT cssg_node *cssg_node_new(cssg_node_type type);

/** Same as `cssg_node_new`, but explicitly listing the memory
 * allocator used to allocate the node.  Note:  be sure to use the same
 * allocator for every node in a tree, or bad things can happen.
 */
CSSG_EXPORT cssg_node *cssg_node_new_with_mem(cssg_node_type type,
                                                 cssg_mem *mem);

/** Frees the memory allocated for a node and any children.
 */
CSSG_EXPORT void cssg_node_free(cssg_node *node);

/**
 * ## Tree Traversal
 */

/** Returns the next node in the sequence after 'node', or NULL if
 * there is none.
 */
CSSG_EXPORT cssg_node *cssg_node_next(cssg_node *node);

/** Returns the previous node in the sequence after 'node', or NULL if
 * there is none.
 */
CSSG_EXPORT cssg_node *cssg_node_previous(cssg_node *node);

/** Returns the parent of 'node', or NULL if there is none.
 */
CSSG_EXPORT cssg_node *cssg_node_parent(cssg_node *node);

/** Returns the first child of 'node', or NULL if 'node' has no children.
 */
CSSG_EXPORT cssg_node *cssg_node_first_child(cssg_node *node);

/** Returns the last child of 'node', or NULL if 'node' has no children.
 */
CSSG_EXPORT cssg_node *cssg_node_last_child(cssg_node *node);

/**
 * ## Iterator
 *
 * An iterator will walk through a tree of nodes, starting from a root
 * node, returning one node at a time, together with information about
 * whether the node is being entered or exited.  The iterator will
 * first descend to a child node, if there is one.  When there is no
 * child, the iterator will go to the next sibling.  When there is no
 * next sibling, the iterator will return to the parent (but with
 * a 'cssg_event_type' of `CSSG_EVENT_EXIT`).  The iterator will
 * return `CSSG_EVENT_DONE` when it reaches the root node again.
 * One natural application is an HTML renderer, where an `ENTER` event
 * outputs an open tag and an `EXIT` event outputs a close tag.
 * An iterator might also be used to transform an AST in some systematic
 * way, for example, turning all level-3 headings into regular paragraphs.
 *
 *     void
 *     usage_example(cssg_node *root) {
 *         cssg_event_type ev_type;
 *         cssg_iter *iter = cssg_iter_new(root);
 *
 *         while ((ev_type = cssg_iter_next(iter)) != CSSG_EVENT_DONE) {
 *             cssg_node *cur = cssg_iter_get_node(iter);
 *             // Do something with `cur` and `ev_type`
 *         }
 *
 *         cssg_iter_free(iter);
 *     }
 *
 * Iterators will never return `EXIT` events for leaf nodes, which are nodes
 * of type:
 *
 * * CSSG_NODE_HTML_BLOCK
 * * CSSG_NODE_THEMATIC_BREAK
 * * CSSG_NODE_CODE_BLOCK
 * * CSSG_NODE_TEXT
 * * CSSG_NODE_SOFTBREAK
 * * CSSG_NODE_LINEBREAK
 * * CSSG_NODE_CODE
 * * CSSG_NODE_HTML_INLINE
 *
 * Nodes must only be modified after an `EXIT` event, or an `ENTER` event for
 * leaf nodes.
 */

typedef enum {
  CSSG_EVENT_NONE,
  CSSG_EVENT_DONE,
  CSSG_EVENT_ENTER,
  CSSG_EVENT_EXIT
} cssg_event_type;

/** Creates a new iterator starting at 'root'.  The current node and event
 * type are undefined until 'cssg_iter_next' is called for the first time.
 * The memory allocated for the iterator should be released using
 * 'cssg_iter_free' when it is no longer needed.
 */
CSSG_EXPORT
cssg_iter *cssg_iter_new(cssg_node *root);

/** Frees the memory allocated for an iterator.
 */
CSSG_EXPORT
void cssg_iter_free(cssg_iter *iter);

/** Advances to the next node and returns the event type (`CSSG_EVENT_ENTER`,
 * `CSSG_EVENT_EXIT` or `CSSG_EVENT_DONE`).
 */
CSSG_EXPORT
cssg_event_type cssg_iter_next(cssg_iter *iter);

/** Returns the current node.
 */
CSSG_EXPORT
cssg_node *cssg_iter_get_node(cssg_iter *iter);

/** Returns the current event type.
 */
CSSG_EXPORT
cssg_event_type cssg_iter_get_event_type(cssg_iter *iter);

/** Returns the root node.
 */
CSSG_EXPORT
cssg_node *cssg_iter_get_root(cssg_iter *iter);

/** Resets the iterator so that the current node is 'current' and
 * the event type is 'event_type'.  The new current node must be a
 * descendant of the root node or the root node itself.
 */
CSSG_EXPORT
void cssg_iter_reset(cssg_iter *iter, cssg_node *current,
                      cssg_event_type event_type);

/**
 * ## Accessors
 */

/** Returns the user data of 'node'.
 */
CSSG_EXPORT void *cssg_node_get_user_data(cssg_node *node);

/** Sets arbitrary user data for 'node'.  Returns 1 on success,
 * 0 on failure.
 */
CSSG_EXPORT int cssg_node_set_user_data(cssg_node *node, void *user_data);

/** Returns the type of 'node', or `CSSG_NODE_NONE` on error.
 */
CSSG_EXPORT cssg_node_type cssg_node_get_type(cssg_node *node);

/** Like 'cssg_node_get_type', but returns a string representation
    of the type, or `"<unknown>"`.
 */
CSSG_EXPORT
const char *cssg_node_get_type_string(cssg_node *node);

/** Returns the string contents of 'node', or an empty
    string if none is set.  Returns NULL if called on a
    node that does not have string content.
 */
CSSG_EXPORT const char *cssg_node_get_literal(cssg_node *node);

/** Sets the string contents of 'node'.  Returns 1 on success,
 * 0 on failure.
 */
CSSG_EXPORT int cssg_node_set_literal(cssg_node *node, const char *content);

/** Returns the heading level of 'node', or 0 if 'node' is not a heading.
 */
CSSG_EXPORT int cssg_node_get_heading_level(cssg_node *node);

/* For backwards compatibility */
#define cssg_node_get_header_level cssg_node_get_heading_level
#define cssg_node_set_header_level cssg_node_set_heading_level

/** Sets the heading level of 'node', returning 1 on success and 0 on error.
 */
CSSG_EXPORT int cssg_node_set_heading_level(cssg_node *node, int level);

/** Returns the list type of 'node', or `CSSG_NO_LIST` if 'node'
 * is not a list.
 */
CSSG_EXPORT cssg_list_type cssg_node_get_list_type(cssg_node *node);

/** Sets the list type of 'node', returning 1 on success and 0 on error.
 */
CSSG_EXPORT int cssg_node_set_list_type(cssg_node *node,
                                          cssg_list_type type);

/** Returns the list delimiter type of 'node', or `CSSG_NO_DELIM` if 'node'
 * is not a list.
 */
CSSG_EXPORT cssg_delim_type cssg_node_get_list_delim(cssg_node *node);

/** Sets the list delimiter type of 'node', returning 1 on success and 0
 * on error.
 */
CSSG_EXPORT int cssg_node_set_list_delim(cssg_node *node,
                                           cssg_delim_type delim);

/** Returns starting number of 'node', if it is an ordered list, otherwise 0.
 */
CSSG_EXPORT int cssg_node_get_list_start(cssg_node *node);

/** Sets starting number of 'node', if it is an ordered list. Returns 1
 * on success, 0 on failure.
 */
CSSG_EXPORT int cssg_node_set_list_start(cssg_node *node, int start);

/** Returns 1 if 'node' is a tight list, 0 otherwise.
 */
CSSG_EXPORT int cssg_node_get_list_tight(cssg_node *node);

/** Sets the "tightness" of a list.  Returns 1 on success, 0 on failure.
 */
CSSG_EXPORT int cssg_node_set_list_tight(cssg_node *node, int tight);

/** Returns the info string from a fenced code block.
 */
CSSG_EXPORT const char *cssg_node_get_fence_info(cssg_node *node);

/** Sets the info string in a fenced code block, returning 1 on
 * success and 0 on failure.
 */
CSSG_EXPORT int cssg_node_set_fence_info(cssg_node *node, const char *info);

/** Returns the URL of a link or image 'node', or an empty string
    if no URL is set.  Returns NULL if called on a node that is
    not a link or image.
 */
CSSG_EXPORT const char *cssg_node_get_url(cssg_node *node);

/** Sets the URL of a link or image 'node'. Returns 1 on success,
 * 0 on failure.
 */
CSSG_EXPORT int cssg_node_set_url(cssg_node *node, const char *url);

/** Returns the title of a link or image 'node', or an empty
    string if no title is set.  Returns NULL if called on a node
    that is not a link or image.
 */
CSSG_EXPORT const char *cssg_node_get_title(cssg_node *node);

/** Sets the title of a link or image 'node'. Returns 1 on success,
 * 0 on failure.
 */
CSSG_EXPORT int cssg_node_set_title(cssg_node *node, const char *title);

/** Returns the literal "on enter" text for a custom 'node', or
    an empty string if no on_enter is set.  Returns NULL if called
    on a non-custom node.
 */
CSSG_EXPORT const char *cssg_node_get_on_enter(cssg_node *node);

/** Sets the literal text to render "on enter" for a custom 'node'.
    Any children of the node will be rendered after this text.
    Returns 1 on success 0 on failure.
 */
CSSG_EXPORT int cssg_node_set_on_enter(cssg_node *node,
                                         const char *on_enter);

/** Returns the literal "on exit" text for a custom 'node', or
    an empty string if no on_exit is set.  Returns NULL if
    called on a non-custom node.
 */
CSSG_EXPORT const char *cssg_node_get_on_exit(cssg_node *node);

/** Sets the literal text to render "on exit" for a custom 'node'.
    Any children of the node will be rendered before this text.
    Returns 1 on success 0 on failure.
 */
CSSG_EXPORT int cssg_node_set_on_exit(cssg_node *node, const char *on_exit);

/** Returns the line on which 'node' begins.
 */
CSSG_EXPORT int cssg_node_get_start_line(cssg_node *node);

/** Returns the column at which 'node' begins.
 */
CSSG_EXPORT int cssg_node_get_start_column(cssg_node *node);

/** Returns the line on which 'node' ends.
 */
CSSG_EXPORT int cssg_node_get_end_line(cssg_node *node);

/** Returns the column at which 'node' ends.
 */
CSSG_EXPORT int cssg_node_get_end_column(cssg_node *node);

/**
 * ## Tree Manipulation
 */

/** Unlinks a 'node', removing it from the tree, but not freeing its
 * memory.  (Use 'cssg_node_free' for that.)
 */
CSSG_EXPORT void cssg_node_unlink(cssg_node *node);

/** Inserts 'sibling' before 'node'.  Returns 1 on success, 0 on failure.
 */
CSSG_EXPORT int cssg_node_insert_before(cssg_node *node,
                                          cssg_node *sibling);

/** Inserts 'sibling' after 'node'. Returns 1 on success, 0 on failure.
 */
CSSG_EXPORT int cssg_node_insert_after(cssg_node *node, cssg_node *sibling);

/** Replaces 'oldnode' with 'newnode' and unlinks 'oldnode' (but does
 * not free its memory).
 * Returns 1 on success, 0 on failure.
 */
CSSG_EXPORT int cssg_node_replace(cssg_node *oldnode, cssg_node *newnode);

/** Adds 'child' to the beginning of the children of 'node'.
 * Returns 1 on success, 0 on failure.
 */
CSSG_EXPORT int cssg_node_prepend_child(cssg_node *node, cssg_node *child);

/** Adds 'child' to the end of the children of 'node'.
 * Returns 1 on success, 0 on failure.
 */
CSSG_EXPORT int cssg_node_append_child(cssg_node *node, cssg_node *child);

/** Consolidates adjacent text nodes.
 */
CSSG_EXPORT void cssg_consolidate_text_nodes(cssg_node *root);

/**
 * ## Parsing
 *
 * Simple interface:
 *
 *     cssg_node *document = cssg_parse_document("Hello *world*", 13,
 *                                                 CSSG_OPT_DEFAULT);
 *
 * Streaming interface:
 *
 *     cssg_parser *parser = cssg_parser_new(CSSG_OPT_DEFAULT);
 *     FILE *fp = fopen("myfile.md", "rb");
 *     while ((bytes = fread(buffer, 1, sizeof(buffer), fp)) > 0) {
 *         cssg_parser_feed(parser, buffer, bytes);
 *         if (bytes < sizeof(buffer)) {
 *             break;
 *         }
 *     }
 *     document = cssg_parser_finish(parser);
 *     cssg_parser_free(parser);
 */

/** Creates a new parser object.
 */
CSSG_EXPORT
cssg_parser *cssg_parser_new(int options);

/** Creates a new parser object with the given memory allocator
 *
 * A generalization of `cssg_parser_new`:
 * ```c
 * cssg_parser_new(options)
 * ```
 * is the same as:
 * ```c
 * cssg_parser_new_with_mem(options, cssg_get_default_mem_allocator())
 * ```
 */
CSSG_EXPORT
cssg_parser *cssg_parser_new_with_mem(int options, cssg_mem *mem);

/** Creates a new parser object with the given node to use as the root
 * node of the parsed AST.
 *
 * When parsing, children are always appended, not prepended; that means
 * if `root` already has children, the newly-parsed children will appear
 * after the given children.
 *
 * A generalization of `cssg_parser_new_with_mem`:
 * ```c
 * cssg_parser_new_with_mem(options, mem)
 * ```
 * is approximately the same as:
 * ```c
 * cssg_parser_new_with_mem_into_root(options, mem, cssg_node_new(CSSG_NODE_DOCUMENT))
 * ```
 *
 * This is useful for creating a single document out of multiple parsed
 * document fragments.
 */
CSSG_EXPORT
cssg_parser *cssg_parser_new_with_mem_into_root(
    int options, cssg_mem *mem, cssg_node *root);

/** Frees memory allocated for a parser object.
 */
CSSG_EXPORT
void cssg_parser_free(cssg_parser *parser);

/** Feeds a string of length 'len' to 'parser'.
 */
CSSG_EXPORT
void cssg_parser_feed(cssg_parser *parser, const char *buffer, size_t len);

/** Finish parsing and return a pointer to a tree of nodes.
 */
CSSG_EXPORT
cssg_node *cssg_parser_finish(cssg_parser *parser);

/** Parse a CommonMark document in 'buffer' of length 'len'.
 * Returns a pointer to a tree of nodes.  The memory allocated for
 * the node tree should be released using 'cssg_node_free'
 * when it is no longer needed.
 */
CSSG_EXPORT
cssg_node *cssg_parse_document(const char *buffer, size_t len, int options);

/** Parse a CommonMark document in file 'f', returning a pointer to
 * a tree of nodes.  The memory allocated for the node tree should be
 * released using 'cssg_node_free' when it is no longer needed.
 */
CSSG_EXPORT
cssg_node *cssg_parse_file(FILE *f, int options);

/**
 * ## Rendering
 */

/** Render a 'node' tree as XML.  It is the caller's responsibility
 * to free the returned buffer.
 */
CSSG_EXPORT
char *cssg_render_xml(cssg_node *root, int options);

/** Render a 'node' tree as an HTML fragment.  It is up to the user
 * to add an appropriate header and footer. It is the caller's
 * responsibility to free the returned buffer.
 */
CSSG_EXPORT
char *cssg_render_html(cssg_node *root, int options);

/** Render a 'node' tree as a groff man page, without the header.
 * It is the caller's responsibility to free the returned buffer.
 */
CSSG_EXPORT
char *cssg_render_man(cssg_node *root, int options, int width);

/** Render a 'node' tree as a commonmark document.
 * It is the caller's responsibility to free the returned buffer.
 */
CSSG_EXPORT
char *cssg_render_commonmark(cssg_node *root, int options, int width);

/**
 * ## Options
 */

/** Default options.
 */
#define CSSG_OPT_DEFAULT 0

/**
 * ### Options affecting rendering
 */

/** Include a `data-sourcepos` attribute on all block elements.
 */
#define CSSG_OPT_SOURCEPOS (1 << 1)

/** Render `softbreak` elements as hard line breaks.
 */
#define CSSG_OPT_HARDBREAKS (1 << 2)

/** `CSSG_OPT_SAFE` is defined here for API compatibility,
    but it no longer has any effect. "Safe" mode is now the default:
    set `CSSG_OPT_UNSAFE` to disable it.
 */
#define CSSG_OPT_SAFE (1 << 3)

/** Render raw HTML and unsafe links (`javascript:`, `vbscript:`,
 * `file:`, and `data:`, except for `image/png`, `image/gif`,
 * `image/jpeg`, or `image/webp` mime types).  By default,
 * raw HTML is replaced by a placeholder HTML comment. Unsafe
 * links are replaced by empty strings.
 */
#define CSSG_OPT_UNSAFE (1 << 17)

/** Render `softbreak` elements as spaces.
 */
#define CSSG_OPT_NOBREAKS (1 << 4)

/**
 * ### Options affecting parsing
 */

/** Legacy option (no effect).
 */
#define CSSG_OPT_NORMALIZE (1 << 8)

/** Validate UTF-8 in the input before parsing, replacing illegal
 * sequences with the replacement character U+FFFD.
 */
#define CSSG_OPT_VALIDATE_UTF8 (1 << 9)

/** Convert straight quotes to curly, `---` to em dashes, `--` to en dashes.
 */
#define CSSG_OPT_SMART (1 << 10)

/**
 * ## Version information
 */

/** The library version as integer for runtime checks. Also available as
 * macro CSSG_VERSION for compile time checks.
 *
 * * Bits 16-23 contain the major version.
 * * Bits 8-15 contain the minor version.
 * * Bits 0-7 contain the patchlevel.
 *
 * In hexadecimal format, the number 0x010203 represents version 1.2.3.
 */
CSSG_EXPORT
int cssg_version(void);

/** The library version string for runtime checks. Also available as
 * macro CSSG_VERSION_STRING for compile time checks.
 */
CSSG_EXPORT
const char *cssg_version_string(void);

/** # AUTHORS
 *
 * John MacFarlane, Vicent Marti,  Kārlis Gaņģis, Nick Wellnhofer.
 */

#ifndef CSSG_NO_SHORT_NAMES
#define NODE_DOCUMENT CSSG_NODE_DOCUMENT
#define NODE_BLOCK_QUOTE CSSG_NODE_BLOCK_QUOTE
#define NODE_LIST CSSG_NODE_LIST
#define NODE_ITEM CSSG_NODE_ITEM
#define NODE_CODE_BLOCK CSSG_NODE_CODE_BLOCK
#define NODE_HTML_BLOCK CSSG_NODE_HTML_BLOCK
#define NODE_CUSTOM_BLOCK CSSG_NODE_CUSTOM_BLOCK
#define NODE_PARAGRAPH CSSG_NODE_PARAGRAPH
#define NODE_HEADING CSSG_NODE_HEADING
#define NODE_HEADER CSSG_NODE_HEADER
#define NODE_THEMATIC_BREAK CSSG_NODE_THEMATIC_BREAK
#define NODE_HRULE CSSG_NODE_HRULE
#define NODE_TEXT CSSG_NODE_TEXT
#define NODE_SOFTBREAK CSSG_NODE_SOFTBREAK
#define NODE_LINEBREAK CSSG_NODE_LINEBREAK
#define NODE_CODE CSSG_NODE_CODE
#define NODE_HTML_INLINE CSSG_NODE_HTML_INLINE
#define NODE_CUSTOM_INLINE CSSG_NODE_CUSTOM_INLINE
#define NODE_EMPH CSSG_NODE_EMPH
#define NODE_STRONG CSSG_NODE_STRONG
#define NODE_LINK CSSG_NODE_LINK
#define NODE_IMAGE CSSG_NODE_IMAGE
#define BULLET_LIST CSSG_BULLET_LIST
#define ORDERED_LIST CSSG_ORDERED_LIST
#define PERIOD_DELIM CSSG_PERIOD_DELIM
#define PAREN_DELIM CSSG_PAREN_DELIM
#endif

#ifdef __cplusplus
}
#endif

#endif
