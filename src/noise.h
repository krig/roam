#pragma once

#include "stb_perlin.h"
#include "common.h"

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

/* init the noise function using the given seed */
void opensimplex_init(uint64_t seed);

/* generate 2D, 3D, 4D noise data */
double opensimplex_noise_2d(double x, double y);
double opensimplex_noise_3d(double x, double y, double z);
double opensimplex_noise_4d(double x, double y, double z, double w);

/*
 * A speed-improved simplex noise algorithm for 2D
 *
 * Based on example code by Stefan Gustavson (stegu@itn.liu.se).
 * Optimisations by Peter Eastman (peastman@drizzle.stanford.edu).
 * Better rank ordering method by Stefan Gustavson in 2012.
 *
 * This could be speeded up even further, but it's useful as it is.
 *
 * Version 2012-03-09
 *
 * This code was placed in the public domain by its original author,
 * Stefan Gustavson. You may use it as you see fit, but
 * attribution is appreciated.
 *
 */

void simplex_init(uint64_t seed);
double simplex_noise_2d(double x, double y);


static inline
double fbm_simplex_2d(double x, double y, double gain, double frequency, double lacunarity, int octaves)
{
	double sum = 0.0;
	double amplitude = 1.0;
	for (int i = 0; i < octaves; ++i) {
		sum += simplex_noise_2d(x * frequency, y * frequency) * amplitude;
        frequency *= lacunarity;
        amplitude *= gain;
	}
	return sum;
}
