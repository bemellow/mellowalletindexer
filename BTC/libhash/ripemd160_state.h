#pragma once
#include <stdint.h>

typedef struct {
  uint64_t length;
  union {
    uint32_t w[16];
    uint8_t  b[64];
  } buf;
  uint32_t h[5];
  uint8_t bufpos;
} ripemd160_state;
