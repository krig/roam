#pragma once

/*

 * voxel map = regions + chunks + ocean + trees + caves + mountains

 */

#define CHUNK_SIZE 16
#define VIEW_DISTANCE 16
#define GROUND_LEVEL 80
#define MAP_CHUNK_WIDTH (VIEW_DISTANCE*2)
#define MAP_CHUNK_HEIGHT 8
#define MAP_BLOCK_WIDTH (MAP_CHUNK_WIDTH*CHUNK_SIZE)
#define MAP_BLOCK_HEIGHT (MAP_CHUNK_HEIGHT*CHUNK_SIZE)
#define MAP_BUFFER_SIZE (MAP_BLOCK_WIDTH*MAP_BLOCK_WIDTH*MAP_BLOCK_HEIGHT)

enum game_blocktype {
	BLOCK_AIR = 0,
	BLOCK_GRASS,
	BLOCK_DIRT,
	BLOCK_SAND,
	BLOCK_STONE,
	BLOCK_DARKSTONE,
	BLOCK_BONE,
	BLOCK_SKIN,
	BLOCK_DIAMOND_ORE,
	BLOCK_CLAY,
	BLOCK_COBBLESTONE,
	BLOCK_LAVA,
	BLOCK_WATER,
	BLOCK_TREETRUNK,
	BLOCK_DEADSKIN,
	BLOCK_SHROOM,
	BLOCK_GRASS_BILLBOARD,
	BLOCK_LEAVES,
	BLOCK_MOSS,
	BLOCK_SNOWY_SPRUCE,
	NUM_BLOCKTYPES
};

#define BLOCK_TC_W (16.0 / 128.0)

static inline ml_vec2 idx2tc(int idx) {
	ml_vec2 tc;
	tc.x = (float)((idx - 1) % 8) * BLOCK_TC_W;
	tc.y = (float)((idx - 1) / 8) * BLOCK_TC_W;
	return tc;
}

#pragma pack(push, 1)

typedef struct tc2us_t {
	uint16_t u;
	uint16_t v;
} tc2us_t;

typedef struct game_block_vtx {
	uint32_t pos; // 10_10_10_2
	int8_t n[4];
	tc2us_t tc;
	uint32_t clr;
} game_block_vtx;

typedef struct game_block {
	uint8_t type;
	uint8_t meta;
} game_block;

#pragma pack(pop)

typedef struct game_chunk {
	int x; // actual coordinates of chunk
	int z;
	ml_mesh data[MAP_CHUNK_HEIGHT];
	// add per-chunk state information here (things like command blocks...)
} game_chunk;

typedef struct game_map {
	// caches the block data for the area around the player
	// blocks = block id for each block
	// meta = light levels + flags?
	// as the player moves, blocks should be swapped in/out
	// (load or generate asynchronously, then copy over in update?)
	game_block blocks[MAP_BUFFER_SIZE]; // 128MB
	uint16_t light[MAP_BUFFER_SIZE]; // 128MB
	game_chunk chunks[MAP_CHUNK_WIDTH*MAP_CHUNK_WIDTH];
	unsigned long seed;
} game_map;

void gameFreeMap(void);
void gameInitMap(void);
void gameUpdateMap(void);
void gameDrawMap(void);

// mod which handles negative numbers
static inline int mod(int a, int b) {
   if(b < 0) //you can check for b == 0 separately and do what you want
     return mod(-a, -b);
   int ret = a % b;
   if(ret < 0)
     ret+=b;
   return ret;
}

// elements xyz are 0-32
// w should be 0-3
static inline uint32_t
mapPackVectorChunkCoord(unsigned int x, unsigned int y, unsigned int z, unsigned int w) {
	return ((x)*(unsigned int)(1023/CHUNK_SIZE)) + (((y)*(unsigned int)(1023/CHUNK_SIZE))<<10) + (((z)*(unsigned int)(1023/CHUNK_SIZE))<<20) + ((w)<<30);
}
