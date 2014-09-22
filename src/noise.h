#pragma once

/*
 * A C port of "OpenSimplex (Simplectic) Noise in Java".
 * https://gist.github.com/KdotJPG/b1270127455a94ac5d19
 *
 * (v1.0.1 With new gradient set and corresponding normalization factor, 9/19/14)
 *
 * This is free and unencumbered software released into the public domain.
 *
 * Anyone is free to copy, modify, publish, use, compile, sell, or
 * distribute this software, either in source code form or as a compiled
 * binary, for any purpose, commercial or non-commercial, and by any
 * means.
 *
 * In jurisdictions that recognize copyright laws, the author or authors
 * of this software dedicate any and all copyright interest in the
 * software to the public domain. We make this dedication for the benefit
 * of the public at large and to the detriment of our heirs and
 * successors. We intend this dedication to be an overt act of
 * relinquishment in perpetuity of all present and future rights to this
 * software under copyright law.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

/* init the noise function using the default permutation table */
void osnInit();
/* init the noise function with the given 256-entry permutation table */
void osnInitPerm(const uint8_t* userperm);
/* init the noise function using the given random generator */
void osnInitRand(unsigned long (*randomFunction)(void*), void* randomState);

/* generate 3D noise data */
double osnNoise(double x, double y, double z);

/* LCG random generator, based on stb.h */
typedef struct osn_lcgrand {
	unsigned long seed;
} osn_lcgrand;

static inline void osnSRandLCG(osn_lcgrand* state, unsigned long seed) {
	state->seed = seed;
}

static inline unsigned long osnRandLCG(osn_lcgrand* state) {
	state->seed = state->seed * 2147001325 + 715136305; // BCPL generator
   // shuffle non-random bits to the middle, and xor to decorrelate with seed
   return 0x31415926 ^ ((state->seed >> 16) + (state->seed << 16));
}

static inline double osnFRandLCG(osn_lcgrand* state) {
	return osnRandLCG(state) / ((double) (1 << 16) * (1 << 16));
}

