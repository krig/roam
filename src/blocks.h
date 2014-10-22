#pragma once

enum blocktexidx_t {
	BLOCK_TEX_TOP,
	BLOCK_TEX_BOTTOM,
	BLOCK_TEX_LEFT,
	BLOCK_TEX_RIGHT,
	BLOCK_TEX_FRONT,
	BLOCK_TEX_BACK
};

enum blockflags_t {
	BLOCK_SOLID = 1,
	BLOCK_ALPHA = 2, // draw in alpha pass
	BLOCK_MESH = 4, // custom mesh shape
	BLOCK_TWOSIDED = 4 // render without backface culling
};

#define AIR_DENSITY 0
#define WATER_DENSITY 1
#define SOLID_DENSITY 2

struct blockinfo_t {
	const char* name; // short description
	// image atlas index
	// 0 = fill with previous
	int img[6];

	int density;

	bool alpha; // render in alpha pass
	bool backfaces; // render in backface pass
	bool lightsource; // emits light?

	// todo: custom shape?
	// maybe this has to be hardcoded in the tesselator for each blocktype
};

extern struct blockinfo_t blockinfo[];

enum block_types {
	BLOCK_AIR,
	BLOCK_GRASS,
	BLOCK_WET_GRASS,
	BLOCK_DIRT,
	BLOCK_WET_DIRT,
	BLOCK_SOLID_DIRT,
	BLOCK_STONE,
	BLOCK_CLAY,
	BLOCK_SNOW,
	BLOCK_MINT,
	BLOCK_RED_LEAVES,
	BLOCK_PIG,
	BLOCK_MEAT,
	BLOCK_TORCH,
	BLOCK_MELON,
	BLOCK_TREE,
	BLOCK_BIRCH,
	BLOCK_OAK,
	BLOCK_PALM,
	BLOCK_SNOWY_GRASS_1,
	BLOCK_SNOWY_GRASS_2,
	BLOCK_TESTCUBE,
	BLOCK_BLACKROCK,
	BLOCK_OCEAN,
	BLOCK_MUSHROOM,
	NUM_BLOCKTYPES
};

void blocks_init(void);
