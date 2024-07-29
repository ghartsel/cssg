#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>

#include "node.h"
#include "cssg.h"
#include "iterator.h"

static const int S_leaf_mask =
    (1 << CSSG_NODE_HTML_BLOCK) | (1 << CSSG_NODE_THEMATIC_BREAK) |
    (1 << CSSG_NODE_CODE_BLOCK) | (1 << CSSG_NODE_TEXT) |
    (1 << CSSG_NODE_SOFTBREAK) | (1 << CSSG_NODE_LINEBREAK) |
    (1 << CSSG_NODE_CODE) | (1 << CSSG_NODE_HTML_INLINE);

cssg_iter *cssg_iter_new(cssg_node *root) {
  if (root == NULL) {
    return NULL;
  }
  cssg_mem *mem = root->mem;
  cssg_iter *iter = (cssg_iter *)mem->calloc(1, sizeof(cssg_iter));
  iter->mem = mem;
  iter->root = root;
  iter->cur.ev_type = CSSG_EVENT_NONE;
  iter->cur.node = NULL;
  iter->next.ev_type = CSSG_EVENT_ENTER;
  iter->next.node = root;
  return iter;
}

void cssg_iter_free(cssg_iter *iter) { iter->mem->free(iter); }

static bool S_is_leaf(cssg_node *node) {
  return ((1 << node->type) & S_leaf_mask) != 0;
}

cssg_event_type cssg_iter_next(cssg_iter *iter) {
  cssg_event_type ev_type = iter->next.ev_type;
  cssg_node *node = iter->next.node;

  iter->cur.ev_type = ev_type;
  iter->cur.node = node;

  if (ev_type == CSSG_EVENT_DONE) {
    return ev_type;
  }

  /* roll forward to next item, setting both fields */
  if (ev_type == CSSG_EVENT_ENTER && !S_is_leaf(node)) {
    if (node->first_child == NULL) {
      /* stay on this node but exit */
      iter->next.ev_type = CSSG_EVENT_EXIT;
    } else {
      iter->next.ev_type = CSSG_EVENT_ENTER;
      iter->next.node = node->first_child;
    }
  } else if (node == iter->root) {
    /* don't move past root */
    iter->next.ev_type = CSSG_EVENT_DONE;
    iter->next.node = NULL;
  } else if (node->next) {
    iter->next.ev_type = CSSG_EVENT_ENTER;
    iter->next.node = node->next;
  } else if (node->parent) {
    iter->next.ev_type = CSSG_EVENT_EXIT;
    iter->next.node = node->parent;
  } else {
    assert(false);
    iter->next.ev_type = CSSG_EVENT_DONE;
    iter->next.node = NULL;
  }

  return ev_type;
}

void cssg_iter_reset(cssg_iter *iter, cssg_node *current,
                      cssg_event_type event_type) {
  iter->next.ev_type = event_type;
  iter->next.node = current;
  cssg_iter_next(iter);
}

cssg_node *cssg_iter_get_node(cssg_iter *iter) { return iter->cur.node; }

cssg_event_type cssg_iter_get_event_type(cssg_iter *iter) {
  return iter->cur.ev_type;
}

cssg_node *cssg_iter_get_root(cssg_iter *iter) { return iter->root; }

void cssg_consolidate_text_nodes(cssg_node *root) {
  if (root == NULL) {
    return;
  }
  cssg_iter *iter = cssg_iter_new(root);
  cssg_strbuf buf = CSSG_BUF_INIT(iter->mem);
  cssg_event_type ev_type;
  cssg_node *cur, *tmp, *next;

  while ((ev_type = cssg_iter_next(iter)) != CSSG_EVENT_DONE) {
    cur = cssg_iter_get_node(iter);
    if (ev_type == CSSG_EVENT_ENTER && cur->type == CSSG_NODE_TEXT &&
        cur->next && cur->next->type == CSSG_NODE_TEXT) {
      cssg_strbuf_clear(&buf);
      cssg_strbuf_put(&buf, cur->data, cur->len);
      tmp = cur->next;
      while (tmp && tmp->type == CSSG_NODE_TEXT) {
        cssg_iter_next(iter); // advance pointer
        cssg_strbuf_put(&buf, tmp->data, tmp->len);
        cur->end_column = tmp->end_column;
        next = tmp->next;
        cssg_node_free(tmp);
        tmp = next;
      }
      iter->mem->free(cur->data);
      cur->len = buf.size;
      cur->data = cssg_strbuf_detach(&buf);
    }
  }

  cssg_strbuf_free(&buf);
  cssg_iter_free(iter);
}
