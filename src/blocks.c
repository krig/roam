#include "common.h"
#include "math3d.h"
#include "images.h"
#include "blocks.h"

struct blockinfo_t blockinfo[] = {
	{ .name = "air", .img = { 0 } },
	{ .name = "grass", .img = { IMG_GREENGRASS } },
	{ .name = "wetgrass", .img = { IMG_WETGRASS } },
	{ .name = "dirt", .img = { IMG_DIRT } },
	{ .name = "solid dirt", .img = { IMG_SOLID_DIRT } },
	{ .name = "stone", .img = { IMG_LIGHT_STONE } },
	{ .name = "clay", .img = { IMG_CLAY } },
	{ .name = "snow", .img = { IMG_SNOW } },
	{ .name = "red leaves", .img = { IMG_RED_LEAVES } },
	{ .name = "pig", .img = { IMG_PIGBACK, IMG_PIG_SKIN, IMG_PIGBACK, IMG_PIGBACK, IMG_PIG_HEAD, IMG_PIG_SKIN } },
	{ .name = "meat", .img = { IMG_MEAT_TOP, IMG_MEAT_TOP, IMG_MEAT_SIDE } },
	{ .name = "torch", .img = { IMG_TORCH_TOP, IMG_SWORD_HILT, IMG_TORCH_SIDE } },
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
