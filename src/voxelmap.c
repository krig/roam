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

uint32_t
blockType(int x, int y, int z)
{
	if (y < 0)
		return BLOCK_BLACKROCK;
	if (y >= MAP_BLOCK_HEIGHT)
		return BLOCK_AIR;
	size_t idx = blockIndex(x, y, z);
	if (idx >= MAP_BUFFER_SIZE)
		return BLOCK_AIR;
	return map_blocks[idx] & 0xff;
}

static
void set_sunlight(int x, int y, int z, uint32_t light)
{
	size_t idx = blockIndex(x, y, z);
	if (idx < MAP_BUFFER_SIZE)
		map_blocks[idx] = (map_blocks[idx] & 0xfffffff) + ((light&0xf) << 28);
}

static
void set_lamplight(int x, int y, int z, uint32_t light)
{
	size_t idx = blockIndex(x, y, z);
	if (idx < MAP_BUFFER_SIZE)
		map_blocks[idx] = (map_blocks[idx] & 0xf000ffff) + ((light&0xfff) << 16);
}

static void
set_light(int x, int y, int z, uint32_t light)
{
	size_t idx = blockIndex(x, y, z);
	if (idx < MAP_BUFFER_SIZE)
		map_blocks[idx] = (map_blocks[idx] & 0x0000ffff) + ((light&0xffff) << 16);
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
	for (int z = -VIEW_DISTANCE - 1; z < VIEW_DISTANCE + 1; ++z)
		for (int x = -VIEW_DISTANCE - 1; x < VIEW_DISTANCE + 1; ++x)
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

		for (int dz = -VIEW_DISTANCE - 1; dz < VIEW_DISTANCE + 1; ++dz) {
			for (int dx = -VIEW_DISTANCE - 1; dx < VIEW_DISTANCE + 1; ++dx) {
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
static ml_mesh* alphas[MAX_ALPHAS];
static ml_vec3 alphaoffsets[MAX_ALPHAS];
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
				alphas[nalphas] = &(chunk->alpha);
				alphaoffsets[nalphas] = offset;
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

void gameDrawAlphaPass() {
	ml_material* material;
	if (nalphas > 0) {
		material = game.materials + MAT_CHUNK_ALPHA;
		glEnable(GL_TEXTURE_2D);
		mlBindTexture2D(&blocks_texture, 0);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glDepthMask(GL_FALSE);
		glDepthFunc(GL_LEQUAL);
		glUseProgram(material->program);
		glUniform1i(material->tex0, 0);
		mlUniformMatrix(material->projmat, mlGetMatrix(&game.projection));
		mlUniformMatrix(material->modelview, mlGetMatrix(&game.modelview));
		mlUniformVec3(material->amb_light, &game.amb_light);
		mlUniformVec4(material->fog_color, &game.fog_color);

		for (size_t i = 0; i < nalphas; ++i) {
			mlUniformVec3(material->chunk_offset, alphaoffsets + i);
			mlMapMeshToMaterial(alphas[i], material);
			glDrawArrays(alphas[i]->mode, 0, alphas[i]->count);
		}

		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glUseProgram(0);
		glDisable(GL_BLEND);
		glDepthMask(GL_TRUE);
		glDisable(GL_TEXTURE_2D);
	}
}

void gameUpdateBlock(ml_ivec3 block) {
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
				if (b == BLOCK_AIR) {
					// in sunlight
				} else if (p == BLOCK_AIR && b != BLOCK_AIR) {
					// in sunlight
				} else {
					// not in sunlight
				}
				size_t idx = blockIndex(fillx, filly, fillz);
				if (idx >= MAP_BUFFER_SIZE) {
					printf("bad index: %d, %d, %d\n", fillx, filly, fillz);
					abort();
				}
				blocks[idx] = b;
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
#define POS(x, y, z) mlMakeVec3(x, y, z)//mapPackVectorChunkCoord((unsigned int)(x), (unsigned int)(y), (unsigned int)(z), 0)

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

#define BNONSOLID(x,y,z) (blockinfo[blockType(x,y,z)].density < density)

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

	size_t idx;
	uint32_t t;
	int density;

	// fill in verts
	for (iz = 0; iz < CHUNK_SIZE; ++iz) {
		for (ix = 0; ix < CHUNK_SIZE; ++ix) {
			for (iy = 0; iy < CHUNK_SIZE; ++iy) {
				idx = blockIndex(bx+ix, by+iy, bz+iz);
				t = map_blocks[idx] & 0xff;
				density = blockinfo[t].density;
				if (t == BLOCK_AIR) {
					++nprocessed;
					continue;
				}

				size_t save_vi;
				if (blockinfo[t].alpha) {
					save_vi = vi;
					verts = alpha_buffer;
					vi = *alphai;
				}

				if (BNONSOLID(bx+ix, by+iy+1, bz+iz)) {
					const tc2us_t* tc = &BLOCKTC(t, BLOCK_TEX_TOP, 0);
					const game_block_vtx corners[4] = {
						{POS(  ix, by+iy+1, iz+1), tc[0], 0xffffffff },
						{POS(ix+1, by+iy+1, iz+1), tc[1], 0xffffffff },
						{POS(ix+1, by+iy+1,   iz), tc[2], 0xffffffff },
						{POS(  ix, by+iy+1,   iz), tc[3], 0xffffffff },
					};
					verts[vi++] = corners[0];
					verts[vi++] = corners[1];
					verts[vi++] = corners[3];
					verts[vi++] = corners[3];
					verts[vi++] = corners[1];
					verts[vi++] = corners[2];
				}

				if (BNONSOLID(bx+ix, by+iy-1, bz+iz)) {
					const tc2us_t* tc = &BLOCKTC(t, BLOCK_TEX_BOTTOM, 0);
					const game_block_vtx corners[4] = {
						{POS(  ix, by+iy,   iz), tc[0], 0xffffffff },
						{POS(ix+1, by+iy,   iz), tc[1], 0xffffffff },
						{POS(ix+1, by+iy, iz+1), tc[2], 0xffffffff },
						{POS(  ix, by+iy, iz+1), tc[3], 0xffffffff },
					};
					verts[vi++] = corners[0];
					verts[vi++] = corners[1];
					verts[vi++] = corners[3];
					verts[vi++] = corners[3];
					verts[vi++] = corners[1];
					verts[vi++] = corners[2];
				}
				if (BNONSOLID(bx+ix-1, by+iy, bz+iz)) {
					const tc2us_t* tc = &BLOCKTC(t, BLOCK_TEX_LEFT, 0);
					const game_block_vtx corners[4] = {
						{POS(ix,   by+iy,   iz), tc[0], 0xffffffff },
						{POS(ix,   by+iy, iz+1), tc[1], 0xffffffff },
						{POS(ix, by+iy+1, iz+1), tc[2], 0xffffffff },
						{POS(ix, by+iy+1,   iz), tc[3], 0xffffffff },
					};
					verts[vi++] = corners[0];
					verts[vi++] = corners[1];
					verts[vi++] = corners[3];
					verts[vi++] = corners[3];
					verts[vi++] = corners[1];
					verts[vi++] = corners[2];
				}
				if (BNONSOLID(bx+ix+1, by+iy, bz+iz)) {
					const tc2us_t* tc = &BLOCKTC(t, BLOCK_TEX_RIGHT, 0);
					const game_block_vtx corners[4] = {
						{POS(ix+1,   by+iy, iz+1), tc[0], 0xffffffff },
						{POS(ix+1,   by+iy,   iz), tc[1], 0xffffffff },
						{POS(ix+1, by+iy+1,   iz), tc[2], 0xffffffff },
						{POS(ix+1, by+iy+1, iz+1), tc[3], 0xffffffff },
					};
					verts[vi++] = corners[0];
					verts[vi++] = corners[1];
					verts[vi++] = corners[3];
					verts[vi++] = corners[3];
					verts[vi++] = corners[1];
					verts[vi++] = corners[2];
				}
				if (BNONSOLID(bx+ix, by+iy, bz+iz+1)) {
					tc2us_t* tc = &BLOCKTC(t, BLOCK_TEX_FRONT, 0);
					game_block_vtx corners[4] = {
						{POS(  ix,   by+iy, iz+1), tc[0], 0xffffffff },
						{POS(ix+1,   by+iy, iz+1), tc[1], 0xffffffff },
						{POS(ix+1, by+iy+1, iz+1), tc[2], 0xffffffff },
						{POS(  ix, by+iy+1, iz+1), tc[3], 0xffffffff },
					};
					verts[vi++] = corners[0];
					verts[vi++] = corners[1];
					verts[vi++] = corners[3];
					verts[vi++] = corners[3];
					verts[vi++] = corners[1];
					verts[vi++] = corners[2];
				}
				if (BNONSOLID(bx+ix, by+iy, bz+iz-1)) {
					const tc2us_t* tc = &BLOCKTC(t, BLOCK_TEX_BACK, 0);
					const game_block_vtx corners[4] = {
						{POS(ix+1,   by+iy, iz), tc[0], 0xffffffff },
						{POS(  ix,   by+iy, iz), tc[1], 0xffffffff },
						{POS(  ix, by+iy+1, iz), tc[2], 0xffffffff },
						{POS(ix+1, by+iy+1, iz), tc[3], 0xffffffff },
					};
					verts[vi++] = corners[0];
					verts[vi++] = corners[1];
					verts[vi++] = corners[3];
					verts[vi++] = corners[3];
					verts[vi++] = corners[1];
					verts[vi++] = corners[2];
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
