#include "common.h"
#include "math3d.h"
#include "images.h"
#include "blocks.h"

struct blockinfo_t blockinfo[] = {
	{ .name = "air", .img = { 0 } },
	{ .name = "grass", .img = { IMG_GREEN_GRASS }, .density = 100 },
	{ .name = "wet grass", .img = { IMG_WET_GRASS, IMG_WET_DIRT, IMG_WET_GRASS_SIDE }, .density = 100 },
	{ .name = "dirt", .img = { IMG_DIRT }, .density = 100  },
	{ .name = "wet dirt", .img = { IMG_WET_DIRT }, .density = 100  },
	{ .name = "solid dirt", .img = { IMG_SOLID_DIRT }, .density = 100  },
	{ .name = "stone", .img = { IMG_LIGHT_STONE }, .density = 100  },
	{ .name = "clay", .img = { IMG_CLAY }, .density = 100  },
	{ .name = "snow", .img = { IMG_SNOW }, .density = 100  },
	{ .name = "mint", .img = { IMG_MINT }, .density = 100  },
	{ .name = "red leaves", .img = { IMG_RED_LEAVES }, .density = 100  },
	{ .name = "pig", .img = { IMG_PIGBACK, IMG_PIG_SKIN, IMG_PIGBACK, IMG_PIGBACK, IMG_PIG_HEAD, IMG_PIG_SKIN }, .density = 100 },
	{ .name = "meat", .img = { IMG_MEAT_TOP, IMG_MEAT_TOP, IMG_MEAT_SIDE }, .density = 100 },
	{ .name = "torch", .img = { IMG_TORCH_TOP, IMG_SWORD_HILT, IMG_TORCH_SIDE }, .density = 100 },
	{ .name = "melon", .img = { IMG_MELON_CUT, IMG_MELON_SKIN, IMG_MELON_SIDE }, .density = 100 },
	{ .name = "tree", .img = { IMG_TREE_CUT, IMG_TREE_CUT, IMG_TREE }, .density = 100 },
	{ .name = "birch", .img = { IMG_BIRCH_CUT, IMG_BIRCH_CUT, IMG_BIRCH_TREE }, .density = 100 },
	{ .name = "oak", .img = { IMG_OAK_CUT, IMG_OAK_CUT, IMG_OAK_TREE }, .density = 100 },
	{ .name = "palm", .img = { IMG_PALM_CUT, IMG_PALM_CUT, IMG_PALM_TREE }, .density = 100 },
	{ .name = "snow grass 1", .img = { IMG_SNOW, IMG_WET_GRASS, IMG_GRASS_SNOW_1 }, .density = 100 },
	{ .name = "snow grass 2", .img = { IMG_SNOW, IMG_WET_GRASS, IMG_GRASS_SNOW_2 }, .density = 100 },
	{ .name = "test cube", .img = { IMG_TEST_U, IMG_TEST_D, IMG_TEST_L, IMG_TEST_R, IMG_TEST_F, IMG_TEST_B }, .density = 100 },
	{ .name = "black rock", .img = { IMG_BLACKROCK }, .density = 100 },
	{ .name = "ocean", .img = { IMG_OCEAN1 }, .density = 50, .alpha = true },
	{ .name = "mushroom", .img = { IMG_BB_SHROOM1 }, .backfaces = true },
};

void initBlockInfo() {
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
