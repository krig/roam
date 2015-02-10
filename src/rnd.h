#pragma once
#include "common.h"

/* RNGs, hash functions etc. */

/* LCG random generator, based on stb.h */

static inline unsigned long lcg_rand(unsigned long* seed) {
	*seed = *seed * 2147001325 + 715136305; // BCPL generator
   // shuffle non-random bits to the middle, and xor to decorrelate with seed
   return 0x31415926 ^ ((*seed >> 16) + (*seed << 16));
}

static inline double lcg_frand(unsigned long* seed) {
	return lcg_rand(seed) / ((double) (1 << 16) * (1 << 16));
}

static inline unsigned long djb2_hash(unsigned char *str) {
    unsigned long hash = 5381;
    int c;
    while ((c = *str++) != 0)
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
    return hash;
}

/* RNG stream, from the OpenSimplex noise code */
static inline uint64_t rand64(uint64_t seed) {
		return seed * UINT64_C(6364136223846793005) + UINT64_C(1442695040888963407);
}

// spatially localized hash function for chunks
static inline uint64_t chunk_hash(int x, int y, int z) {
	uint64_t hash = 0;
	for (size_t i = 0; i < 21; ++i) {
		hash += (z & 1) + ((x & 1) << 1) + ((y & 1) << 2);
		x >>= 1;
		y >>= 1;
		z >>= 1;
	}
	return hash;
}
