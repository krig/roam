#include "common.h"
#include "math3d.h"
#include "images.h"
#include "blocks.h"

struct blockinfo_t blockinfo[] = {
	{ .name = "air", .img = { 0 } },
	{ .name = "grass", .img = { IMG_GREEN_GRASS }, .density = SOLID_DENSITY },
	{ .name = "wet grass", .img = { IMG_WET_GRASS, IMG_WET_DIRT, IMG_WET_GRASS_SIDE }, .density = SOLID_DENSITY },
	{ .name = "dirt", .img = { IMG_DIRT }, .density = SOLID_DENSITY  },
	{ .name = "wet dirt", .img = { IMG_WET_DIRT }, .density = SOLID_DENSITY  },
	{ .name = "solid dirt", .img = { IMG_SOLID_DIRT }, .density = SOLID_DENSITY  },
	{ .name = "stone", .img = { IMG_LIGHT_STONE }, .density = SOLID_DENSITY  },
	{ .name = "clay", .img = { IMG_CLAY }, .density = SOLID_DENSITY  },
	{ .name = "snow", .img = { IMG_SNOW }, .density = SOLID_DENSITY  },
	{ .name = "mint", .img = { IMG_MINT }, .density = SOLID_DENSITY  },
	{ .name = "red leaves", .img = { IMG_RED_LEAVES }, .density = SOLID_DENSITY  },
	{ .name = "pig", .img = { IMG_PIGBACK, IMG_PIG_SKIN, IMG_PIGBACK, IMG_PIGBACK, IMG_PIG_HEAD, IMG_PIG_SKIN }, .density = SOLID_DENSITY },
	{ .name = "meat", .img = { IMG_MEAT_TOP, IMG_MEAT_TOP, IMG_MEAT_SIDE }, .density = SOLID_DENSITY },
	{ .name = "torch", .img = { IMG_TORCH_TOP, IMG_SWORD_HILT, IMG_TORCH_SIDE }, .density = SOLID_DENSITY },
	{ .name = "melon", .img = { IMG_MELON_CUT, IMG_MELON_SKIN, IMG_MELON_SIDE }, .density = SOLID_DENSITY },
	{ .name = "tree", .img = { IMG_TREE_CUT, IMG_TREE_CUT, IMG_TREE }, .density = SOLID_DENSITY },
	{ .name = "birch", .img = { IMG_BIRCH_CUT, IMG_BIRCH_CUT, IMG_BIRCH_TREE }, .density = SOLID_DENSITY },
	{ .name = "oak", .img = { IMG_OAK_CUT, IMG_OAK_CUT, IMG_OAK_TREE }, .density = SOLID_DENSITY },
	{ .name = "palm", .img = { IMG_PALM_CUT, IMG_PALM_CUT, IMG_PALM_TREE }, .density = SOLID_DENSITY },
	{ .name = "snow grass 1", .img = { IMG_SNOW, IMG_WET_GRASS, IMG_GRASS_SNOW_1 }, .density = SOLID_DENSITY },
	{ .name = "snow grass 2", .img = { IMG_SNOW, IMG_WET_GRASS, IMG_GRASS_SNOW_2 }, .density = SOLID_DENSITY },
	{ .name = "test cube", .img = { IMG_TEST_U, IMG_TEST_D, IMG_TEST_L, IMG_TEST_R, IMG_TEST_F, IMG_TEST_B }, .density = SOLID_DENSITY },
	{ .name = "black rock", .img = { IMG_BLACKROCK }, .density = SOLID_DENSITY },
	{ .name = "ocean", .img = { IMG_OCEAN1 }, .density = WATER_DENSITY, .alpha = true },
	{ .name = "mushroom", .img = { IMG_BB_SHROOM1 }, .backfaces = true },
	{ .name = "test alpha", .img = { IMG_TEST_ALPHA }, .density = WATER_DENSITY, .alpha = true },
};

void blocks_init()
{
	for (int i = 0; i < sizeof(blockinfo)/sizeof(struct blockinfo_t); ++i) {
		int prev = IMG_BLACKROCK;
		for (int m = 0; m < 6; ++m) {
			if (blockinfo[i].img[m] == IMG_INVALID)
				blockinfo[i].img[m] = prev;
			else
				prev = blockinfo[i].img[m];
		}
	}
}
