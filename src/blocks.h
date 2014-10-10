#pragma once

enum blocktexidx_t {
	BLOCK_TEX_TOP,
	BLOCK_TEX_BOTTOM,
	BLOCK_TEX_LEFT,
	BLOCK_TEX_RIGHT,
	BLOCK_TEX_FRONT,
	BLOCK_TEX_BACK
};

struct blockinfo_t {
	const char* name; // short description
	// image atlas index
	// 0 = fill with previous
	int img[6];
};

extern struct blockinfo_t blockinfo[];

enum block_types {
	BLOCK_AIR,
	BLOCK_GRASS,
	BLOCK_WETGRASS,
	BLOCK_DIRT,
	BLOCK_SOLID_DIRT,
	BLOCK_STONE,
	BLOCK_CLAY,
	BLOCK_SNOW,
	BLOCK_RED_LEAVES,
	BLOCK_PIG,
	BLOCK_MEAT,
	BLOCK_TORCH,
	NUM_BLOCKTYPES
};

void initBlockInfo(void);
