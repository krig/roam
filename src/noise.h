#pragma once

void initOpenSimplexNoise();

void initOpenSimplexNoisePerm(const uint8_t* userperm);

/*
  Initializes the class using a permutation array generated from a 64-bit seed.
  Generates a proper permutation (i.e. doesn't merely perform N successive pair swaps on a base array)
  Needs a source of randomness
*/
void initOpenSimplexNoiseRand(int (*randomFunction)(void*), void* randomState);

double openSimplexNoise(double x, double y, double z);
