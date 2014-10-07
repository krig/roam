#include "common.h"
#include "math3d.h"
#include "game.h"
#include "voxelmap.h"
#include "noise.h"
#include <time.h>

static inline size_t blockIndex(int x, int y, int z) {
	int bx = mod(x, MAP_BLOCK_WIDTH);
	int bz = mod(z, MAP_BLOCK_WIDTH);
	return bz * MAP_BLOCK_WIDTH * MAP_BLOCK_WIDTH + bx * MAP_BLOCK_WIDTH + y;
}

static inline uint8_t blockType(int x, int y, int z) {
	if (y < 0)
		return BLOCK_DARKSTONE;
	if (y >= MAP_BLOCK_HEIGHT)
		return BLOCK_AIR;
	return game.map.blocks[blockIndex(x, y, z)].type;
}

extern ml_tex2d blocks_texture;

void gameLoadChunk(int x, int z);

void gameInitMap() {
	printf("* Allocate and build initial map...\n");
	memset(&game.map, 0, sizeof(game_map));
	game.map.seed = time(0);
	printf("* Seed: %lx\n", game.map.seed);

	ml_ivec3 player = { 0, 0, 0 };
	for (int z = -VIEW_DISTANCE; z < VIEW_DISTANCE; ++z)
		for (int x = -VIEW_DISTANCE; x < VIEW_DISTANCE; ++x)
			gameLoadChunk(player.x + x, player.z + z);
	gameUpdateMap();
	printf("* Map load complete.\n");
}

void gameFreeMap() {
}

void gameUpdateMap() {
}

void gameDrawMap() {
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
		ml_vec3 offset = { (float)x - game.camera.offset.x,
		                   -game.camera.offset.y,
		                   (float)z - game.camera.offset.z };
		// skip chunk if outside view distance (not loaded/generated then)
		if (abs(x) > VIEW_DISTANCE || abs(z) > VIEW_DISTANCE)
			continue;
		// todo: figure out which meshes need to be drawn
		for (int j = 0; j < MAP_CHUNK_HEIGHT; ++j) {
			ml_mesh* mesh = chunks[i].data + j;
			if (mesh->vbo != 0) {
				// update the chunk offset uniform
				// bind the VBO
				// can this be done once? probably...
				mlUniformVec3(material->chunk_offset, &offset);
				mlMapMeshToMaterial(mesh, material);
				glDrawArrays(mesh->mode, 0, mesh->count);
				glBindBuffer(GL_ARRAY_BUFFER, 0);
			}
		}
	}

	mlPopMatrix(&game.modelview);
	glUseProgram(0);
	glDisable(GL_TEXTURE_2D);

	/*
	ml_vec3 tlight;
	ml_matrix33 normalmat;
	mlPushMatrix(&game.modelview);
	mlGetRotationMatrix(&normalmat, mlGetMatrix(&game.modelview));
	tlight = mlMulMat33Vec(&normalmat, &light_dir);
	mlDrawBegin(&renderables[2]);
	mlUniformMatrix(renderables[2].material->projmat, mlGetMatrix(&game.projection));
	mlUniformMatrix(renderables[2].material->modelview, mlGetMatrix(&game.modelview));
	mlGetRotationMatrix(&normalmat, mlGetMatrix(&game.modelview));
	mlUniformMatrix33(renderables[2].material->normalmat, &normalmat);
	ml_vec3 chunk_offset = { -1, -1, -1 };
	mlUniformVec3(renderables[2].material->chunk_offset, &chunk_offset);
	mlUniformVec4(renderables[2].material->amb_color, &amb_color);
	mlUniformVec4(renderables[2].material->fog_color, &fog_color);
	mlUniformVec3(renderables[2].material->light_dir, &tlight);
	mlDrawEnd(&renderables[2]);
	mlPopMatrix(&game.modelview);
	*/
}

// TODO: chunk saving/loading
// TODO: asynchronous
// as a test, just fill in the designated chunk
// and tesselate the whole thing

size_t gameTesselateSubChunk(ml_mesh* mesh, int cx, int cy, int cz);

void gameLoadChunk(int x, int z) {
	int bufx = mod(x, MAP_CHUNK_WIDTH);
	int bufz = mod(z, MAP_CHUNK_WIDTH);
	printf("Loading (%d,%d) into (%d, %d)\n", x, z, bufx, bufz);
	game_chunk* chunk = &(game.map.chunks[bufz*MAP_CHUNK_WIDTH + bufx]);
	game_block* blocks = game.map.blocks;
	chunk->x = x;
	chunk->z = z;

	int blockx = x * CHUNK_SIZE;
	int blockz = z * CHUNK_SIZE;
	if (blocks[blockIndex(blockx, 0, blockz)].type != 0)
		printf("Cache contention! (%d, %d) with (%d, %d)\n",
		       bufx,
		       bufz,
		       blocks[blockIndex(blockx, 0, blockz)].type,
		       blocks[blockIndex(blockx, 0, blockz)].meta);
	blocks[blockIndex(blockx, 0, blockz)].type = x;
	blocks[blockIndex(blockx, 0, blockz)].meta = z;

	// fill blocks in column...
	for (int fillz = blockz; fillz < blockz + CHUNK_SIZE; ++fillz) {
		for (int fillx = blockx; fillx < blockx + CHUNK_SIZE; ++fillx) {
			for (int filly = 0; filly < MAP_BLOCK_HEIGHT; ++filly) {
				if (filly < 5)
					blocks[blockIndex(fillx, filly, fillz)].type = BLOCK_DARKSTONE;
				else if (filly < GROUND_LEVEL - 5)
					blocks[blockIndex(fillx, filly, fillz)].type = BLOCK_STONE;
				else if (filly < GROUND_LEVEL)
					blocks[blockIndex(fillx, filly, fillz)].type = BLOCK_DIRT;
				else if (filly == GROUND_LEVEL)
					blocks[blockIndex(fillx, filly, fillz)].type = BLOCK_GRASS;
				else
					blocks[blockIndex(fillx, filly, fillz)].type = BLOCK_AIR;
			}
		}
	}

	// tesselate column
	for (int y = 0; y < MAP_CHUNK_HEIGHT; ++y)
		gameTesselateSubChunk(chunk->data + y, bufx, y, bufz);
}

// tesselation buffer: size is maximum number of triangles generated
//   1: fill tesselation buffer
//  2: allocate mesh
//   3: fill vertices
//   returns num verts in chunk

enum {
	FACE_LEFT = 1, FACE_RIGHT = 2,
	FACE_BOTTOM = 4, FACE_TOP = 8,
	FACE_BACK = 16, FACE_FRONT = 32
};

#define TYPE_AT(x, y, z) type[(z)*CHUNK_SIZE*CHUNK_SIZE + (x)*CHUNK_SIZE + (y)]
#define FACE_AT(x, y, z) faces[(z)*CHUNK_SIZE*CHUNK_SIZE + (x)*CHUNK_SIZE + (y)]

#define TESSELATION_BUFFER_SIZE (4*1024*1024)
static uint8_t tesselation_buffer[TESSELATION_BUFFER_SIZE];

extern struct blockinfo_t blockinfo[];

static inline tc2us_t tc2us(ml_vec2 tc) {
	tc2us_t to = {
		(uint16_t)(tc.x * (double)0xffff),
		(uint16_t)(tc.y * (double)0xffff)
	};
	return to;
}

size_t gameTesselateSubChunk(ml_mesh* mesh, int cx, int cy, int cz) {
	printf("tesselate subchunk (%d, %d, %d)\n", cx, cy, cz);

	int ix, iy, iz, vi;
	size_t nverts;
	game_block_vtx* verts;
	uint8_t type[CHUNK_SIZE*CHUNK_SIZE*CHUNK_SIZE];
	uint8_t faces[CHUNK_SIZE*CHUNK_SIZE*CHUNK_SIZE];
	memset(type, 0, sizeof(type));
	memset(faces, 0, sizeof(faces));
	nverts = 0;
	int bx, by, bz;
	bx = cx*CHUNK_SIZE;
	by = cy*CHUNK_SIZE;
	bz = cz*CHUNK_SIZE;
	for (iz = 0; iz < CHUNK_SIZE; ++iz) {
		for (ix = 0; ix < CHUNK_SIZE; ++ix) {
			for (iy = 0; iy < CHUNK_SIZE; ++iy) {
				TYPE_AT(ix, iy, iz) = blockType(bx+ix, by+iy, bz+iz);
				if (TYPE_AT(ix, iy, iz) != BLOCK_AIR) {
					if (blockType(bx+ix-1, by+iy, bx+iz) == BLOCK_AIR) {
						FACE_AT(ix, iy, iz) |= FACE_LEFT;
						nverts += 6;
					}
					if (blockType(bx+ix-1, by+iy, bz+iz) == BLOCK_AIR) {
						FACE_AT(ix, iy, iz) |= FACE_RIGHT;
						nverts += 6;
					}
					if (blockType(bx+ix, by+iy-1, bz+iz) == BLOCK_AIR) {
						FACE_AT(ix, iy, iz) |= FACE_BOTTOM;
						nverts += 6;
					}
					if (blockType(bx+ix, by+iy+1, bz+iz) == BLOCK_AIR) {
						FACE_AT(ix, iy, iz) |= FACE_TOP;
						nverts += 6;
					}
					if (blockType(bx+ix, by+iy, bz+iz-1) == BLOCK_AIR) {
						FACE_AT(ix, iy, iz) |= FACE_BACK;
						nverts += 6;
					}
					if (blockType(bx+ix, by+iy, bz+iz+1) == BLOCK_AIR) {
						FACE_AT(ix, iy, iz) |= FACE_FRONT;
						nverts += 6;
					}
				}
			}
		}
	}

	if (nverts == 0)
		return nverts;

	vi = 0;

	if (nverts * sizeof(game_block_vtx) > TESSELATION_BUFFER_SIZE)
		roamError("Tesselation buffer too small: %zu", nverts * sizeof(game_block_vtx));

	verts = (game_block_vtx*)tesselation_buffer;//malloc(nverts * sizeof(game_block_vtx));

#define POS(x, y, z) mlPackVector_U_10_10_10_2((float)(x)/(float)CHUNK_SIZE, (float)(y)/(float)CHUNK_SIZE, (float)(z)/(float)CHUNK_SIZE, 0.f)

	// fill in verts
	for (iz = 0; iz < CHUNK_SIZE; ++iz) {
		for (iy = 0; iy < CHUNK_SIZE; ++iy) {
			for (ix = 0; ix < CHUNK_SIZE; ++ix) {
				uint8_t t = TYPE_AT(ix, iy, iz);
				ml_vec2 tc[6] = {
					idx2tc(blockinfo[t].tex[0]),
					idx2tc(blockinfo[t].tex[1]),
					idx2tc(blockinfo[t].tex[2]),
					idx2tc(blockinfo[t].tex[3]),
					idx2tc(blockinfo[t].tex[4]),
					idx2tc(blockinfo[t].tex[5]),
				};
				ml_vec2 tcoffs[4] = {
					{0, 0},
					{BLOCK_TC_W, 0},
					{0, BLOCK_TC_W},
					{BLOCK_TC_W, BLOCK_TC_W}
				};
				if (FACE_AT(ix, iy, iz) & FACE_LEFT) {
					game_block_vtx corners[4] = {
						{POS(ix, iy, iz), {-127, 0, 0, 0}, tc2us(mlVec2Add(tc[2], tcoffs[0])), 0xffffffff },
						{POS(ix, iy, (iz+1)), {-127, 0, 0, 0}, tc2us(mlVec2Add(tc[2], tcoffs[1])), 0xffffffff },
						{POS(ix, (iy+1), iz), {-127, 0, 0, 0}, tc2us(mlVec2Add(tc[2], tcoffs[2])), 0xffffffff },
						{POS(ix, (iy+1), (iz+1)), {-127, 0, 0, 0}, tc2us(mlVec2Add(tc[2], tcoffs[3])), 0xfffffff },
					};
					verts[vi + 0] = corners[0];
					verts[vi + 1] = corners[1];
					verts[vi + 2] = corners[2];
					verts[vi + 3] = corners[2];
					verts[vi + 4] = corners[1];
					verts[vi + 5] = corners[3];
					vi += 6;
				}

				if (FACE_AT(ix, iy, iz) & FACE_RIGHT) {
					game_block_vtx corners[4] = {
						{POS((ix+1), iy, iz), {127, 0, 0, 0}, tc2us(mlVec2Add(tc[3], tcoffs[0])), 0xffffffff },
						{POS((ix+1), iy, (iz+1)), {127, 0, 0, 0}, tc2us(mlVec2Add(tc[3], tcoffs[1])), 0xffffffff },
						{POS((ix+1), (iy+1), (iz+1)), {127, 0, 0, 0}, tc2us(mlVec2Add(tc[3], tcoffs[2])), 0xffffffff },
						{POS((ix+1), (iy+1), iz), {127, 0, 0, 0}, tc2us(mlVec2Add(tc[3], tcoffs[3])), 0xffffffff },
					};
					verts[vi + 0] = corners[0];
					verts[vi + 1] = corners[2];
					verts[vi + 2] = corners[1];
					verts[vi + 3] = corners[2];
					verts[vi + 4] = corners[0];
					verts[vi + 5] = corners[3];
					vi += 6;
				}
				if (FACE_AT(ix, iy, iz) & FACE_TOP) {
					game_block_vtx corners[4] = {
						{POS(ix, (iy+1), iz), {0, 127, 0, 0}, tc2us(mlVec2Add(tc[0], tcoffs[0])), 0xffffffff },
						{POS(ix, (iy+1), (iz+1)), {0, 127, 0, 0}, tc2us(mlVec2Add(tc[0], tcoffs[1])), 0xffffffff },
						{POS((ix+1), (iy+1), (iz+1)), {0, 127, 0, 0}, tc2us(mlVec2Add(tc[0], tcoffs[2])), 0xffffffff },
						{POS((ix+1), (iy+1), iz), {0, 127, 0, 0}, tc2us(mlVec2Add(tc[0], tcoffs[3])), 0xffffffff },
					};
					verts[vi + 0] = corners[0];
					verts[vi + 1] = corners[1];
					verts[vi + 2] = corners[2];
					verts[vi + 3] = corners[0];
					verts[vi + 4] = corners[2];
					verts[vi + 5] = corners[3];
					vi += 6;
				}

				if (FACE_AT(ix, iy, iz) & FACE_BOTTOM) {
					game_block_vtx corners[4] = {
						{POS(ix, iy, iz), {0, -127, 0, 0}, tc2us(mlVec2Add(tc[1], tcoffs[0])), 0xffffffff },
						{POS(ix, iy, (iz+1)), {0, -127, 0, 0}, tc2us(mlVec2Add(tc[1], tcoffs[1])), 0xffffffff },
						{POS((ix+1), iy, (iz+1)), {0, -127, 0, 0}, tc2us(mlVec2Add(tc[1], tcoffs[2])), 0xffffffff },
						{POS((ix+1), iy, iz), {0, -127, 0, 0}, tc2us(mlVec2Add(tc[1], tcoffs[3])), 0xffffffff },
					};
					verts[vi + 0] = corners[0];
					verts[vi + 1] = corners[2];
					verts[vi + 2] = corners[1];
					verts[vi + 3] = corners[2];
					verts[vi + 4] = corners[0];
					verts[vi + 5] = corners[3];
					vi += 6;
				}
				if (FACE_AT(ix, iy, iz) & FACE_FRONT) {
					game_block_vtx corners[4] = {
						{POS(ix, iy, (iz+1)), {127, 127, 255, 0}, tc2us(mlVec2Add(tc[4], tcoffs[0])), 0xffffffff },
						{POS((ix+1), iy, (iz+1)), {127, 127, 255, 0}, tc2us(mlVec2Add(tc[4], tcoffs[1])), 0xffffffff },
						{POS((ix+1), (iy+1), (iz+1)), {127, 127, 255, 0}, tc2us(mlVec2Add(tc[4], tcoffs[2])), 0xffffffff },
						{POS(ix, (iy+1), (iz+1)), {127, 127, 255, 0}, tc2us(mlVec2Add(tc[4], tcoffs[3])), 0xffffffff },
					};
					verts[vi + 0] = corners[0];
					verts[vi + 1] = corners[1];
					verts[vi + 2] = corners[2];
					verts[vi + 3] = corners[0];
					verts[vi + 4] = corners[2];
					verts[vi + 5] = corners[3];
					vi += 6;
				}
				if (FACE_AT(ix, iy, iz) & FACE_BACK) {
					game_block_vtx corners[4] = {
						{POS(ix, iy, iz), {0, 0, -127, 0}, tc2us(mlVec2Add(tc[5], tcoffs[0])), 0xffffffff },
						{POS((ix+1), iy, iz), {0, 0, -127, 0}, tc2us(mlVec2Add(tc[5], tcoffs[1])), 0xffffffff },
						{POS((ix+1), (iy+1), iz), {0, 0, -127, 0}, tc2us(mlVec2Add(tc[5], tcoffs[2])), 0xffffffff },
						{POS(ix, (iy+1), iz), {0, 0, -127, 0}, tc2us(mlVec2Add(tc[5], tcoffs[3])), 0xffffffff },
					};
					verts[vi + 0] = corners[0];
					verts[vi + 1] = corners[2];
					verts[vi + 2] = corners[1];
					verts[vi + 3] = corners[2];
					verts[vi + 4] = corners[0];
					verts[vi + 5] = corners[3];
					vi += 6;
				}
			}
		}
	}

	mlCreateMesh(mesh, vi, verts, ML_POS_10_2 | ML_N_4B | ML_TC_2US | ML_CLR_4UB);

	printf("Tesselated, %zu verts\n", nverts);
	//free(verts);
	return nverts;
}

