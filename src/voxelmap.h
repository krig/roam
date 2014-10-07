#pragma once

/*

 * voxel map = regions + chunks + ocean + trees + caves + mountains

 */

#define CHUNK_SIZE 32
#define VIEW_DISTANCE 8
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
};

#define BLOCK_TC_W (16.0 / 128.0)

static inline ml_vec2 idx2tc(int idx) {
	ml_vec2 tc;
	tc.x = (float)((idx - 1) % 8) * BLOCK_TC_W;
	tc.y = (float)((idx - 1) / 8) * BLOCK_TC_W;
	return tc;
}

#pragma pack(push, 1)
typedef struct game_block_vtx {
	uint32_t pos; // 10_10_10_2
	int8_t n[4];
	uint16_t tc[2];
	uint8_t clr[4];
} game_block_vtx;
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
	uint8_t blocks[MAP_BUFFER_SIZE]; // 64MB
	uint8_t meta[MAP_BUFFER_SIZE]; // 64MB
	uint16_t light[MAP_BUFFER_SIZE]; // 128MB 4 bits/channel, [r, g, b, a]
	game_chunk chunks[MAP_CHUNK_WIDTH*MAP_CHUNK_WIDTH];
	unsigned long seed;
} game_map;

void gameFreeMap();
void gameInitMap();
void gameUpdateMap();
void gameDrawMap();
