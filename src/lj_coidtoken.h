#ifndef _LJ_COIDTOKEN_H_
#define _LJ_COIDTOKEN_H_

#include <string.h>

typedef struct coidtoken {
    const char * _ptr;
    const char * _pte;
} coidtoken;

static coidtoken coid_token_from_cstr(const char* s) {
    coidtoken res;
    res._ptr = s;
    res._ptr = res._ptr + strlen(s);
    return res;
}

#define coid_tok_len(t) ((uint32_t)(t._pte - t._ptr))
#define coid_tok_is_empty(t) (t._pte == t._ptr)
#endif