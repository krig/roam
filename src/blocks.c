#include "common.h"
#include "math3d.h"
#include "images.h"
#include "blocks.h"

struct blockinfo_t blockinfo[] = {
	{ .name = "air", .img = { 0 } },
	{ .name = "grass", .img = { IMG_GREEN_GRASS } },
	{ .name = "wet grass", .img = { IMG_WET_GRASS, IMG_WET_DIRT, IMG_WET_GRASS_SIDE } },
	{ .name = "dirt", .img = { IMG_DIRT } },
	{ .name = "wet dirt", .img = { IMG_WET_DIRT } },
	{ .name = "solid dirt", .img = { IMG_SOLID_DIRT } },
	{ .name = "stone", .img = { IMG_LIGHT_STONE } },
	{ .name = "clay", .img = { IMG_CLAY } },
	{ .name = "snow", .img = { IMG_SNOW } },
	{ .name = "mint", .img = { IMG_MINT } },
	{ .name = "red leaves", .img = { IMG_RED_LEAVES } },
	{ .name = "pig", .img = { IMG_PIGBACK, IMG_PIG_SKIN, IMG_PIGBACK, IMG_PIGBACK, IMG_PIG_HEAD, IMG_PIG_SKIN } },
	{ .name = "meat", .img = { IMG_MEAT_TOP, IMG_MEAT_TOP, IMG_MEAT_SIDE } },
	{ .name = "torch", .img = { IMG_TORCH_TOP, IMG_SWORD_HILT, IMG_TORCH_SIDE } },
	{ .name = "melon", .img = { IMG_MELON_CUT, IMG_MELON_SKIN, IMG_MELON_SIDE } },
	{ .name = "tree", .img = { IMG_TREE_CUT, IMG_TREE_CUT, IMG_TREE } },
	{ .name = "birch", .img = { IMG_BIRCH_CUT, IMG_BIRCH_CUT, IMG_BIRCH_TREE } },
	{ .name = "oak", .img = { IMG_OAK_CUT, IMG_OAK_CUT, IMG_OAK_TREE } },
	{ .name = "palm", .img = { IMG_PALM_CUT, IMG_PALM_CUT, IMG_PALM_TREE } },
	{ .name = "snow grass 1", .img = { IMG_SNOW, IMG_WET_GRASS, IMG_GRASS_SNOW_1 } },
	{ .name = "snow grass 2", .img = { IMG_SNOW, IMG_WET_GRASS, IMG_GRASS_SNOW_2 } },
	{ .name = "test cube", .img = { IMG_TEST_U, IMG_TEST_D, IMG_TEST_L, IMG_TEST_R, IMG_TEST_F, IMG_TEST_B } },
	{ .name = "black rock", .img = { IMG_BLACKROCK } },
	{ .name = "ocean", .img = { IMG_OCEAN1 } },
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
