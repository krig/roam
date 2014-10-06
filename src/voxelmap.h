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
	BLOCK_STONE,
//	BLOCK_DIRT,
//	BLOCK_GRASS,
//	BLOCK_SAND,
//	BLOCK_WOOD,
//	BLOCK_LEAF,
//	BLOCK_WATER,
//	BLOCK_LAVA
};

#pragma pack(push, 4)
typedef struct game_block_vtx {
	uint32_t pos; // 10_10_10_2
	uint32_t n; // 4_4_4_4
	uint32_t clr; // 4_4_4_4
} game_block_vtx;
#pragma pack(pop)

typedef struct game_chunk {
	int x; // actual coordinates of chunk
	int z;
	ml_mesh* data[MAP_CHUNK_HEIGHT];
} game_chunk;

typedef struct game_map {
	// caches the block data for the area around the player
	// blocks = block id for each block
	// meta = light levels + flags?
	// as the player moves, blocks should be swapped in/out
	// (load or generate asynchronously, then copy over in update?)
	uint8_t blocks[MAP_BUFFER_SIZE]; // 64MB
	uint8_t meta[MAP_BUFFER_SIZE]; // 64MB
	uint8_t light[MAP_BUFFER_SIZE]; // 64MB
	game_chunk chunks[MAP_CHUNK_WIDTH*MAP_CHUNK_WIDTH];
	unsigned long seed;
} game_map;

void gameFreeMap();
void gameInitMap();
void gameUpdateMap();
void gameDrawMap();
