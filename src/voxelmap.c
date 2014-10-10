#include "common.h"
#include "math3d.h"
#include "game.h"
#include "voxelmap.h"
#include "noise.h"
#include "rnd.h"
#include <time.h>
#include <assert.h>

static inline size_t blockIndex(int x, int y, int z) {
	return mod(z, MAP_BLOCK_WIDTH) * (MAP_BLOCK_WIDTH * MAP_BLOCK_HEIGHT) +
		mod(x, MAP_BLOCK_WIDTH) * MAP_BLOCK_HEIGHT +
		y;
}

static inline uint8_t blockType(int x, int y, int z) {
	if (y < 0)
		return BLOCK_DARKSTONE;
	if (y >= MAP_BLOCK_HEIGHT)
		return BLOCK_AIR;
	size_t idx = blockIndex(x, y, z);
	if (idx >= MAP_BUFFER_SIZE)
		return BLOCK_AIR;
	return game.map.blocks[idx].type;
}

extern ml_tex2d blocks_texture;

static tc2us_t block_texcoords[NUM_BLOCKTYPES * 6 * 4];

extern struct blockinfo_t blockinfo[];

void gameLoadChunk(int x, int z);
size_t gameTesselateSubChunk(int cx, int cy, int cz);

#define TC_BIAS (1.0/4000.0)

static inline tc2us_t tc2us(ml_vec2 tc) {
	tc2us_t to = {
		(uint16_t)(tc.x * (double)0xffff),
		(uint16_t)(tc.y * (double)0xffff)
	};
	return to;
}

#define BLOCKTC(t, f, i) block_texcoords[(t)*(6*4) + (f)*4 + (i)]

static void initBlockTexcoords() {
	ml_vec2 tcoffs[4] = {
		{0, 0},
		{0, BLOCK_TC_W - TC_BIAS},
		{BLOCK_TC_W - TC_BIAS, 0},
		{BLOCK_TC_W - TC_BIAS, BLOCK_TC_W - TC_BIAS}
	};
	for (int t = 0; t < NUM_BLOCKTYPES; ++t) {
		for (int i = 0; i < 6; ++i) {
			BLOCKTC(t, i, 0) = tc2us(mlVec2Add(mlVec2AddScalar(idx2tc(blockinfo[t].tex[i]), TC_BIAS), tcoffs[0]));
			BLOCKTC(t, i, 1) = tc2us(mlVec2Add(mlVec2AddScalar(idx2tc(blockinfo[t].tex[i]), TC_BIAS), tcoffs[1]));
			BLOCKTC(t, i, 2) = tc2us(mlVec2Add(mlVec2AddScalar(idx2tc(blockinfo[t].tex[i]), TC_BIAS), tcoffs[2]));
			BLOCKTC(t, i, 3) = tc2us(mlVec2Add(mlVec2AddScalar(idx2tc(blockinfo[t].tex[i]), TC_BIAS), tcoffs[3]));
		}
	}
}

void gameInitMap() {
	printf("* Allocate and build initial map...\n");

	initBlockTexcoords();

	memset(&game.map, 0, sizeof(game_map));
	game.map.seed = good_seed();
	printf("* Seed: %lx\n", game.map.seed);
	simplexInit(game.map.seed);
	osnInit(game.map.seed);

	/*
	for (size_t i = 0; i < MAP_BUFFER_SIZE; ++i) {
		if ((i % 5) == 0) {
			game.map.blocks[i].type = BLOCK_SHROOM;
		} else {
			game.map.blocks[i].type = BLOCK_AIR;
		}
		}*/

	ml_ivec3 player = { game.camera.cx, 0, game.camera.cz };
	for (int z = -VIEW_DISTANCE; z < VIEW_DISTANCE; ++z)
		for (int x = -VIEW_DISTANCE; x < VIEW_DISTANCE; ++x)
			gameLoadChunk(player.x + x, player.z + z);
	printf("\n");


	// tesselate column
	for (int z = -VIEW_DISTANCE; z < VIEW_DISTANCE; ++z)
		for (int x = -VIEW_DISTANCE; x < VIEW_DISTANCE; ++x)
			for (int y = 0; y < MAP_CHUNK_HEIGHT; ++y)
				gameTesselateSubChunk(player.x + x, y, player.z + z);

	gameUpdateMap();
	printf("\n* Map load complete.\n");
}

void gameFreeMap() {
}

void gameUpdateMap() {
}

void gameDrawMap(ml_frustum* frustum) {
	// for each visible chunk...
	// set up material etc. once.
	ml_material* material = game.materials + MAT_CHUNK;

	glEnable(GL_TEXTURE_2D);
	glDisable(GL_BLEND);
	glEnable(GL_DEPTH_TEST);
	glUseProgram(material->program);
	mlBindTexture2D(&blocks_texture, 0);

	ml_vec3 tlight;
	ml_matrix33 normalmat;
	mlPushMatrix(&game.modelview);
	mlGetRotationMatrix(&normalmat, mlGetMatrix(&game.modelview));
	tlight = mlMulMat33Vec(&normalmat, &game.light_dir);

	glUniform1i(material->tex0, 0);
	mlUniformMatrix(material->projmat, mlGetMatrix(&game.projection));
	mlUniformMatrix(material->modelview, mlGetMatrix(&game.modelview));
	mlUniformVec4(material->amb_color, &game.ambient_color);
	mlUniformVec4(material->fog_color, &game.fog_color);
	mlUniformVec3(material->light_dir, &tlight);
	mlUniformMatrix33(material->normalmat, &normalmat);

	// figure out which chunks are visible
	// draw visible chunks

	game_chunk* chunks = game.map.chunks;
	for (int i = 0; i < MAP_CHUNK_WIDTH*MAP_CHUNK_WIDTH; ++i) {
		// calc chunk offset for this chunk
		int x = chunks[i].x - game.camera.cx;
		int z = chunks[i].z - game.camera.cz;
		if (game.single_chunk_mode && (x != -1 || z != -1))
			continue;
		// todo: figure out which meshes need to be drawn
		for (int j = 0; j < MAP_CHUNK_HEIGHT; ++j) {
			ml_mesh* mesh = chunks[i].data + j;
			if (mesh->vbo == 0)
				continue;

			ml_vec3 offset = { (float)(x*CHUNK_SIZE) - 0.5f,
			                   (float)(j*CHUNK_SIZE) - 0.5f,
			                   (float)(z*CHUNK_SIZE) - 0.5f};

			ml_vec3 p1 = {offset.x + (float)CHUNK_SIZE,
			              offset.y + (float)CHUNK_SIZE,
			              offset.z + (float)CHUNK_SIZE };
			// not calculating the frustum right...
			if (mlTestFrustumAABB(frustum, offset, p1) == ML_OUTSIDE)
				continue;

			// update the chunk offset uniform
			// bind the VBO
			// can this be done once? probably...
			mlUniformVec3(material->chunk_offset, &offset);
			mlMapMeshToMaterial(mesh, material);
			glDrawArrays(mesh->mode, 0, mesh->count);
		}
	}

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	mlPopMatrix(&game.modelview);
	glUseProgram(0);
	glDisable(GL_TEXTURE_2D);
}

// TODO: chunk saving/loading
// TODO: asynchronous
// as a test, just fill in the designated chunk
// and tesselate the whole thing

void gameLoadChunk(int x, int z) {
	int bufx = mod(x, MAP_CHUNK_WIDTH);
	int bufz = mod(z, MAP_CHUNK_WIDTH);
	game_chunk* chunk = &(game.map.chunks[bufz*MAP_CHUNK_WIDTH + bufx]);
	game_block* blocks = game.map.blocks;
	chunk->x = x;
	chunk->z = z;

	int blockx = x * CHUNK_SIZE;
	int blockz = z * CHUNK_SIZE;

	// fill blocks in column...
	/**/

	int fillx, filly, fillz;

	/*
	for (fillz = blockz; fillz < blockz + CHUNK_SIZE; ++fillz) {
		for (fillx = blockx; fillx < blockx + CHUNK_SIZE; ++fillx) {
			for (filly = 0; filly < MAP_BLOCK_HEIGHT; ++filly) {
				if (filly < GROUND_LEVEL - 2)
					blocks[blockIndex(fillx, filly, fillz)].type = BLOCK_STONE;
				else
					blocks[blockIndex(fillx, filly, fillz)].type = BLOCK_AIR;
			}
		}
	}

	*/

#define NOISE_SCALE (1.0/(double)CHUNK_SIZE)

	printf("+");
	fflush(stdout);
/*
	for (fillz = blockz; fillz < blockz + CHUNK_SIZE; ++fillz) {
		for (fillx = blockx; fillx < blockx + CHUNK_SIZE; ++fillx) {
			double height = simplexNoise(fillx * NOISE_SCALE, fillz * NOISE_SCALE) * 0.5 + 1.0;
			for (filly = 0; filly < MAP_BLOCK_HEIGHT; ++filly) {
				double density = (((double)MAP_BLOCK_HEIGHT - (double)filly) / (double)MAP_BLOCK_HEIGHT); // gradient
				density -= simplexNoise(fillx * NOISE_SCALE,
				                      filly * NOISE_SCALE);
				density -= simplexNoise(filly * NOISE_SCALE,
				                      fillz * NOISE_SCALE);
				if (density < 0.0) {
					if (filly < 20)
						blocks[blockIndex(fillx, filly, fillz)].type = BLOCK_WATER;
					else
						blocks[blockIndex(fillx, filly, fillz)].type = BLOCK_AIR;
				} else if (((double)filly / (double)MAP_BLOCK_HEIGHT) < height) {
					if (density < 0.2) {
					blocks[blockIndex(fillx, filly, fillz)].type = BLOCK_MOSS;
					} else if (density < 0.5) {
						blocks[blockIndex(fillx, filly, fillz)].type = BLOCK_DARKSTONE;
					} else {
						blocks[blockIndex(fillx, filly, fillz)].type = BLOCK_LAVA;
					}
				} else {
					if (filly < 20)
						blocks[blockIndex(fillx, filly, fillz)].type = BLOCK_WATER;
					else
						blocks[blockIndex(fillx, filly, fillz)].type = BLOCK_AIR;
				}
			}
		}
		}*/

	for (fillz = blockz; fillz < blockz + CHUNK_SIZE; ++fillz) {
		for (fillx = blockx; fillx < blockx + CHUNK_SIZE; ++fillx) {
			for (filly = 0; filly < MAP_BLOCK_HEIGHT; ++filly) {
				if (filly == GROUND_LEVEL - 3)
					blocks[blockIndex(fillx, filly, fillz)].type = BLOCK_GRASS;
				else
					blocks[blockIndex(fillx, filly, fillz)].type = BLOCK_AIR;
			}
		}
	}
	
	for (int i = 0; i < NUM_BLOCKTYPES; ++i) {
		int x = (i*2) % CHUNK_SIZE;
		int z = ((i*2) / CHUNK_SIZE) * 2;
		blocks[blockIndex(blockx + x, GROUND_LEVEL - 1, blockz + z)].type = i;
	}

	for (int z = 2; z < 4; ++z)
		for (int x = 2; x < 4; ++x)
			for (int y = 2; y < 4; ++y)
				blocks[blockIndex(x, GROUND_LEVEL + y, z)].type = BLOCK_STONE;

	for (fillz = CHUNK_SIZE; fillz < CHUNK_SIZE*2; ++fillz) {
		for (fillx = CHUNK_SIZE; fillx < CHUNK_SIZE*2; ++fillx) {
			for (filly = GROUND_LEVEL + 2; filly < GROUND_LEVEL + 2 + CHUNK_SIZE; ++filly) {
				blocks[blockIndex(fillx, filly, fillz)].type = (rand() % 2) ? BLOCK_SAND : BLOCK_AIR;
			}
		}
	}
	
}

// tesselation buffer: size is maximum number of triangles generated
//   1: fill tesselation buffer
//  2: allocate mesh
//   3: fill vertices
//   returns num verts in chunk

#define TESSELATION_BUFFER_SIZE (CHUNK_SIZE*CHUNK_SIZE*CHUNK_SIZE*36)
static game_block_vtx tesselation_buffer[TESSELATION_BUFFER_SIZE];
#define POS(x, y, z) mapPackVectorChunkCoord((unsigned int)(x), (unsigned int)(y), (unsigned int)(z), 0)

size_t gameTesselateSubChunk(int cx, int cy, int cz) {
	ml_mesh* mesh;
	int ix, iy, iz;
	int bx, by, bz;
	size_t vi;
	game_block_vtx* verts;

	verts = tesselation_buffer;
	vi = 0;
	cx = mod(cx, MAP_CHUNK_WIDTH);
	cz = mod(cz, MAP_CHUNK_WIDTH);
	mesh = game.map.chunks[cz*MAP_CHUNK_WIDTH + cx].data + cy;
	bx = cx*CHUNK_SIZE;
	by = cy*CHUNK_SIZE;
	bz = cz*CHUNK_SIZE;
	printf(".");
	fflush(stdout);
	size_t nprocessed = 0;

	// fill in verts
	for (iz = 0; iz < CHUNK_SIZE; ++iz) {
		for (ix = 0; ix < CHUNK_SIZE; ++ix) {
			for (iy = 0; iy < CHUNK_SIZE; ++iy) {
				uint8_t t = blockType(bx+ix, by+iy, bz+iz);
				if (t == BLOCK_AIR) {
					++nprocessed;
					continue;
				}

				if (blockType(bx+ix, by+iy+1, bz+iz) == BLOCK_AIR && blockinfo[t].tex[BLOCK_TEX_TOP] != 0) {
					tc2us_t* tc = &BLOCKTC(t, BLOCK_TEX_TOP, 0);
					game_block_vtx corners[4] = {
						{POS(ix, iy+1, iz), {0, INT8_MAX, 0, 0}, tc[0], 0xffffffff },
						{POS(ix, iy+1, iz+1), {0, INT8_MAX, 0, 0}, tc[1], 0xffffffff },
						{POS(ix+1, iy+1, iz+1), {0, INT8_MAX, 0, 0}, tc[3], 0xffffffff },
						{POS(ix+1, iy+1, iz), {0, INT8_MAX, 0, 0}, tc[2], 0xffffffff },
					};
					verts[vi++] = corners[0];
					verts[vi++] = corners[1];
					verts[vi++] = corners[2];
					verts[vi++] = corners[0];
					verts[vi++] = corners[2];
					verts[vi++] = corners[3];
				}

				if (blockType(bx+ix, by+iy-1, bz+iz) == BLOCK_AIR && blockinfo[t].tex[BLOCK_TEX_BOTTOM] != 0) {
					tc2us_t* tc = &BLOCKTC(t, BLOCK_TEX_BOTTOM, 0);
					game_block_vtx corners[4] = {
						{POS(ix, iy, iz), {0, INT8_MIN, 0, 0}, tc[0], 0xffffffff },
						{POS(ix, iy, iz+1), {0, INT8_MIN, 0, 0}, tc[1], 0xffffffff },
						{POS(ix+1, iy, iz+1), {0, INT8_MIN, 0, 0}, tc[3], 0xffffffff },
						{POS(ix+1, iy, iz), {0, INT8_MIN, 0, 0}, tc[2], 0xffffffff },
					};
					verts[vi++] = corners[0];
					verts[vi++] = corners[2];
					verts[vi++] = corners[1];
					verts[vi++] = corners[2];
					verts[vi++] = corners[0];
					verts[vi++] = corners[3];
				}
				if (blockType(bx+ix-1, by+iy, bx+iz) == BLOCK_AIR && blockinfo[t].tex[BLOCK_TEX_LEFT] != 0) {
					tc2us_t* tc = &BLOCKTC(t, BLOCK_TEX_LEFT, 0);
					game_block_vtx corners[4] = {
						{POS(ix, iy, iz), {INT8_MIN, 0, 0, 0}, tc[1], 0xffffffff },
						{POS(ix, iy, iz+1), {INT8_MIN, 0, 0, 0}, tc[3], 0xffffffff },
						{POS(ix, iy+1, iz), {INT8_MIN, 0, 0, 0}, tc[0], 0xffffffff },
						{POS(ix, iy+1, iz+1), {INT8_MIN, 0, 0, 0}, tc[2], 0xfffffff },
					};
					verts[vi++] = corners[0];
					verts[vi++] = corners[1];
					verts[vi++] = corners[2];
					verts[vi++] = corners[2];
					verts[vi++] = corners[1];
					verts[vi++] = corners[3];
				}
				if (blockType(bx+ix+1, by+iy, bz+iz) == BLOCK_AIR && blockinfo[t].tex[BLOCK_TEX_RIGHT] != 0) {
					tc2us_t* tc = &BLOCKTC(t, BLOCK_TEX_RIGHT, 0);
					game_block_vtx corners[4] = {
						{POS(ix+1, iy, iz), {INT8_MAX, 0, 0, 0}, tc[1], 0xffffffff },
						{POS(ix+1, iy, iz+1), {INT8_MAX, 0, 0, 0}, tc[3], 0xffffffff },
						{POS(ix+1, iy+1, iz+1), {INT8_MAX, 0, 0, 0}, tc[2], 0xffffffff },
						{POS(ix+1, iy+1, iz), {INT8_MAX, 0, 0, 0}, tc[0], 0xffffffff },
					};
					verts[vi++] = corners[0];
					verts[vi++] = corners[2];
					verts[vi++] = corners[1];
					verts[vi++] = corners[2];
					verts[vi++] = corners[0];
					verts[vi++] = corners[3];
				}
				if (blockType(bx+ix, by+iy, bz+iz+1) == BLOCK_AIR && blockinfo[t].tex[BLOCK_TEX_FRONT] != 0) {
					tc2us_t* tc = &BLOCKTC(t, BLOCK_TEX_FRONT, 0);
					game_block_vtx corners[4] = {
						{POS(ix, iy, iz+1), {0, 0, INT8_MAX, 0}, tc[1], 0xffffffff },
						{POS(ix+1, iy, iz+1), {0, 0, INT8_MAX, 0}, tc[3], 0xffffffff },
						{POS(ix+1, iy+1, iz+1), {0, 0, INT8_MAX, 0}, tc[2], 0xffffffff },
						{POS(ix, iy+1, iz+1), {0, 0, INT8_MAX, 0}, tc[0], 0xffffffff },
					};
					verts[vi++] = corners[0];
					verts[vi++] = corners[1];
					verts[vi++] = corners[2];
					verts[vi++] = corners[0];
					verts[vi++] = corners[2];
					verts[vi++] = corners[3];
				}
				if (blockType(bx+ix, by+iy, bz+iz-1) == BLOCK_AIR && blockinfo[t].tex[BLOCK_TEX_BACK] != 0) {
					tc2us_t* tc = &BLOCKTC(t, BLOCK_TEX_BACK, 0);
					game_block_vtx corners[4] = {
						{POS(ix, iy, iz), {0, 0, INT8_MIN, 0}, tc[1], 0xffffffff },
						{POS((ix+1), iy, iz), {0, 0, INT8_MIN, 0}, tc[3], 0xffffffff },
						{POS((ix+1), (iy+1), iz), {0, 0, INT8_MIN, 0}, tc[2], 0xffffffff },
						{POS(ix, (iy+1), iz), {0, 0, INT8_MIN, 0}, tc[0], 0xffffffff },
					};
					verts[vi++] = corners[0];
					verts[vi++] = corners[2];
					verts[vi++] = corners[1];
					verts[vi++] = corners[2];
					verts[vi++] = corners[0];
					verts[vi++] = corners[3];
				}
				++nprocessed;

				if (vi > TESSELATION_BUFFER_SIZE)
					roamError("Tesselation buffer too small for chunk (%d, %d, %d): %zu verts, %zu blocks processed", cx, cy, cz, vi, nprocessed);
			}
		}
	}

	if (vi == 0)
		return vi;

	mlCreateMesh(mesh, vi, verts, ML_POS_10_2 | ML_N_4B | ML_TC_2US | ML_CLR_4UB);
	return vi;
}

