#include "common.h"
#include "math3d.h"
#include "game.h"
#include "voxelmap.h"
#include "noise.h"
#include <time.h>

//#define CHUNK_SCALE (1.0/16.0)
//#define CHUNK_AT(c,x,y,z) ((c)->data[(z)*256 + (y)*16 + (x)])
//#define GetBlock(x,y,z) (game.map->blocks[(y)*() + (z)*() + (x)])

extern ml_tex2d blocks_texture;

void gameLoadChunk(int x, int z);

void gameInitMap() {
	printf("* Allocate and build initial map...\n");
	game.map = calloc(1, sizeof(game_map));
	game.map->seed = time(0);
	printf("* Seed: %lx\n", game.map->seed);

	// for now, always start at (0, 0)
	for (int z = -VIEW_DISTANCE; z < VIEW_DISTANCE; ++z)
		for (int x = -VIEW_DISTANCE; x < VIEW_DISTANCE; ++x)
			gameLoadChunk(x, z);
	gameUpdateMap();
	printf("* Map load complete.\n");
}

void gameFreeMap() {
	free(game.map);
	game.map = NULL;
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
	game_chunk* chunks = game.map->chunks;
	for (int i = 0; i < MAP_CHUNK_WIDTH*MAP_CHUNK_WIDTH; ++i) {
		// calc chunk offset for this chunk
		int x = chunks[i].x - game.camera.chunk[0];
		int z = chunks[i].z - game.camera.chunk[2];
		ml_vec3 offset = { (float)x - game.camera.offset.x,
		                   -game.camera.offset.y,
		                   (float)z - game.camera.offset.z };
		// skip chunk if outside view distance (not loaded/generated then)
		if (abs(x) >= VIEW_DISTANCE || abs(z) >= VIEW_DISTANCE)
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

size_t gameTesselateSubChunk(ml_mesh* mesh, int x, int z);


void gameLoadChunk(int x, int z) {
	game_chunk* chunk = &(game.map->chunks[z*MAP_CHUNK_WIDTH + x]);

	// fill blocks in column...

	// tesselate column
	for (int y = 0; y < MAP_CHUNK_HEIGHT; ++y)
		gameTesselateSubChunk(chunk->data + y, x, z);
}

size_t gameTesselateSubChunk(ml_mesh* mesh, int x, int z) {
	// if all one material.. if air, skip; if solid, make a big cube
	return 0;
}


/*
static uint8_t gameGenerateBlock(double x, double y, double z) {
	double height = simplexNoise(x, z);
	double density = osnNoise(x, y, z);
	return (height - density) < 0 ? BLOCK_AIR : BLOCK_STONE;
}

// just a test...
void gameGenerateChunk(game_chunk* chunk, int x, int y, int z) {
	int ix, iy, iz;
	double dx, dy, dz;
	dx = (double)x;
	dy = (double)y;
	dz = (double)z;
	for (iz = 0; iz < 16; ++iz) {
		double ddz = dz + CHUNK_SCALE * (double)iz;
		for (iy = 0; iy < 16; ++iy) {
			double ddy = dy + CHUNK_SCALE * (double)iy;
			for (ix = 0; ix < 16; ++ix) {
				double ddx = dx + CHUNK_SCALE*(double)ix;
				CHUNK_AT(chunk, ix, iy, iz) = gameGenerateBlock(ddx, ddy, ddz);
			}
		}
	}

	chunk->x = x;
	chunk->y = y;
	chunk->z = z;
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

#define FACE_AT(x, y, z) faces[(z)*256 + (y)* 16 + (x)]

size_t gameTesselateChunk(ml_mesh* mesh, game_chunk* chunk) {
	int ix, iy, iz, vi;
	size_t nverts;
	game_block_vtx* verts;
	uint8_t faces[16*16*16];
	memset(faces, 0, sizeof(faces));
	nverts = 0;
	for (iz = 0; iz < 16; ++iz) {
		for (iy = 0; iy < 16; ++iy) {
			for (ix = 0; ix < 16; ++ix) {
				if (CHUNK_AT(chunk, ix, iy, iz) != BLOCK_AIR) {
					if (ix == 0 || CHUNK_AT(chunk, ix-1, iy, iz) == BLOCK_AIR) {
						FACE_AT(ix, iy, iz) |= FACE_LEFT;
						nverts += 6;
					}
					if (ix == 15 || CHUNK_AT(chunk, ix+1, iy, iz) == BLOCK_AIR) {
						FACE_AT(ix, iy, iz) |= FACE_RIGHT;
						nverts += 6;
					}
					if (iy == 0 || CHUNK_AT(chunk, ix, iy-1, iz) == BLOCK_AIR) {
						FACE_AT(ix, iy, iz) |= FACE_BOTTOM;
						nverts += 6;
					}
					if (iy == 15 || CHUNK_AT(chunk, ix, iy+1, iz) == BLOCK_AIR) {
						FACE_AT(ix, iy, iz) |= FACE_TOP;
						nverts += 6;
					}
					if (iz == 0 || CHUNK_AT(chunk, ix, iy, iz-1) == BLOCK_AIR) {
						FACE_AT(ix, iy, iz) |= FACE_BACK;
						nverts += 6;
					}
					if (iz == 15 || CHUNK_AT(chunk, ix, iy, iz+1) == BLOCK_AIR) {
						FACE_AT(ix, iy, iz) |= FACE_FRONT;
						nverts += 6;
					}
				}
			}
		}
	}

	//memset(faces, 0xff, sizeof(faces));
	//nverts = 16*16*16*12;

	if (nverts == 0)
		return nverts;

	vi = 0;

	verts = malloc(nverts * sizeof(game_block_vtx));

	// fill in verts
	for (iz = 0; iz < 16; ++iz) {
		for (iy = 0; iy < 16; ++iy) {
			for (ix = 0; ix < 16; ++ix) {
				if (FACE_AT(ix, iy, iz) & FACE_LEFT) {
					game_block_vtx corners[4] = {
						{{ix*15, iy*15, iz*15, CHUNK_AT(chunk, ix, iy, iz)}, {-127, 0, 0, 0} },
						{{ix*15, iy*15, (iz+1)*15, CHUNK_AT(chunk, ix, iy, iz)}, {-127, 0, 0, 0} },
						{{ix*15, (iy+1)*15, iz*15, CHUNK_AT(chunk, ix, iy, iz)}, {-127, 0, 0, 0} },
						{{ix*15, (iy+1)*15, (iz+1)*15, CHUNK_AT(chunk, ix, iy, iz)}, {-127, 0, 0, 0} },
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
						{{(ix+1)*15, iy*15, iz*15, CHUNK_AT(chunk, ix, iy, iz)}, {127, 0, 0, 0} },
						{{(ix+1)*15, iy*15, (iz+1)*15, CHUNK_AT(chunk, ix, iy, iz)}, {127, 0, 0, 0} },
						{{(ix+1)*15, (iy+1)*15, (iz+1)*15, CHUNK_AT(chunk, ix, iy, iz)}, {127, 0, 0, 0} },
						{{(ix+1)*15, (iy+1)*15, iz*15, CHUNK_AT(chunk, ix, iy, iz)}, {127, 0, 0, 0} },
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
						{{ix*15, (iy+1)*15, iz*15, CHUNK_AT(chunk, ix, iy, iz)}, {0, 127, 0, 0} },
						{{ix*15, (iy+1)*15, (iz+1)*15, CHUNK_AT(chunk, ix, iy, iz)}, {0, 127, 0, 0} },
						{{(ix+1)*15, (iy+1)*15, (iz+1)*15, CHUNK_AT(chunk, ix, iy, iz)}, {0, 127, 0, 0} },
						{{(ix+1)*15, (iy+1)*15, iz*15, CHUNK_AT(chunk, ix, iy, iz)}, {0, 127, 0, 0} },
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
						{{ix*15, iy*15, iz*15, CHUNK_AT(chunk, ix, iy, iz)}, {0, -127, 0, 0} },
						{{ix*15, iy*15, (iz+1)*15, CHUNK_AT(chunk, ix, iy, iz)}, {0, -127, 0, 0} },
						{{(ix+1)*15, iy*15, (iz+1)*15, CHUNK_AT(chunk, ix, iy, iz)}, {0, -127, 0, 0} },
						{{(ix+1)*15, iy*15, iz*15, CHUNK_AT(chunk, ix, iy, iz)}, {0, -127, 0, 0} },
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
						{{ix*15, iy*15, (iz+1)*15, CHUNK_AT(chunk, ix, iy, iz)}, {127, 127, 255, 0} },
						{{(ix+1)*15, iy*15, (iz+1)*15, CHUNK_AT(chunk, ix, iy, iz)}, {127, 127, 255, 0} },
						{{(ix+1)*15, (iy+1)*15, (iz+1)*15, CHUNK_AT(chunk, ix, iy, iz)}, {127, 127, 255, 0} },
						{{ix*15, (iy+1)*15, (iz+1)*15, CHUNK_AT(chunk, ix, iy, iz)}, {127, 127, 255, 0} },
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
						{{ix*15, iy*15, iz*15, CHUNK_AT(chunk, ix, iy, iz)}, {0, 0, -127, 0} },
						{{(ix+1)*15, iy*15, iz*15, CHUNK_AT(chunk, ix, iy, iz)}, {0, 0, -127, 0} },
						{{(ix+1)*15, (iy+1)*15, iz*15, CHUNK_AT(chunk, ix, iy, iz)}, {0, 0, -127, 0} },
						{{ix*15, (iy+1)*15, iz*15, CHUNK_AT(chunk, ix, iy, iz)}, {0, 0, -127, 0} },
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

	mlCreateMesh(mesh, vi, verts, ML_POS_4UB | ML_N_4B);
	free(verts);
	return nverts;
}

*/

