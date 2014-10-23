#pragma once

/*

 * voxel map = regions + chunks + ocean + trees + caves + mountains

 */

#include "blocks.h"

#define CHUNK_SIZE 32
#define CHUNK_SIZE_STR(s) CHUNK_SIZE_STR_2(s)
#define CHUNK_SIZE_STR_2(s) #s
#define VIEW_DISTANCE 8
#define OCEAN_LEVEL 32
#define MAP_CHUNK_WIDTH (VIEW_DISTANCE*2)
#define MAP_CHUNK_HEIGHT 8
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

enum CHUNK_GEN_STATE {
	CHUNK_GEN_S0, /* no blocks generated for this chunk yet */
	CHUNK_GEN_S1, /* base terrain blocks generated */
	CHUNK_GEN_S2, /* structures / plants generated */
	CHUNK_GEN_S3, /* light fully propagated */
};

enum CHUNK_TESS_STATE {
	CHUNK_TESS_S0, /* no mesh generated */
	CHUNK_TESS_S1, /* gen-state 1 mesh generated */
	CHUNK_TESS_S2, /* gen-state 2 mesh generated */
	CHUNK_TESS_S3
};

typedef struct game_chunk {
	int x; // actual coordinates of chunk
	int z;
	bool dirty;
	uint32_t sgen;
	uint32_t stess;
	ml_mesh solid[MAP_CHUNK_HEIGHT];
	ml_mesh alpha; // sort before rendering - should not be a mesh? no backface culling?
	ml_mesh twosided; // render twosided (same shader as solid meshes but different render state)
	// add per-chunk state information here (things like command blocks..., entities)
} game_chunk;

// block:
// SSSSRRRRGGGGBBBBMMMMMMMMTTTTTTTT
// S: sunlight value 0-15
// R: red lamplight value 0-15
// G: green lamplight value 0-15
// B: blue lamplight value 0-15
// M: metadata 0-255
// T: blocktype 0-255

struct game_map_t {
	game_chunk chunks[MAP_CHUNK_WIDTH*MAP_CHUNK_WIDTH];
	// caches the block data for the area around the player
	// blocks = [blockid | meta | sunlightlevel | torchlightlevel ]
	unsigned long seed;
};

extern uint32_t* map_blocks;

void gameFreeMap(void);
void gameInitMap(void);
void gameUpdateMap(void);
void gameDrawMap(ml_frustum* frustum);
void gameDrawAlphaPass(void);
void gameLoadChunk(int x, int z);
void gameUnloadChunk(int x, int z);
void gameTesselateChunk(int x, int z);
void gameUpdateBlock(ml_ivec3 block, uint32_t value);
bool gameRayTest(ml_dvec3 origin, ml_vec3 dir, int len, ml_ivec3* hit, ml_ivec3* prehit);
uint32_t blockData(int x, int y, int z);

// mod which handles negative numbers
static inline
int mod(int a, int b)
{
	if (b < 0) { a = -a; b = -b; }
	int ret = a % b;
	return (ret < 0) ? ret + b : ret;
}

// elements xyz are 0-32
// w should be 0-3
static inline
uint32_t mapPackVectorChunkCoord(unsigned int x, unsigned int y, unsigned int z, unsigned int w)
{
	assert(x <= CHUNK_SIZE && y <= CHUNK_SIZE && z <= CHUNK_SIZE && w < 4);
	return ((x)*(unsigned int)(1023/CHUNK_SIZE)) + (((y)*(unsigned int)(1023/CHUNK_SIZE))<<10) + (((z)*(unsigned int)(1023/CHUNK_SIZE))<<20) + ((w)<<30);
}


static inline
size_t blockIndex(int x, int y, int z)
{
	return mod(z, MAP_BLOCK_WIDTH) * (MAP_BLOCK_WIDTH * MAP_BLOCK_HEIGHT) +
		mod(x, MAP_BLOCK_WIDTH) * MAP_BLOCK_HEIGHT +
		y;
}

static inline
uint32_t* blockColumn(int x, int z)
{
	return map_blocks +
		mod(z, MAP_BLOCK_WIDTH) * (MAP_BLOCK_WIDTH * MAP_BLOCK_HEIGHT) +
		mod(x, MAP_BLOCK_WIDTH) * MAP_BLOCK_HEIGHT;
}

static inline
uint32_t blockType(int x, int y, int z)
{
	return blockData(x, y, z) & 0xff;
}

static inline
size_t blockByCoord(ml_ivec3 xyz)
{
	return blockIndex(xyz.x, xyz.y, xyz.z);
}

static inline
uint32_t blockTypeByCoord(ml_ivec3 xyz)
{
	return blockType(xyz.x, xyz.y, xyz.z);
}

static inline
ml_chunk blockToChunk(ml_ivec3 block)
{
	ml_chunk c = { floor(round(block.x) / CHUNK_SIZE),
	               floor(round(block.z) / CHUNK_SIZE) };
	return c;
}
