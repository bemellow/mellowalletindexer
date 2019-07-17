#pragma once
/*************************** HEADER FILES ***************************/
#include <stdint.h>

/****************************** MACROS ******************************/
#define SHA256_BLOCK_SIZE 32            // SHA256 outputs a 32 byte digest

/**************************** DATA TYPES ****************************/
typedef uint8_t u8;
typedef uint32_t u32;
typedef uint64_t u64;

typedef struct {
	u8 data[64];
	u32 datalen;
	u64 bitlen;
	u32 state[8];
} SHA256_CTX;
