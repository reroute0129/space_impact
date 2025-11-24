#include <stdint.h>

static uint32_t rng_state[4] = { 0u, 0u, 0u, 0u };

static uint32_t rotl32(uint32_t x, int k) {
    return (uint32_t)((x << k) | (x >> (32 - k)));
}

static uint32_t splitmix32(uint32_t* seed) {
    uint32_t z = (*seed += 0x9E3779B9u);
    z = (z ^ (z >> 16)) * 0x85EBCA6Bu;
    z = (z ^ (z >> 13)) * 0xC2B2AE35u;
    z ^= z >> 16;
    return z;
}

void rng_seed(uint32_t seed) {
    if (seed == 0u) {
        seed = 0xA3C59AC3u;
    }
    uint32_t s = seed;
    rng_state[0] = splitmix32(&s);
    rng_state[1] = splitmix32(&s);
    rng_state[2] = splitmix32(&s);
    rng_state[3] = splitmix32(&s);
}

uint32_t rng_u32(void) {
    uint32_t result = rotl32(rng_state[0] + rng_state[3], 7) + rng_state[0];

    uint32_t t = rng_state[1] << 9;

    rng_state[2] ^= rng_state[0];
    rng_state[3] ^= rng_state[1];
    rng_state[1] ^= rng_state[2];
    rng_state[0] ^= rng_state[3];
    rng_state[2] ^= t;

    rng_state[3] = rotl32(rng_state[3], 11);

    return result;
}

