#ifndef CSSG_INLINES_H
#define CSSG_INLINES_H

#include "chunk.h"
#include "references.h"

#ifdef __cplusplus
extern "C" {
#endif

unsigned char *cssg_clean_url(cssg_mem *mem, cssg_chunk *url);
unsigned char *cssg_clean_title(cssg_mem *mem, cssg_chunk *title);

void cssg_parse_inlines(cssg_mem *mem, cssg_node *parent,
                         cssg_reference_map *refmap, int options);

bufsize_t cssg_parse_reference_inline(cssg_mem *mem, cssg_chunk *input,
                                       cssg_reference_map *refmap);

#ifdef __cplusplus
}
#endif

#endif
