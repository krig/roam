#pragma once

#define AIR_DENSITY 0
#define WATER_DENSITY 100
#define GLASS_DENSITY 500
#define SOLID_DENSITY 1000

enum BlockTex {
	BLOCK_TEX_TOP,
	BLOCK_TEX_BOTTOM,
	BLOCK_TEX_LEFT,
	BLOCK_TEX_RIGHT,
	BLOCK_TEX_FRONT,
	BLOCK_TEX_BACK
};

enum BlockFlags {
	BLOCK_COLLIDER = 1, // collide with player
	BLOCK_ALPHA = 2, // draw in alpha pass
	BLOCK_MESH = 4, // custom mesh shape
	BLOCK_SPRITE = 8, // render without backface culling
	BLOCK_SLOPE = 0x10, // can walk up/down this block
};

enum BlockTypes {
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
	BLOCK_OCEAN2,
	BLOCK_MUSHROOM,
	BLOCK_TEST_ALPHA,
	BLOCK_GOLD_SAND,
	NUM_BLOCKTYPES
};


struct blockinfo {
	const char* name; // short description
	// image atlas index
	// 0 = fill with previous
	int img[6];

	uint32_t flags;
	int density; // relative density is used when meshing
	int light; // light emitted by block (rgb)
	int attenuation; // light attenuation (used when lighting)

	// todo: custom shape?
	// maybe this has to be hardcoded in the tesselator for each blocktype
};

extern struct blockinfo blockinfo[];


void blocks_init(void);
