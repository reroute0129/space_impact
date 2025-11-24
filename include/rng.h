#ifndef RNG_H
#define RNG_H

#include <stdint.h>

void rng_seed(uint32_t seed);
uint32_t rng_u32(void);

#endif

