#pragma once

/*

 * voxel map = regions + chunks + ocean + trees + caves + mountains

 */

#include "blocks.h"

#define CHUNK_SIZE 16
#define MAX_SUBCHUNKS 64 // allow chunks populated across 1km (!)
#define CHUNK_SIZE_STR(s) CHUNK_SIZE_STR_2(s)
#define CHUNK_SIZE_STR_2(s) #s
#define VIEW_DISTANCE 20
#define OCEAN_LEVEL 32
#define MAP_CHUNK_WIDTH (VIEW_DISTANCE*2)
#define MAP_CHUNK_HEIGHT 8
#define MAP_BLOCK_WIDTH (MAP_CHUNK_WIDTH*CHUNK_SIZE)
#define MAP_BLOCK_HEIGHT (MAP_CHUNK_HEIGHT*CHUNK_SIZE)
#define MAP_BUFFER_SIZE (MAP_BLOCK_WIDTH*MAP_BLOCK_WIDTH*MAP_BLOCK_HEIGHT)

#pragma pack(push, 1)

typedef struct tc2us_t {
	uint16_t u;
	uint16_t v;
} tc2us_t;

typedef struct block_vtx_t {
	vec3_t pos;
//	uint32_t n; // normal + extra uint8 value (water depth?)
	tc2us_t tc;
	uint32_t clr;
} block_vtx_t;

typedef struct block_face_t {
	block_vtx_t vtx[3];
} block_face_t;

#pragma pack(pop)

enum ChunkGenState {
	CHUNK_GEN_S0, /* no blocks generated for this chunk yet */
	CHUNK_GEN_S1, /* base terrain blocks generated */
	CHUNK_GEN_S2, /* structures / plants generated */
	CHUNK_GEN_S3, /* light fully propagated */
};

enum ChunkMeshState {
	CHUNK_MESH_S0, /* no mesh generated */
	CHUNK_MESH_S1, /* gen-state 1 mesh generated */
	CHUNK_MESH_S2, /* gen-state 2 mesh generated */
	CHUNK_MESH_S3
};

enum ChunkFlags {
	BLOCK_CHANGED,
};

typedef struct game_subchunk {
	uint32_t block[CHUNK_SIZE * CHUNK_SIZE * CHUNK_SIZE];
} game_subchunk;

typedef struct game_chunk {
	int x; // actual coordinates of chunk
	int z;
	bool dirty;
	uint32_t genstate;
	uint32_t meshstate;
	int offset_y;
	int height_y;
	uint32_t subchunks[MAX_SUBCHUNKS]; // number of 16x16x16 subchunks
	// can have special subchunk indices for special subchunk types (all-air, all-solid...)
	mesh_t solid[MAP_CHUNK_HEIGHT]; // a solid mesh for each subchunk
	mesh_t alpha;
	mesh_t sprite; // render twosided (same shader as solid meshes but different render state)
	// add per-chunk state information here (things like command blocks..., entities?)
} game_chunk;

// uint32_t allocate_subchunk(map);
// void free_subchunk(map, uint32_t);
// game_subchunk* get_subchunk(map, uint32_t);
// 

// block:
// SSSSRRRRGGGGBBBBMMMMMMMMTTTTTTTT
// S: sunlight value 0-15
// R: red lamplight value 0-15
// G: green lamplight value 0-15
// B: blue lamplight value 0-15
// M: metadata 0-255
// T: blocktype 0-255

struct game_map {
	game_chunk chunks[MAP_CHUNK_WIDTH*MAP_CHUNK_WIDTH];
	// caches the block data for the area around the player
	// blocks = [blockid | meta | sunlightlevel | torchlightlevel ]
	unsigned long seed;
};

extern uint32_t* map_blocks;

void map_init(void);
void map_exit(void);
void map_tick(void);
void map_draw(frustum_t* frustum);
void map_draw_alphapass(void);
void chunk_load(int x, int z);
void chunk_mark_dirty(int x, int z);
void chunk_build_mesh_ptr(int bufx, int bufz, game_chunk* chunk);
void chunk_build_mesh(int x, int z);
void map_update_block(ivec3_t block, uint32_t value);
bool map_raycast(dvec3_t origin, vec3_t dir, int len, ivec3_t* hit, ivec3_t* prehit);
uint32_t block_at(int x, int y, int z);


// mod which handles negative numbers
static inline
int mod(int a, int b)
{
	if (b < 0) { a = -a; b = -b; }
	int ret = a % b;
	return (ret < 0) ? ret + b : ret;
}


// elements xyz are 0-32
// w should be 0-3
static inline
uint32_t packvec_1010102(unsigned int x, unsigned int y, unsigned int z, unsigned int w)
{
	assert(x <= CHUNK_SIZE && y <= CHUNK_SIZE && z <= CHUNK_SIZE && w < 4);
	return ((x)*(unsigned int)(1023/CHUNK_SIZE)) + (((y)*(unsigned int)(1023/CHUNK_SIZE))<<10) + (((z)*(unsigned int)(1023/CHUNK_SIZE))<<20) + ((w)<<30);
}


static inline
size_t block_index(int x, int y, int z)
{
	return mod(z, MAP_BLOCK_WIDTH) * (MAP_BLOCK_WIDTH * MAP_BLOCK_HEIGHT) +
		mod(x, MAP_BLOCK_WIDTH) * MAP_BLOCK_HEIGHT +
		y;
}


static inline
uint32_t* block_column(int x, int z)
{
	return map_blocks +
		mod(z, MAP_BLOCK_WIDTH) * (MAP_BLOCK_WIDTH * MAP_BLOCK_HEIGHT) +
		mod(x, MAP_BLOCK_WIDTH) * MAP_BLOCK_HEIGHT;
}


static inline
uint32_t blocktype(int x, int y, int z)
{
	return block_at(x, y, z) & 0xff;
}


static inline
size_t block_by_coord(ivec3_t xyz)
{
	return block_index(xyz.x, xyz.y, xyz.z);
}


static inline
uint32_t blocktype_by_coord(ivec3_t xyz)
{
	return blocktype(xyz.x, xyz.y, xyz.z);
}


static inline
chunkpos_t block_chunk(ivec3_t block)
{
	chunkpos_t c = { floor(round(block.x) / CHUNK_SIZE),
	               floor(round(block.z) / CHUNK_SIZE) };
	return c;
}


static inline
bool is_collider(int x, int y, int z) {
	return blockinfo[blocktype(x, y, z)].flags & BLOCK_COLLIDER;
}
