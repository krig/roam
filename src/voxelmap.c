#include "common.h"
#include "math3d.h"
#include "game.h"
#include "voxelmap.h"
#include "noise.h"
#include "rnd.h"
#include "images.h"
#include "blocks.h"
#include "ui.h"

static inline
tc2us_t make_tc2us(ml_vec2 tc)
{
	tc2us_t to = {
		(uint16_t)(tc.x * (double)0xffff),
		(uint16_t)(tc.y * (double)0xffff)
	};
	return to;
}

uint32_t blockData(int x, int y, int z)
{
	if (y < 0 || y >= MAP_BLOCK_HEIGHT)
		return (0xf0000000|BLOCK_AIR);
	size_t idx = blockIndex(x, y, z);
	if (idx >= MAP_BUFFER_SIZE)
		return (0xf0000000|BLOCK_AIR);
	return map_blocks[idx];
}

void map_unload_chunk_ptr(game_chunk* chunk);

/*
  Set up a lookup table used for the texcoords of all regular blocks.
  Things with different dimensions need a different system..
 */
static tc2us_t block_texcoords[NUM_BLOCKTYPES * 6 * 4];
#define BLOCKTC(t, f, i) block_texcoords[(t)*(6*4) + (f)*4 + (i)]
static
void gen_block_tcs()
{
	float bw = IMG_TCW - IMG_TC_BIAS;
	ml_vec2 tcoffs[4] = {{0, bw}, {bw, bw}, {bw, 0}, {0, 0} };
	for (int t = 0; t < NUM_BLOCKTYPES; ++t) {
		for (int i = 0; i < 6; ++i) {
			ml_vec2 tl = mlVec2AddScalar(imgTC(blockinfo[t].img[i]), IMG_TC_BIAS);
			for (int j = 0; j < 4; ++j)
				BLOCKTC(t, i, j) = make_tc2us(mlVec2Add(tl, tcoffs[j]));
		}
	}
}

extern ml_tex2d blocks_texture;
uint32_t* map_blocks = NULL;
static ml_chunk map_chunk;

#define TESSELATION_QUEUE_SIZE (MAP_CHUNK_WIDTH*MAP_CHUNK_WIDTH)
static ml_chunk tesselation_queue[TESSELATION_QUEUE_SIZE];
static size_t tesselation_queue_head = 0;
static size_t tesselation_queue_len = 0;

static bool pushChunkTesselation(int x, int z) {
	ml_chunk chunk = { x, z };
	if (tesselation_queue_len >= TESSELATION_QUEUE_SIZE)
		return false;
	size_t pos = (tesselation_queue_head + tesselation_queue_len) % TESSELATION_QUEUE_SIZE;
	tesselation_queue[pos] = chunk;
	++tesselation_queue_len;
	return true;
}

static bool popChunkTesselation(ml_chunk* chunk) {
	if (tesselation_queue_len == 0)
		return false;
	*chunk = tesselation_queue[tesselation_queue_head];
	tesselation_queue_head = (tesselation_queue_head + 1) % TESSELATION_QUEUE_SIZE;
	return true;
}

void gameInitMap() {
	blocks_init();
	gen_block_tcs();

	printf("* Allocate and build initial map...\n");
	memset(&game.map, 0, sizeof(game_map));
	map_blocks = (uint32_t*)malloc(sizeof(uint32_t)*MAP_BUFFER_SIZE);
	memset(map_blocks, 0, sizeof(uint32_t)*MAP_BUFFER_SIZE);

	game.map.seed = sys_urandom();
	printf("* Seed: %lx\n", game.map.seed);
	simplexInit(game.map.seed);
	osnInit(game.map.seed);

	ml_chunk camera = playerChunk();
	for (int z = -VIEW_DISTANCE; z < VIEW_DISTANCE; ++z)
		for (int x = -VIEW_DISTANCE; x < VIEW_DISTANCE; ++x)
			gameLoadChunk(camera.x + x, camera.z + z);

	// tesselate column
	for (int z = -VIEW_DISTANCE; z < VIEW_DISTANCE; ++z) {
		for (int x = -VIEW_DISTANCE; x < VIEW_DISTANCE; ++x) {
			if (!pushChunkTesselation(camera.x + x, camera.z + z)) {
				gameTesselateChunk(camera.x + x, camera.z + z);
			}
		}
	}
	map_chunk = camera;

	gameUpdateMap();
	printf("* Map load complete.\n");
}

void gameFreeMap() {
	for (size_t i = 0; i < MAP_CHUNK_WIDTH*MAP_CHUNK_WIDTH; ++i)
		map_unload_chunk_ptr(game.map.chunks + i);

	free(map_blocks);
	map_blocks = NULL;
}

void gameUpdateMap() {
	ml_chunk nc = playerChunk();
	if (nc.x != map_chunk.x || nc.z != map_chunk.z) {
		game_chunk* chunks = game.map.chunks;
		int cx = nc.x;
		int cz = nc.z;
		printf("[%d, %d] -> [%d, %d] (%g, %g)\n",
		       map_chunk.x, map_chunk.z, cx, cz,
		       game.camera.pos.x, game.camera.pos.z);
		map_chunk = nc;

		for (int dz = -VIEW_DISTANCE; dz < VIEW_DISTANCE; ++dz) {
			for (int dx = -VIEW_DISTANCE; dx < VIEW_DISTANCE; ++dx) {
				int bx = mod(cx + dx, MAP_CHUNK_WIDTH);
				int bz = mod(cz + dz, MAP_CHUNK_WIDTH);
				game_chunk* chunk = chunks + (bz*MAP_CHUNK_WIDTH + bx);
				if (chunk->x != cx + dx ||
				    chunk->z != cz + dz) {
					gameUnloadChunk(chunk->x, chunk->z);
					gameLoadChunk(cx + dx, cz + dz);
					chunk = chunks + (bz*MAP_CHUNK_WIDTH + bx);
					assert(chunk->x == (cx + dx) && chunk->z == (cz + dz));
				}
			}
		}
		for (int dz = -VIEW_DISTANCE; dz < VIEW_DISTANCE; ++dz) {
			for (int dx = -VIEW_DISTANCE; dx < VIEW_DISTANCE; ++dx) {
				if (!pushChunkTesselation(cx + dx, cz + dz)) {
					gameTesselateChunk(cx + dx, cz + dz);
				}
			}
		}
	}
	ml_chunk chunk;
	for (int i = 0; i < 5; ++i) {
		if (popChunkTesselation(&chunk)) {
			gameTesselateChunk(chunk.x, chunk.z);
		}
	}
}

#define MAX_ALPHAS ((VIEW_DISTANCE*2)*(VIEW_DISTANCE*2))

struct alpha_t {
	ml_mesh* mesh;
	ml_vec3 offset;
};

static struct alpha_t alphas[MAX_ALPHAS];
static size_t nalphas;

void gameDrawMap(ml_frustum* frustum) {
	// for each visible chunk...
	// set up material etc. once.
	ml_material* material = game.materials + MAT_CHUNK;

	glDisable(GL_BLEND);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_TEXTURE_2D);
	mlBindTexture2D(&blocks_texture, 0);
	glUseProgram(material->program);

	glUniform1i(material->tex0, 0);
	mlUniformMatrix(material->projmat, mlGetMatrix(&game.projection));
	mlUniformMatrix(material->modelview, mlGetMatrix(&game.modelview));
	mlUniformVec3(material->amb_light, &game.amb_light);
	mlUniformVec4(material->fog_color, &game.fog_color);

	float chunk_radius = (float)CHUNK_SIZE*0.5f;
	ml_vec3 offset, center, extent;
	game_chunk* chunks = game.map.chunks;
	ml_chunk camera = playerChunk();

	int dx, dz, bx, bz, x, z, j;
	game_chunk* chunk;
	ml_mesh* mesh;

	nalphas = 0;

	for (dz = -VIEW_DISTANCE; dz < VIEW_DISTANCE; ++dz) {
		for (dx = -VIEW_DISTANCE; dx < VIEW_DISTANCE; ++dx) {
			bx = mod(camera.x + dx, MAP_CHUNK_WIDTH);
			bz = mod(camera.z + dz, MAP_CHUNK_WIDTH);
			chunk = chunks + (bz*MAP_CHUNK_WIDTH + bx);
			x = chunk->x - camera.x;
			z = chunk->z - camera.z;

			mlVec3Assign(offset, (float)(x*CHUNK_SIZE) - 0.5f, -0.5f, (float)(z*CHUNK_SIZE) - 0.5f);
			mlVec3Assign(center, offset.x + chunk_radius, MAP_BLOCK_HEIGHT*0.5f, offset.z + chunk_radius);
			mlVec3Assign(extent, chunk_radius, MAP_BLOCK_HEIGHT*0.5f, chunk_radius);
			if (mlTestFrustumAABB_XZ(frustum, center, extent) == ML_OUTSIDE)
				continue;

			if (game.debug_mode && chunk->dirty) {
				ui_debug_aabb(center, extent, 0x44ff2222);
				continue;
			}

			extent.y = chunk_radius;

			mlUniformVec3(material->chunk_offset, &offset);

			if (chunk->alpha.vbo != 0 && nalphas < MAX_ALPHAS) {
				alphas[nalphas].mesh = &(chunk->alpha);
				alphas[nalphas].offset = offset;
				nalphas++;
			}

			for (j = 0; j < MAP_CHUNK_HEIGHT; ++j) {
				mesh = chunk->solid + j;
				if (mesh->vbo == 0)
					continue;
				center.y = (float)(CHUNK_SIZE*j) - 0.5f + chunk_radius;
				if (mlTestFrustumAABB_Y(frustum, center, extent) == ML_OUTSIDE)
					continue;
				mlMapMeshToMaterial(mesh, material);
				glDrawArrays(mesh->mode, 0, mesh->count);
			}
		}
	}

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glUseProgram(0);
	glDisable(GL_BLEND);
	glDepthMask(GL_TRUE);
	glDisable(GL_TEXTURE_2D);
}

static
int alpha_sort(const void* va, const void* vb)
{
	struct alpha_t *a = (struct alpha_t *)va;
	struct alpha_t *b = (struct alpha_t *)vb;
	int ret = 0;
	float da = fabs(a->offset.x + a->offset.z);
	float db = fabs(b->offset.x + b->offset.z);
	if (da < db)
		ret = -1;
	if (da > db)
		ret = 1;
	return ret;
}

void gameDrawAlphaPass() {
	size_t i;
	ml_material* material;
	struct alpha_t* alpha;
	if (nalphas == 0)
		return;

	// TODO: alphasort faces in chunk
	qsort(alphas, nalphas, sizeof(struct alpha_t), alpha_sort);

	material = game.materials + MAT_CHUNK_ALPHA;
	glEnable(GL_TEXTURE_2D);
	mlBindTexture2D(&blocks_texture, 0);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDepthMask(GL_FALSE);
	glDepthFunc(GL_LESS);
	glUseProgram(material->program);
	glUniform1i(material->tex0, 0);
	mlUniformMatrix(material->projmat, mlGetMatrix(&game.projection));
	mlUniformMatrix(material->modelview, mlGetMatrix(&game.modelview));
	mlUniformVec3(material->amb_light, &game.amb_light);
	mlUniformVec4(material->fog_color, &game.fog_color);

	for (i = 0, alpha = alphas; i < nalphas; ++i, ++alpha) {
		mlUniformVec3(material->chunk_offset, &alpha->offset);
		mlMapMeshToMaterial(alpha->mesh, material);
		glDrawArrays(alpha->mesh->mode, 0, alpha->mesh->count);
	}

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glUseProgram(0);
	glDisable(GL_BLEND);
	glDepthMask(GL_TRUE);
	glDisable(GL_TEXTURE_2D);
}

void gameUpdateBlock(ml_ivec3 block, uint32_t value) {
	size_t idx = blockByCoord(block);

	// relight column down (fixes sunlight)
	uint32_t sunlight = 0xf0000000;
	map_blocks[idx] = value;
	idx -= block.y;
	for (int dy = MAP_BLOCK_HEIGHT-1; dy >= 0; --dy) {
		uint32_t t = map_blocks[idx + dy];
		if ((t & 0xff) != BLOCK_AIR)
			sunlight = 0;
		map_blocks[idx + dy] = (t & 0x0fffffff) | sunlight;
	}
	// TODO: need to re-propagate light from lightsources affected by this change

	ml_chunk chunk = blockToChunk(block);
	bool tess[4] = { false, false, false, false };
	int mx = block.x % CHUNK_SIZE;
	int mz = block.z % CHUNK_SIZE;
	if (mx == 0 || mx == -CHUNK_SIZE-1) {
		tess[0] = true;
	} else if (mx == -1 || mx == CHUNK_SIZE-1) {
		tess[1] = true;
	}
	if (mz == 0 || mz == -CHUNK_SIZE-1) {
		tess[2] = true;
	} else if (mz == -1 || mz == CHUNK_SIZE-1) {
		tess[3] = true;
	}
	printf("reload chunk [%d, %d] [%d, %d]\n", chunk.x, chunk.z, mx, mz);
	gameUnloadChunk(chunk.x, chunk.z);
	gameTesselateChunk(chunk.x, chunk.z);
	if (tess[0]) {
		printf("reload chunk [%d, %d]\n", chunk.x-1, chunk.z);
		gameUnloadChunk(chunk.x-1, chunk.z);
		gameTesselateChunk(chunk.x-1, chunk.z);
	}
	if (tess[1]) {
		printf("reload chunk [%d, %d]\n", chunk.x+1, chunk.z);
		gameUnloadChunk(chunk.x+1, chunk.z);
		gameTesselateChunk(chunk.x+1, chunk.z);
	}
	if (tess[2]) {
		printf("reload chunk [%d, %d]\n", chunk.x, chunk.z-1);
		gameUnloadChunk(chunk.x, chunk.z-1);
		gameTesselateChunk(chunk.x, chunk.z-1);
	}
	if (tess[3]) {
		printf("reload chunk [%d, %d]\n", chunk.x, chunk.z+1);
		gameUnloadChunk(chunk.x, chunk.z+1);
		gameTesselateChunk(chunk.x, chunk.z+1);
	}
}

// propagate light from x,y,z
// so (x,y,z) is the light "source"
//   push (x,y,z) to process queue
//   check its neighbours
//   if neighbour is !solid and...
//      has lightlevel < this - 2,
//       increase their lightlevel
//       add that neighbour to propagation queue
//   loop until queue is empty
//   neighbour data can be packed into uint8[3]
// TODO: per-thread queue, queue retesselation of
// lit chunks as they are modified
void propagate_light(int x, int y, int z) {
}


// TODO: chunk saving/loading
// TODO: asynchronous
// as a test, just fill in the designated chunk
// and tesselate the whole thing

void gameLoadChunk(int x, int z) {
	int blockx, blockz;
	int fillx, filly, fillz;
	int bufx = mod(x, MAP_CHUNK_WIDTH);
	int bufz = mod(z, MAP_CHUNK_WIDTH);
	game_chunk* chunk = game.map.chunks + (bufz*MAP_CHUNK_WIDTH + bufx);
	uint32_t* blocks = map_blocks;
	chunk->x = x;
	chunk->z = z;
	chunk->dirty = true;
	//printf("load: (%d, %d) [%d, %d]\n", x, z, bufx, bufz);

	blockx = x * CHUNK_SIZE;
	blockz = z * CHUNK_SIZE;

#define NOISE_SCALE (1.0/((double)CHUNK_SIZE * 16))

	uint64_t snowseed = game.map.seed ^ ((uint64_t)blockx << 32) ^ (uint64_t)blockz;

	uint32_t p, b;
	for (fillz = blockz; fillz < blockz + CHUNK_SIZE; ++fillz) {
		for (fillx = blockx; fillx < blockx + CHUNK_SIZE; ++fillx) {
			size_t idx0 = blockIndex(fillx, 0, fillz);
			if (idx0 >= MAP_BUFFER_SIZE) {
				printf("bad index: %d, %d, %d\n", fillx, 0, fillz);
				abort();
			}
			uint32_t sunlight = 0xf0000000;
			double height = fBmSimplex(fillx, fillz, 0.5, NOISE_SCALE, 2.1117, 5);
			height = 32.0 + height * 24.0;
			b = BLOCK_AIR;
			p = BLOCK_AIR;
			for (filly = MAP_BLOCK_HEIGHT-1; filly >= 0; --filly) {
				if (filly > height && filly > OCEAN_LEVEL) {
					b = BLOCK_AIR;
				}
				else if (filly < 2) {
					b = BLOCK_BLACKROCK;
				} else if (filly < height) {
					if (p == BLOCK_AIR) {
						snowseed = osnRand(snowseed);
						if (height > (140.0 - (snowseed % 10))) {
							b = BLOCK_SNOWY_GRASS_1;
						} else {
							b = BLOCK_WET_GRASS;
						}
					} else {
						b = BLOCK_WET_DIRT;
					}
				} else if (filly <= OCEAN_LEVEL) {
					b = BLOCK_OCEAN;
				} else {
					b = BLOCK_AIR;
				}
				if (b != BLOCK_AIR)
					sunlight = 0;
				blocks[idx0 + filly] = b | sunlight;
				p = b;
			}
		}
	}

	/*
	for (int i = 0; i < NUM_BLOCKTYPES; ++i) {
		int x = (i*2) % CHUNK_SIZE;
		int z = ((i*2) / CHUNK_SIZE) * 2;
		blocks[blockIndex(blockx + x, OCEAN_LEVEL + 2, blockz + z)] = i;
		}
	*/
}

void map_unload_chunk_ptr(game_chunk* chunk)
{
	for (int i = 0; i < MAP_CHUNK_HEIGHT; ++i)
		mlDestroyMesh(chunk->solid + i);
	mlDestroyMesh(&chunk->alpha);
	chunk->dirty = true;
	//printf("unloaded chunk: [%d, %d] [%d, %d]\n", x, z, bufx, bufz);
}

void gameUnloadChunk(int x, int z)
{
	int bufx = mod(x, MAP_CHUNK_WIDTH);
	int bufz = mod(z, MAP_CHUNK_WIDTH);
	game_chunk* chunk = game.map.chunks + (bufz*MAP_CHUNK_WIDTH + bufx);
	if (chunk->x != x || chunk->z != z)
		return;
	map_unload_chunk_ptr(chunk);
}

// tesselation buffer: size is maximum number of triangles generated
//   1: fill tesselation buffer
//  2: allocate mesh
//   3: fill vertices
//   returns num verts in chunk

#define ALPHA_BUFFER_SIZE (CHUNK_SIZE*CHUNK_SIZE*CHUNK_SIZE*36)
static game_block_vtx alpha_buffer[ALPHA_BUFFER_SIZE];

#define TESSELATION_BUFFER_SIZE (CHUNK_SIZE*CHUNK_SIZE*CHUNK_SIZE*36)
static game_block_vtx tesselation_buffer[TESSELATION_BUFFER_SIZE];

bool gameTesselateSubChunk(ml_mesh* mesh, int bufx, int bufz, int cy, size_t* alphai);

void gameTesselateChunk(int x, int z) {
	int bufx = mod(x, MAP_CHUNK_WIDTH);
	int bufz = mod(z, MAP_CHUNK_WIDTH);
	game_chunk* chunk = game.map.chunks + bufz*MAP_CHUNK_WIDTH + bufx;
	if (chunk->x != x || chunk->z != z)
		return;
	if (chunk->dirty) {
		size_t alphai = 0;
		ml_mesh* mesh = chunk->solid;
		for (int y = 0; y < MAP_CHUNK_HEIGHT; ++y)
			gameTesselateSubChunk(mesh + y, bufx, bufz, y, &alphai);
		chunk->dirty = false;
		//printf("tesselated chunk: [%d, %d] [%d, %d]\n", x, z, bufx, bufz);
		if (alphai > 0) {
			ml_mesh* alpha = &(chunk->alpha);
			mlCreateMesh(alpha, alphai, alpha_buffer, ML_POS_3F | ML_TC_2US | ML_CLR_4UB);
		}
	}
}

// Interleave lower 16 bits of x and y in groups of 4
// ie turn 0xabcd into 0xaabbccdd
static
uint32_t bitexpand16(uint32_t x) {
	x = ((x << 12) & 0xF000000) + ((x << 8) & 0xF0000) + ((x << 4) & 0xF00) + (x & 0xF);
	return x | (x << 4);
}

// turn 0xaabbccdd into 0xabcd
static
uint32_t bitcontract16(uint32_t x) {
	return ((x >> 16) & 0xF000) + ((x >> 12) & 0xF00) + ((x >> 8) & 0xF0) + ((x & 0xF0) >> 4);
}

// take light from the given four blocks and mix it into a single light value
// assert(avglight(0xabcd0000, 0xabcd0000, 0xabcd0000, 0xabcd0000) == 0xafbfcfdf);
// assert(avglight(0, 0, 0, 0) == 0x0f0f0f0f);
// assert(avglight(0xffff0000, 0xffff0000, 0xffff0000, 0xffff0000) == 0xffff0000);
// MC smooth lighting: average light of four blocks around vert on the positive face side
static
uint32_t avglight(uint32_t b0, uint32_t b1, uint32_t b2, uint32_t b3) {
	static const uint32_t M[4] = { 0xf0000000, 0xf000000, 0xf00000, 0xf0000 };
	static const uint32_t S[4] = { 28, 24, 20, 16 };
	uint32_t ret = 0;
	ret |= ((((b0 & M[0]) >> S[0]) + ((b1 & M[0]) >> S[0]) + ((b2 & M[0]) >> S[0]) + ((b3 & M[0]) >> S[0]) + 4) * 4 - 1) << 24;
	ret |= ((((b0 & M[1]) >> S[1]) + ((b1 & M[1]) >> S[1]) + ((b2 & M[1]) >> S[1]) + ((b3 & M[1]) >> S[1]) + 4) * 4 - 1) << 16;
	ret |= ((((b0 & M[2]) >> S[2]) + ((b1 & M[2]) >> S[2]) + ((b2 & M[2]) >> S[2]) + ((b3 & M[2]) >> S[2]) + 4) * 4 - 1) << 8;
	ret |= ((((b0 & M[3]) >> S[3]) + ((b1 & M[3]) >> S[3]) + ((b2 & M[3]) >> S[3]) + ((b3 & M[3]) >> S[3]) + 4) * 4 - 1);
	return ret;
}

// TODO: calculate index of surrounding blocks directly without going through blockType
#define POS(x, y, z) mlMakeVec3(x, y, z)
#define BNONSOLID(t) (blockinfo[(n[(t)] & 0xff)].density < density)
//#define BNONSOLID(t) ((n[(t)] & 0xff) == BLOCK_AIR)
#define BLOCKAT(x, y, z) (map_blocks[blockIndex((x), (y), (z))])
#define BLOCKLIGHT(a, b, c, d) avglight(n[a], n[b], n[c], n[d])
#define GETCOL(np, ng, x, y, z) memcpy(n + (np), map_blocks + blockIndex(bx + (x), by + (y), bz + (z)), sizeof(uint32_t) * (ng))
#define FLIPCHECK() ((corners[0].clr/2) + (corners[2].clr/2) > (corners[1].clr/2) + (corners[3].clr/2))

// n array layout:
//+y       +y       +y
// \ 2 5 8  | b e h  | k n q
// \ 1 4 7  | a d g  | j m p
// \ 0 3 6  | 9 c f  | i l o
// +-----+x +-----+x +-----+x
//  (iz-1)   (iz)     (iz+1)


bool gameTesselateSubChunk(ml_mesh* mesh, int bufx, int bufz, int cy, size_t* alphai) {
	int ix, iy, iz;
	int bx, by, bz;
	size_t vi;
	game_block_vtx* verts;

	verts = tesselation_buffer;
	vi = 0;
	bx = bufx*CHUNK_SIZE;
	by = cy*CHUNK_SIZE;
	bz = bufz*CHUNK_SIZE;
	size_t nprocessed = 0;

	size_t idx0;
	uint32_t t;
	uint32_t n[27]; // blocktypes for a 3x3 cube around this block
	int density;
	size_t save_vi;

	// fill in verts
	for (iz = 0; iz < CHUNK_SIZE; ++iz) {
		for (ix = 0; ix < CHUNK_SIZE; ++ix) {
			idx0 = blockIndex(bx+ix, by, bz+iz);
			for (iy = 0; iy < CHUNK_SIZE; ++iy) {
				t = map_blocks[idx0 + iy] & 0xff;
				density = blockinfo[t].density;
				if (t == BLOCK_AIR) {
					++nprocessed;
					continue;
				}

				if (blockinfo[t].alpha) {
					save_vi = vi;
					verts = alpha_buffer;
					vi = *alphai;
				}

				if (by+iy+1 >= MAP_BLOCK_HEIGHT) {
					n[2] = n[5] = n[8] = n[11] = n[14] = n[17] = n[20] = n[23] = n[26] = (0xf0000000|BLOCK_AIR);
					GETCOL(0, 2, ix - 1, iy - 1, iz - 1);
					GETCOL(3, 2, ix, iy - 1, iz - 1);
					GETCOL(6, 2, ix + 1, iy - 1, iz - 1);
					GETCOL(9, 2, ix - 1, iy - 1, iz);
					GETCOL(12, 2, ix, iy - 1, iz);
					GETCOL(15, 2, ix + 1, iy - 1, iz);
					GETCOL(18, 2, ix - 1, iy - 1, iz + 1);
					GETCOL(21, 2, ix, iy - 1, iz + 1);
					GETCOL(24, 2, ix + 1, iy - 1, iz + 1);
				} else if (by+iy-1 < 0) {
					n[0] = n[3] = n[6] = n[9] = n[12] = n[15] = n[18] = n[21] = n[24] = (0xf0000000|BLOCK_AIR);
					GETCOL(1, 2, ix - 1, iy, iz - 1);
					GETCOL(4, 2, ix, iy, iz - 1);
					GETCOL(7, 2, ix + 1, iy, iz - 1);
					GETCOL(10, 2, ix - 1, iy, iz);
					GETCOL(13, 2, ix, iy, iz);
					GETCOL(16, 2, ix + 1, iy, iz);
					GETCOL(19, 2, ix - 1, iy, iz + 1);
					GETCOL(22, 2, ix, iy, iz + 1);
					GETCOL(25, 2, ix + 1, iy, iz + 1);
				} else {
					GETCOL(0, 3, ix - 1, iy - 1, iz - 1);
					GETCOL(3, 3, ix, iy - 1, iz - 1);
					GETCOL(6, 3, ix + 1, iy - 1, iz - 1);
					GETCOL(9, 3, ix - 1, iy - 1, iz);
					GETCOL(12, 3, ix, iy - 1, iz);
					GETCOL(15, 3, ix + 1, iy - 1, iz);
					GETCOL(18, 3, ix - 1, iy - 1, iz + 1);
					GETCOL(21, 3, ix, iy - 1, iz + 1);
					GETCOL(24, 3, ix + 1, iy - 1, iz + 1);
				}

				if (BNONSOLID(14)) {
					assert((n[14]&0xff) != (n[13]&0xff));
					const tc2us_t* tc = &BLOCKTC(t, BLOCK_TEX_TOP, 0);
					const game_block_vtx corners[4] = {
						{POS(  ix, by+iy+1, iz+1), tc[0], BLOCKLIGHT(20,23,11,14)},
						{POS(ix+1, by+iy+1, iz+1), tc[1], BLOCKLIGHT(23,26,14,17)},
						{POS(ix+1, by+iy+1,   iz), tc[2], BLOCKLIGHT( 5, 8,14,17)},
						{POS(  ix, by+iy+1,   iz), tc[3], BLOCKLIGHT( 2, 5,11,14)},
					};
					if (FLIPCHECK()) {
						verts[vi++] = corners[0];
						verts[vi++] = corners[1];
						verts[vi++] = corners[2];
						verts[vi++] = corners[2];
						verts[vi++] = corners[3];
						verts[vi++] = corners[0];
					} else {
						verts[vi++] = corners[0];
						verts[vi++] = corners[1];
						verts[vi++] = corners[3];
						verts[vi++] = corners[3];
						verts[vi++] = corners[1];
						verts[vi++] = corners[2];
					}
				}
				if (BNONSOLID(12)) {
					const tc2us_t* tc = &BLOCKTC(t, BLOCK_TEX_BOTTOM, 0);
					const game_block_vtx corners[4] = {
						{POS(  ix, by+iy,   iz), tc[0], BLOCKLIGHT( 0, 3, 9,12)},
						{POS(ix+1, by+iy,   iz), tc[1], BLOCKLIGHT( 3, 6,12,15)},
						{POS(ix+1, by+iy, iz+1), tc[2], BLOCKLIGHT(12,15,21,24)},
						{POS(  ix, by+iy, iz+1), tc[3], BLOCKLIGHT( 9,12,18,21)},
					};
					if (FLIPCHECK()) {
						verts[vi++] = corners[0];
						verts[vi++] = corners[1];
						verts[vi++] = corners[2];
						verts[vi++] = corners[2];
						verts[vi++] = corners[3];
						verts[vi++] = corners[0];
					} else {
						verts[vi++] = corners[0];
						verts[vi++] = corners[1];
						verts[vi++] = corners[3];
						verts[vi++] = corners[3];
						verts[vi++] = corners[1];
						verts[vi++] = corners[2];
					}
				}
				if (BNONSOLID(10)) {
					const tc2us_t* tc = &BLOCKTC(t, BLOCK_TEX_LEFT, 0);
					const game_block_vtx corners[4] = {
						{POS(ix,   by+iy,   iz), tc[0], BLOCKLIGHT( 0, 1, 9,10)},
						{POS(ix,   by+iy, iz+1), tc[1], BLOCKLIGHT( 9,10,18,19)},
						{POS(ix, by+iy+1, iz+1), tc[2], BLOCKLIGHT(10,11,19,20)},
						{POS(ix, by+iy+1,   iz), tc[3], BLOCKLIGHT( 1, 2,10,11)},
					};
					if (FLIPCHECK()) {
						verts[vi++] = corners[0];
						verts[vi++] = corners[1];
						verts[vi++] = corners[2];
						verts[vi++] = corners[2];
						verts[vi++] = corners[3];
						verts[vi++] = corners[0];
					} else {
						verts[vi++] = corners[0];
						verts[vi++] = corners[1];
						verts[vi++] = corners[3];
						verts[vi++] = corners[3];
						verts[vi++] = corners[1];
						verts[vi++] = corners[2];
					}
				}
				if (BNONSOLID(16)) {
					const tc2us_t* tc = &BLOCKTC(t, BLOCK_TEX_RIGHT, 0);
					const game_block_vtx corners[4] = {
						{POS(ix+1,   by+iy, iz+1), tc[0], BLOCKLIGHT(15,16,24,25)},
						{POS(ix+1,   by+iy,   iz), tc[1], BLOCKLIGHT( 6, 7,15,16)},
						{POS(ix+1, by+iy+1,   iz), tc[2], BLOCKLIGHT( 7, 8,16,17)},
						{POS(ix+1, by+iy+1, iz+1), tc[3], BLOCKLIGHT(16,17,25,26)},
					};
					if (FLIPCHECK()) {
						verts[vi++] = corners[0];
						verts[vi++] = corners[1];
						verts[vi++] = corners[2];
						verts[vi++] = corners[2];
						verts[vi++] = corners[3];
						verts[vi++] = corners[0];
					} else {
						verts[vi++] = corners[0];
						verts[vi++] = corners[1];
						verts[vi++] = corners[3];
						verts[vi++] = corners[3];
						verts[vi++] = corners[1];
						verts[vi++] = corners[2];
					}
				}
				if (BNONSOLID(22)) {
					tc2us_t* tc = &BLOCKTC(t, BLOCK_TEX_FRONT, 0);
					game_block_vtx corners[4] = {
						{POS(  ix,   by+iy, iz+1), tc[0], BLOCKLIGHT(18,19,21,22)},
						{POS(ix+1,   by+iy, iz+1), tc[1], BLOCKLIGHT(21,22,24,25)},
						{POS(ix+1, by+iy+1, iz+1), tc[2], BLOCKLIGHT(22,23,25,26)},
						{POS(  ix, by+iy+1, iz+1), tc[3], BLOCKLIGHT(19,20,22,23)},
					};
					if (FLIPCHECK()) {
						verts[vi++] = corners[0];
						verts[vi++] = corners[1];
						verts[vi++] = corners[2];
						verts[vi++] = corners[2];
						verts[vi++] = corners[3];
						verts[vi++] = corners[0];
					} else {
						verts[vi++] = corners[0];
						verts[vi++] = corners[1];
						verts[vi++] = corners[3];
						verts[vi++] = corners[3];
						verts[vi++] = corners[1];
						verts[vi++] = corners[2];
					}
				}
				if (BNONSOLID(4)) {
					const tc2us_t* tc = &BLOCKTC(t, BLOCK_TEX_BACK, 0);
					const game_block_vtx corners[4] = {
						{POS(ix+1,   by+iy, iz), tc[0], BLOCKLIGHT( 3, 4, 6, 7)},
						{POS(  ix,   by+iy, iz), tc[1], BLOCKLIGHT( 0, 1, 3, 4)},
						{POS(  ix, by+iy+1, iz), tc[2], BLOCKLIGHT( 1, 2, 4, 5)},
						{POS(ix+1, by+iy+1, iz), tc[3], BLOCKLIGHT( 4, 5, 7, 8)},
					};
					if (FLIPCHECK()) {
						verts[vi++] = corners[0];
						verts[vi++] = corners[1];
						verts[vi++] = corners[2];
						verts[vi++] = corners[2];
						verts[vi++] = corners[3];
						verts[vi++] = corners[0];
					} else {
						verts[vi++] = corners[0];
						verts[vi++] = corners[1];
						verts[vi++] = corners[3];
						verts[vi++] = corners[3];
						verts[vi++] = corners[1];
						verts[vi++] = corners[2];
					}
				}

				if (blockinfo[t].alpha) {
					if (vi > ALPHA_BUFFER_SIZE)
						fatal_error("Alpha buffer too small for chunk (%d, %d, %d): %zu verts, %zu blocks processed", bufx, cy, bufz, vi, nprocessed);
					*alphai = vi;
					vi = save_vi;
					verts = tesselation_buffer;
				}

				++nprocessed;

				if (vi > TESSELATION_BUFFER_SIZE)
					fatal_error("Tesselation buffer too small for chunk (%d, %d, %d): %zu verts, %zu blocks processed", bufx, cy, bufz, vi, nprocessed);
			}
		}
	}

	if (vi > 0)
		mlCreateMesh(mesh, vi, verts, ML_POS_3F | ML_TC_2US | ML_CLR_4UB);
	return (vi > 0);
}

bool gameRayTest(ml_dvec3 origin, ml_vec3 dir, int len, ml_ivec3* hit, ml_ivec3* prehit) {
	ml_dvec3 blockf = { origin.x, origin.y, origin.z };
	ml_ivec3 block = { round(origin.x), round(origin.y), round(origin.z) };
	ml_ivec3 prev = {0, 0, 0};
	int step = 32;
	for (int i = 0; i < len*step; ++i) {
		block.x = round(blockf.x);
		block.y = round(blockf.y);
		block.z = round(blockf.z);
		if (block.x != prev.x || block.y != prev.y || block.z != prev.z) {
			uint8_t t = blockTypeByCoord(block);
			if (t != BLOCK_AIR) {
				//if (game.debug_mode) {
				//	ui_debug_block(prev, 0xff0000ff);
				//	ui_debug_block(block, 0xff00ff00);
				//}
				if (hit != NULL)
					*hit = block;
				if (prehit != NULL)
					*prehit = prev;
				return true;
			}
			prev = block;
		}
		blockf.x += dir.x / step;
		blockf.y += dir.y / step;
		blockf.z += dir.z / step;
	}
	return false;
}
