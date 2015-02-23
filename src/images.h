#pragma once

#define IMG_TEXW 8
#define IMG_ATLAS_TEXW 128
#define IMG_ATLAS_ROW (IMG_ATLAS_TEXW / IMG_TEXW)
#define IMG_TCW ((double)IMG_TEXW / (double)IMG_ATLAS_TEXW)
#define IMG_TC_BIAS (1.0/4000.0)
#define IMG_AT(c, r) (IMG_ATLAS_ROW*(r) + (c + 1))

enum ImageTypes {
	IMG_INVALID,
	IMG_GREEN_GRASS = IMG_AT(0, 0),
	IMG_WET_GRASS = IMG_AT(1, 0),
	IMG_DIRT = IMG_AT(2, 0),
	IMG_WET_DIRT = IMG_AT(3, 0),
	IMG_SOLID_DIRT,
	IMG_BLACKROCK,
	IMG_LIGHT_STONE,
	IMG_SNOW,
	IMG_BB_GRASS,
	IMG_BB_REDFLOWER,
	IMG_BB_FRUIT,
	IMG_BB_BERRIES,
	IMG_BB_SHROOM1,
	IMG_BB_SHROOM2,
	IMG_BB_SHROOM3,
	IMG_BB_SHROOM4,
	IMG_GREEN_LEAVES = IMG_AT(0, 1),
	IMG_YELLOW_LEAVES,
	IMG_ORANGE_LEAVES,
	IMG_DARKORANGE_LEAVES,
	IMG_RED_LEAVES,
	IMG_SNOW2,
	IMG_CLAY,
	IMG_BLACKROCK2,
	IMG_BB_GRASS2,
	IMG_BB_YELLOWFLOWER,
	IMG_BB_STRAW,
	IMG_MINT = IMG_AT(0, 2),
	IMG_OCEAN1,
	IMG_OCEAN2,
	IMG_OCEAN3,
	IMG_MOSS,
	IMG_DARKMOSS,
	IMG_BLACKROCK3,
	IMG_FALLENLEAVES,
	IMG_ICON_FRAME,
	IMG_TREE,
	IMG_BIRCH_TREE,
	IMG_PALM_TREE,
	IMG_OAK_TREE,
	IMG_OAK_CUT,
	IMG_BIRCH_CUT,
	IMG_SICK_GRASS = IMG_AT(0, 3),
	IMG_POISON_GRASS,
	IMG_PIG_SKIN = IMG_AT(2, 3),
	IMG_WHITE_SAND,
	IMG_BLOOD_ROCK,
	IMG_PINK_BLOCK,
	IMG_STRONG_MINT,
	IMG_BEIGE_ROCK,
	IMG_BRICK = IMG_AT(10, 3),
	IMG_MELON_SIDE = IMG_AT(11, 3),
	IMG_MELON_CUT = IMG_AT(12, 3),
	IMG_PALM_CUT = IMG_AT(13, 3),
	IMG_TREE_CUT = IMG_AT(14, 3),
	IMG_GOLD_SAND = IMG_AT(0, 4),
	IMG_PALE_CLAY,
	IMG_SKIN,
	IMG_RICH_PURPLE,
	IMG_DEEP_BLACK,
	IMG_PALE_PINK,
	IMG_LAVA = IMG_AT(6, 4),
	IMG_PALE_MUD,
	IMG_GRASS_SNOW_1 = IMG_AT(8, 4),
	IMG_GRASS_SNOW_2 = IMG_AT(9, 4),
	IMG_GOLD_ORE = IMG_AT(10, 4),
	IMG_MELON_SKIN = IMG_AT(11, 4),
	IMG_MEAT_SIDE = IMG_AT(0, 5),
	IMG_PIG_HEAD,
	IMG_TORCH_SIDE = IMG_AT(5, 5),
	IMG_TORCH_TOP,
	IMG_SWORD_BLADE,
	IMG_MEAT_TOP = IMG_AT(0, 6),
	IMG_PIGBACK,
	IMG_SWORD_HILT = IMG_AT(6, 6),
	IMG_TEST_L = IMG_AT(0, 7),
	IMG_TEST_R,
	IMG_TEST_F,
	IMG_TEST_B,
	IMG_TEST_U,
	IMG_TEST_D,
	IMG_WET_GRASS_SIDE,
	IMG_TEST_ALPHA = IMG_AT(0, 8),
	NUM_IMG_TYPES
};

static inline vec2_t imgTC(int idx) {
	vec2_t tc;
	tc.x = (float)((idx-1) % IMG_ATLAS_ROW) * IMG_TCW;
	tc.y = (float)((idx-1) / IMG_ATLAS_ROW) * IMG_TCW;
	return tc;
}
