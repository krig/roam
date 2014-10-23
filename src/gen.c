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
			uint32_t sunlight = 0xf0000000;
			size_t idx0 = blockIndex(fillx, 0, fillz);
			for (filly = MAP_BLOCK_HEIGHT-1; filly >= 0; --filly) {
				uint32_t b = BLOCK_AIR;
					if (filly <= 32)
						b = BLOCK_STONE;

				if (b != BLOCK_AIR)
					sunlight = 0;
				blocks[idx0 + filly] = b | sunlight;
			}
		}
	}

	for (int i = 0; i < NUM_BLOCKTYPES; ++i) {
		int px = (i*2) % CHUNK_SIZE;
		int pz = ((i*2) / CHUNK_SIZE) * 2;
		blocks[blockIndex(blockx + px, 33, blockz + pz)] = i;
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

	uint64_t snowseed = game.map.seed ^ ((uint64_t)blockx << 32) ^ (uint64_t)blockz;

	uint32_t p, b;
	for (fillz = blockz; fillz < blockz + CHUNK_SIZE; ++fillz) {
		for (fillx = blockx; fillx < blockx + CHUNK_SIZE; ++fillx) {
			size_t idx0 = blockIndex(fillx, 0, fillz);
			if (idx0 >= MAP_BUFFER_SIZE) {
				printf("bad index: %d, %d, %d\n", fillx, 0, fillz);
				abort();
			}
			uint32_t sunlight = 0xf0000000;
			double height = fbm_simplex_2d(fillx, fillz, 0.5, NOISE_SCALE, 2.1117, 5);
			height = 32.0 + height * 24.0;
			b = BLOCK_AIR;
			p = BLOCK_AIR;
			for (filly = MAP_BLOCK_HEIGHT-1; filly >= 0; --filly) {
				if (filly > height && filly > OCEAN_LEVEL) {
					b = BLOCK_AIR;
				}
				else if (filly < 2) {
					b = BLOCK_BLACKROCK;
				} else if (filly < height) {
					if (p == BLOCK_AIR) {
						snowseed = rand64(snowseed);
						if (height > (140.0 - (snowseed % 10))) {
							b = BLOCK_SNOWY_GRASS_1;
						} else {
							b = BLOCK_WET_GRASS;
						}
					} else {
						b = BLOCK_WET_DIRT;
					}
				} else if (filly <= OCEAN_LEVEL) {
					b = BLOCK_OCEAN;
				} else {
					b = BLOCK_AIR;
				}
				if (b != BLOCK_AIR)
					sunlight = 0;
				blocks[idx0 + filly] = b | sunlight;
				p = b;
			}
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

void gen_loadchunk(game_chunk* chunk)
{
	gen_testmap(chunk);
}
