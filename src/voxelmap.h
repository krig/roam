#pragma once

/*

 * voxel map = regions + chunks + ocean + trees + caves + mountains

 */

#include "blocks.h"

#define CHUNK_SIZE 32
#define CHUNK_SIZE_STR(s) CHUNK_SIZE_STR_2(s)
#define CHUNK_SIZE_STR_2(s) #s
#define VIEW_DISTANCE 16
#define OCEAN_LEVEL 32
#define MAP_CHUNK_WIDTH (VIEW_DISTANCE*2)
#define MAP_CHUNK_HEIGHT 16
#define MAP_BLOCK_WIDTH (MAP_CHUNK_WIDTH*CHUNK_SIZE)
#define MAP_BLOCK_HEIGHT (MAP_CHUNK_HEIGHT*CHUNK_SIZE)
#define MAP_BUFFER_SIZE (MAP_BLOCK_WIDTH*MAP_BLOCK_WIDTH*MAP_BLOCK_HEIGHT)

#pragma pack(push, 1)

typedef struct tc2us_t {
	uint16_t u;
	uint16_t v;
} tc2us_t;

typedef struct game_block_vtx {
	ml_vec3 pos;
	tc2us_t tc;
	uint32_t clr;
} game_block_vtx;

#pragma pack(pop)

enum CHUNK_STATE {
	CHUNK_UNLOADED,
	CHUNK_ECOLOGY, /* ecology has been generated */
	CHUNK_TERRAIN, /* terrain has been generated */
	CHUNK_PLANTS, /* plants have been generated */
 	CHUNK_LIT /* chunk has been lit */
};

typedef struct game_chunk {
	int x; // actual coordinates of chunk
	int z;
	bool dirty;
	uint32_t state;
	ml_mesh solid[MAP_CHUNK_HEIGHT];
	ml_mesh alpha; // sort before rendering - should not be a mesh?
	ml_mesh twosided; // render twosided
	// add per-chunk state information here (things like command blocks..., entities)
} game_chunk;

typedef struct game_map {
	// caches the block data for the area around the player
	// blocks = block id for each block
	// meta = light levels + flags?
	// as the player moves, blocks should be swapped in/out
	// (load or generate asynchronously, then copy over in update?)
	uint8_t blocks[MAP_BUFFER_SIZE]; // 64MB
	uint16_t light[MAP_BUFFER_SIZE]; // 128MB
	game_chunk chunks[MAP_CHUNK_WIDTH*MAP_CHUNK_WIDTH];
	unsigned long seed;
} game_map;

void gameFreeMap(void);
void gameInitMap(void);
void gameUpdateMap(void);
void gameDrawMap(ml_frustum* frustum);

// mod which handles negative numbers
static inline int mod(int a, int b) {
   if(b < 0) //you can check for b == 0 separately and do what you want
     return mod(-a, -b);
   int ret = a % b;
   if(ret < 0)
     ret+=b;
   return ret;
}

#include <assert.h>

// elements xyz are 0-32
// w should be 0-3
static inline uint32_t
mapPackVectorChunkCoord(unsigned int x, unsigned int y, unsigned int z, unsigned int w) {
	assert(x <= CHUNK_SIZE && y <= CHUNK_SIZE && z <= CHUNK_SIZE && w < 4);
	return ((x)*(unsigned int)(1023/CHUNK_SIZE)) + (((y)*(unsigned int)(1023/CHUNK_SIZE))<<10) + (((z)*(unsigned int)(1023/CHUNK_SIZE))<<20) + ((w)<<30);
}

// origin block
// direction vector
// length in blocks
bool gameRayTest(ml_dvec3 origin, ml_vec3 dir, int len, ml_ivec3* hit, ml_ivec3* prehit);

static inline size_t blockIndex(int x, int y, int z) {
	return mod(z, MAP_BLOCK_WIDTH) * (MAP_BLOCK_WIDTH * MAP_BLOCK_HEIGHT) +
		mod(x, MAP_BLOCK_WIDTH) * MAP_BLOCK_HEIGHT +
		y;
}

static inline size_t blockByCoord(ml_ivec3 xyz) {
	return blockIndex(xyz.x, xyz.y, xyz.z);
}

uint8_t blockType(int x, int y, int z);

static inline uint8_t blockTypeByCoord(ml_ivec3 xyz) {
	return blockType(xyz.x, xyz.y, xyz.z);
}

static inline ml_chunk blockToChunk(ml_ivec3 block) {
	ml_chunk c = { floor(round(block.x) / CHUNK_SIZE),
	               floor(round(block.z) / CHUNK_SIZE) };
	return c;
}

void gameLoadChunk(int x, int z);
void gameUnloadChunk(int x, int z);
void gameTesselateChunk(int x, int z);
void gameUpdateBlock(ml_ivec3 block);
