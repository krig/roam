#include "common.h"
#include "math3d.h"
#include "game.h"
#include "gen.h"
#include "rnd.h"
#include "noise.h"
#include "map.h"

static
void gen_testmap(game_chunk* chunk)
{
	int x, z, blockx, blockz, fillx, filly, fillz;
	uint32_t* blocks;
	x = chunk->x;
	z = chunk->z;
	blocks = map_blocks;

	if (abs(x) >= 2 || abs(z) >= 2)
		return;

	blockx = x * CHUNK_SIZE;
	blockz = z * CHUNK_SIZE;

	for (fillz = blockz; fillz < blockz + CHUNK_SIZE; ++fillz) {
		for (fillx = blockx; fillx < blockx + CHUNK_SIZE; ++fillx) {
			uint32_t sunlight = 0xf;
			size_t idx0 = block_index(fillx, 0, fillz);
			for (filly = MAP_BLOCK_HEIGHT-1; filly >= 0; --filly) {
				uint32_t b = BLOCK_AIR;
					if (filly <= 32)
						b = BLOCK_STONE;

				if (b != BLOCK_AIR)
					sunlight = 0;
				blocks[idx0 + filly] = b | (sunlight << 28);
			}
		}
	}

	for (int i = 0; i < NUM_BLOCKTYPES; ++i) {
		int px = (i*2) % CHUNK_SIZE;
		int pz = ((i*2) / CHUNK_SIZE) * 2;
		blocks[block_index(blockx + px, 33, blockz + pz)] = i;
	}
}

static
void gen_noisemap(game_chunk* chunk)
{
	int x, z, blockx, blockz, fillx, filly, fillz;
	uint32_t* blocks;
	x = chunk->x;
	z = chunk->z;
	blocks = map_blocks;

	blockx = x * CHUNK_SIZE;
	blockz = z * CHUNK_SIZE;

#define NOISE_SCALE (1.0/((double)CHUNK_SIZE * 16))

	uint32_t p, b;
	for (fillz = blockz; fillz < blockz + CHUNK_SIZE; ++fillz) {
		for (fillx = blockx; fillx < blockx + CHUNK_SIZE; ++fillx) {
			size_t idx0 = block_index(fillx, 0, fillz);
			if (idx0 >= MAP_BUFFER_SIZE) {
				printf("bad index: %d, %d, %d\n", fillx, 0, fillz);
				abort();
			}
			uint32_t sunlight = 0xf;
			double noise1 = fbm_simplex_2d(fillx, fillz, 0.3, NOISE_SCALE, 2.1117, 5);
			double noise2 = fbm_simplex_2d(fillx, fillz, 0.5, NOISE_SCALE*2.1331, 2.1117, 3);
			double noise3 = fbm_simplex_2d(fillx + 90.0, fillz + 90.0, 0.8, NOISE_SCALE*0.3344, 2.1117, 4);
			double height = 40.0 + ((noise1 - noise3*0.777) * 24.0 + noise2 * 24.0) * 0.5;
			double dirtdepth = 2.0 + noise2 * 8.0;
			double sanddepth = (noise1 * 6.0);
			b = BLOCK_AIR;
			p = BLOCK_AIR;

			uint32_t base = BLOCK_SOLID_DIRT;
			uint32_t dirt = BLOCK_WET_DIRT;
			uint32_t shore = BLOCK_GOLD_SAND;
			uint32_t ground = BLOCK_WET_GRASS;
			uint32_t water = BLOCK_OCEAN1;

			double depth = 0.0;

			for (filly = MAP_BLOCK_HEIGHT-1; filly >= 0; --filly) {
				if (filly < 2.0) {
					b = BLOCK_BLACKROCK;
				} else if (filly > height) {
					if (filly > OCEAN_LEVEL) {
						b = BLOCK_AIR;
					} else {
						b = water;
					}
				} else if (filly > (double)OCEAN_LEVEL + sanddepth) {
					if (p == BLOCK_AIR) {
						b = ground;
					} else if (depth > dirtdepth) {
						b = base;
					} else {
						b = dirt;
					}
				} else if (filly > (double)OCEAN_LEVEL - sanddepth) {
					if (p == BLOCK_AIR || p == shore) {
						b = shore;
					} else {
						b = base;
					}
				} else if (depth > dirtdepth) {
					b = base;
				} else {
					b = dirt;
				}
				if (b == BLOCK_AIR) {
				} else if (sunlight > 0 && (blockinfo[b].density < SOLID_DENSITY)) {
					sunlight -= 1;
				} else {
					sunlight = 0;
				}
				blocks[idx0 + filly] = b | (sunlight << 28);
				p = b;
				if (p != BLOCK_AIR && p != water)
					depth += 1.0;
			}
		}
	}
}

static
void gen_floating(struct game_map* map, game_chunk* chunk) {

	int x, z, blockx, blockz, fillx, filly, fillz;
	uint32_t* blocks;

	x = chunk->x;
	z = chunk->z;
	blocks = map_blocks;
	blockx = x * CHUNK_SIZE;
	blockz = z * CHUNK_SIZE;

	for (fillz = blockz; fillz < blockz + CHUNK_SIZE; ++fillz) {
		for (fillx = blockx; fillx < blockx + CHUNK_SIZE; ++fillx) {
			size_t idx0 = block_index(fillx, 0, fillz);
			double noise2d = fbm_simplex_2d((double)fillx / MAP_BLOCK_HEIGHT, (double)fillz / MAP_BLOCK_HEIGHT,
							0.45, 0.8, 2.0, 5);
			noise2d = (noise2d + 1.0) * 0.5;
			int groundy = (int)(40.0 * noise2d) + 40;

			int watery = 50;

			uint32_t p = BLOCK_AIR;
			uint32_t sunlight = 0xf;

			for (filly = MAP_BLOCK_HEIGHT-1; filly >= 0; --filly) {
				uint32_t b = BLOCK_AIR;
				if (filly < watery && (sunlight || p == BLOCK_OCEAN3)) {
					b = BLOCK_OCEAN3;
				}

				if (filly > 16.0 && filly < groundy) {
					double gradient = (double)filly / (double)MAP_BLOCK_HEIGHT;
					double density = opensimplex_noise_3d(
						(double)fillx / (double)MAP_BLOCK_HEIGHT * 15.0,
						(double)filly / (double)MAP_BLOCK_HEIGHT * 15.0,
						(double)fillz / (double)MAP_BLOCK_HEIGHT * 15.0);

					double density01 = (density * 0.5) + 0.5;

					if (density01 + (1.0 - gradient) < 0.8) {
					} else if (filly < groundy) {
						int dirt_depth = 2 + (rand64(fillx ^ filly ^ fillz) % 5);
						if (sunlight && (fabs(filly - watery - (density * 3.0)) < 1.5)) {
							b = BLOCK_GOLD_SAND;
						} else if (sunlight == 0xf) {
							b = BLOCK_WET_GRASS;
						} else if (groundy - filly > dirt_depth) {
							b = BLOCK_DEEP_ROCK;
						} else {
							b = BLOCK_WET_DIRT;
						}
					}
				} else if (filly < groundy) {
					b = BLOCK_WET_DIRT;
				}
				if (b == BLOCK_AIR) {
				} else if (sunlight > 0 && (blockinfo[b].density < SOLID_DENSITY)) {
					sunlight--;
				} else {
					sunlight = 0;
				}
				blocks[idx0 + filly] = b | (sunlight << 28);
				p = b;
			}
		}
	}

	int nitems = rand64(blockx + (blockz << 5)) % 10;
	if (nitems > 6) {
		int x = rand64((blockz << 5) + blockx) % CHUNK_SIZE;
		int z = rand64(blockx ^ blockz) % CHUNK_SIZE;
		int y = MAP_BLOCK_HEIGHT-1;
		size_t idx0 = block_index(blockx + x, 0, blockz + z);
		while (y && ((blocks[idx0 + y] >> 28) & 0xf)) {
			--y;
		}
		if (y + 1 < MAP_BLOCK_HEIGHT) {
			blocks[idx0 + y + 1] = BLOCK_MELON;
		}
	}
}

// light propagation has to happen
// after all block generation is completed

// propagate light from x,y,z
// so (x,y,z) is the light "source"
//   push (x,y,z) to process queue
//   check its neighbours
//   if neighbour is !solid and...
//      has lightlevel < this - 2,
//       increase their lightlevel
//       add that neighbour to propagation queue
//   loop until queue is empty
//   neighbour data can be packed into uint8[3]
// TODO: per-thread queue, queue retesselation of
// lit chunks as they are modified

void propagate_light(int x, int y, int z)
{
	// push light propagation to queue
}

void process_light_propagation()
{
}

void gen_loadchunk(struct game_map* map, game_chunk* chunk)
{
	//gen_testmap(chunk);
	//gen_noisemap(chunk);
	gen_floating(map, chunk);
}
