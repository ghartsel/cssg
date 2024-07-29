#ifndef CSSG_CSSG_CTYPE_H
#define CSSG_CSSG_CTYPE_H

#ifdef __cplusplus
extern "C" {
#endif

/** Locale-independent versions of functions from ctype.h.
 * We want cssg to behave the same no matter what the system locale.
 */

int cssg_isspace(char c);

int cssg_ispunct(char c);

int cssg_isdigit(char c);

int cssg_isalpha(char c);

#ifdef __cplusplus
}
#endif

#endif
