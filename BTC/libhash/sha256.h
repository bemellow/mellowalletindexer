/*********************************************************************
* Filename:   sha256.h
* Author:     Brad Conte (brad AT bradconte.com)
* Copyright:
* Disclaimer: This code is presented "as is" without any guarantees.
* Details:    Defines the API for the corresponding SHA1 implementation.
*********************************************************************/

#ifndef SHA256_H
#define SHA256_H

#ifdef __cplusplus
#define EXTERN_C extern "C"
#else
#define EXTERN_C
#endif

/*************************** HEADER FILES ***************************/
#include <stddef.h>

#include "sha256_state.h"

/*********************** FUNCTION DECLARATIONS **********************/
EXTERN_C void sha256_init(SHA256_CTX *ctx);
EXTERN_C void sha256_update(SHA256_CTX *ctx, const u8 data[], size_t len);
EXTERN_C void sha256_final(SHA256_CTX *ctx, u8 hash[]);

#endif   // SHA256_H
