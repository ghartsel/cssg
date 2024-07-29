#ifndef CSSG_ITERATOR_H
#define CSSG_ITERATOR_H

#ifdef __cplusplus
extern "C" {
#endif

#include "cssg.h"

typedef struct {
  cssg_event_type ev_type;
  cssg_node *node;
} cssg_iter_state;

struct cssg_iter {
  cssg_mem *mem;
  cssg_node *root;
  cssg_iter_state cur;
  cssg_iter_state next;
};

#ifdef __cplusplus
}
#endif

#endif
