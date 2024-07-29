#ifndef CSSG_REFERENCES_H
#define CSSG_REFERENCES_H

#include "chunk.h"

#ifdef __cplusplus
extern "C" {
#endif

struct cssg_reference {
  struct cssg_reference *next;
  unsigned char *label;
  unsigned char *url;
  unsigned char *title;
  unsigned int age;
  unsigned int size;
};

typedef struct cssg_reference cssg_reference;

struct cssg_reference_map {
  cssg_mem *mem;
  cssg_reference *refs;
  cssg_reference **sorted;
  unsigned int size;
  unsigned int ref_size;
  unsigned int max_ref_size;
};

typedef struct cssg_reference_map cssg_reference_map;

cssg_reference_map *cssg_reference_map_new(cssg_mem *mem);
void cssg_reference_map_free(cssg_reference_map *map);
cssg_reference *cssg_reference_lookup(cssg_reference_map *map,
                                        cssg_chunk *label);
void cssg_reference_create(cssg_reference_map *map, cssg_chunk *label,
                            cssg_chunk *url, cssg_chunk *title);

#ifdef __cplusplus
}
#endif

#endif
