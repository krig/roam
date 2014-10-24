#include "common.h"
#include "math3d.h"
#include "images.h"
#include "blocks.h"

struct blockinfo blockinfo[] = {
	{ .name = "air",
	  .img = { 0 }
	},
	{ .name = "grass",
	  .img = { IMG_GREEN_GRASS },
	  .density = SOLID_DENSITY,
	  .flags = BLOCK_COLLIDER
	},
	{ .name = "wet grass",
	  .img = { IMG_WET_GRASS, IMG_WET_DIRT, IMG_WET_GRASS_SIDE },
	  .density = SOLID_DENSITY,
	  .flags = BLOCK_COLLIDER
	},
	{ .name = "dirt",
	  .img = { IMG_DIRT },
	  .density = SOLID_DENSITY,
	  .flags = BLOCK_COLLIDER
	},
	{ .name = "wet dirt",
	  .img = { IMG_WET_DIRT },
	  .density = SOLID_DENSITY,
	  .flags = BLOCK_COLLIDER
	},
	{ .name = "solid dirt",
	  .img = { IMG_SOLID_DIRT },
	  .density = SOLID_DENSITY,
	  .flags = BLOCK_COLLIDER
	},
	{ .name = "stone",
	  .img = { IMG_LIGHT_STONE },
	  .density = SOLID_DENSITY,
	  .flags = BLOCK_COLLIDER
	},
	{ .name = "clay",
	  .img = { IMG_CLAY },
	  .density = SOLID_DENSITY,
	  .flags = BLOCK_COLLIDER
	},
	{ .name = "snow",
	  .img = { IMG_SNOW },
	  .density = SOLID_DENSITY,
	  .flags = BLOCK_COLLIDER
	},
	{ .name = "mint",
	  .img = { IMG_MINT },
	  .density = SOLID_DENSITY,
	  .flags = BLOCK_COLLIDER
	},
	{ .name = "red leaves",
	  .img = { IMG_RED_LEAVES },
	  .density = SOLID_DENSITY,
	  .flags = BLOCK_COLLIDER
	},
	{ .name = "pig",
	  .img = { IMG_PIGBACK, IMG_PIG_SKIN, IMG_PIGBACK, IMG_PIGBACK, IMG_PIG_HEAD, IMG_PIG_SKIN },
	  .density = SOLID_DENSITY,
	  .flags = BLOCK_COLLIDER
	},
	{ .name = "meat",
	  .img = { IMG_MEAT_TOP, IMG_MEAT_TOP, IMG_MEAT_SIDE },
	  .density = SOLID_DENSITY,
	  .flags = BLOCK_COLLIDER
	},
	{ .name = "torch",
	  .img = { IMG_TORCH_TOP, IMG_SWORD_HILT, IMG_TORCH_SIDE },
	  .density = SOLID_DENSITY,
	  .flags = BLOCK_COLLIDER
	},
	{ .name = "melon",
	  .img = { IMG_MELON_CUT, IMG_MELON_SKIN, IMG_MELON_SIDE },
	  .density = SOLID_DENSITY,
	  .flags = BLOCK_COLLIDER
	},
	{ .name = "tree",
	  .img = { IMG_TREE_CUT, IMG_TREE_CUT, IMG_TREE },
	  .density = SOLID_DENSITY,
	  .flags = BLOCK_COLLIDER
	},
	{ .name = "birch",
	  .img = { IMG_BIRCH_CUT, IMG_BIRCH_CUT, IMG_BIRCH_TREE },
	  .density = SOLID_DENSITY,
	  .flags = BLOCK_COLLIDER
	},
	{ .name = "oak",
	  .img = { IMG_OAK_CUT, IMG_OAK_CUT, IMG_OAK_TREE },
	  .density = SOLID_DENSITY,
	  .flags = BLOCK_COLLIDER
	},
	{ .name = "palm",
	  .img = { IMG_PALM_CUT, IMG_PALM_CUT, IMG_PALM_TREE },
	  .density = SOLID_DENSITY,
	  .flags = BLOCK_COLLIDER
	},
	{ .name = "snow grass 1",
	  .img = { IMG_SNOW, IMG_WET_GRASS, IMG_GRASS_SNOW_1 },
	  .density = SOLID_DENSITY,
	  .flags = BLOCK_COLLIDER
	},
	{ .name = "snow grass 2",
	  .img = { IMG_SNOW, IMG_WET_GRASS, IMG_GRASS_SNOW_2 },
	  .density = SOLID_DENSITY,
	  .flags = BLOCK_COLLIDER
	},
	{ .name = "test cube",
	  .img = { IMG_TEST_U, IMG_TEST_D, IMG_TEST_L, IMG_TEST_R, IMG_TEST_F, IMG_TEST_B },
	  .density = SOLID_DENSITY,
	  .flags = BLOCK_COLLIDER
	},
	{ .name = "black rock",
	  .img = { IMG_BLACKROCK },
	  .density = SOLID_DENSITY,
	  .flags = BLOCK_COLLIDER

	},
	{ .name = "ocean",
	  .img = { IMG_OCEAN1 },
	  .density = WATER_DENSITY,
	  .flags = BLOCK_ALPHA
	},
	{ .name = "mushroom",
	  .img = { IMG_BB_SHROOM1 },
	  .flags = BLOCK_SPRITE
	},
	{ .name = "test alpha",
	  .img = { IMG_TEST_ALPHA },
	  .density = SOLID_DENSITY,
	  .flags = BLOCK_COLLIDER | BLOCK_ALPHA
	},
};

void blocks_init()
{
	for (int i = 0; i < sizeof(blockinfo)/sizeof(struct blockinfo); ++i) {
		int prev = IMG_BLACKROCK;
		for (int m = 0; m < 6; ++m) {
			if (blockinfo[i].img[m] == IMG_INVALID)
				blockinfo[i].img[m] = prev;
			else
				prev = blockinfo[i].img[m];
		}
	}
}
