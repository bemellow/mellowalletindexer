#ifndef _RIPEMD160_H_
#define _RIPEMD160_H_

#ifdef __cplusplus
#define EXTERN_C extern "C"
#else
#define EXTERN_C
#endif

#include "ripemd160_state.h"
#include <stddef.h>

EXTERN_C void ripemd160_init(ripemd160_state * md);
EXTERN_C void ripemd160_process(ripemd160_state * md, const void *in, size_t inlen);
EXTERN_C void ripemd160_done(ripemd160_state * md, void *out);
EXTERN_C void simple_ripemd160(const void *in, size_t inlen, void *out);

#endif
