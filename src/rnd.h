#pragma once

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
static inline uint64_t osnRand(uint64_t seed) {
		return seed * UINT64_C(6364136223846793005) + UINT64_C(1442695040888963407);
}

uint64_t good_seed();
