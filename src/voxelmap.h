#pragma once

/*

 * voxel map = regions + chunks + ocean + trees + caves + mountains

 */

enum game_blocktype {
	BLOCK_AIR = 0,
	BLOCK_STONE
};

typedef struct game_chunk {
	int x;
	int y;
	int z;
	uint8_t data[16 * 16 * 16];
} game_chunk;

#pragma pack(push, 4)
typedef struct game_block_vtx {
	uint8_t pos[4]; // x, y, z, blocktype
	uint8_t n[4]; // nx, ny, nz, ao/lightlevel?
} game_block_vtx;
#pragma pack(pop)

void gameGenerateChunk(game_chunk* chunk, int x, int y, int z);
size_t gameTesselateChunk(ml_mesh* mesh, game_chunk* chunk);


void gameInitMap();
void gameUpdateMap();
void gameDrawMap();
