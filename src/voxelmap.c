#include "common.h"
#include "math3d.h"
#include "voxelmap.h"
#include "noise.h"

/*
  Theory for how to render:
  Each chunk renders as chunk-relative coordinates + an offset from the players chunk
  Render all chunks as instances where the chunk offset is passed per instance
 */

#define CHUNK_SCALE (1.0/16.0)
#define CHUNK_AT(c,x,y,z) ((c)->data[(z)*256 + (y)*16 + (x)])

static uint8_t gameGenerateBlock(double x, double y, double z) {
	double density = osnNoise(x, y, z);
	return density < 0 ? BLOCK_AIR : BLOCK_STONE;
}

// just a test...
void gameGenerateChunk(game_chunk* chunk, int x, int y, int z) {
	int ix, iy, iz;
	double dx, dy, dz;
	dx = (double)x;
	dy = (double)y;
	dz = (double)z;
	uint8_t data[16 * 16 * 16];
	for (iz = 0; iz < 16; ++iz) {
		double ddz = dz + CHUNK_SCALE * (double)iz;
		for (iy = 0; iy < 16; ++iy) {
			double ddy = dz + CHUNK_SCALE * (double)iy;
			for (ix = 0; ix < 16; ++ix) {
				double ddx = dx + CHUNK_SCALE*(double)ix;
				CHUNK_AT(chunk, ix, iy, iz) = gameGenerateBlock(ddx, ddy, ddz);
			}
		}
	}

	chunk->x = x;
	chunk->y = y;
	chunk->z = z;
	memcpy(chunk->data, data, 16 * 16 * 16);
}

/* tesselation buffer: size is maximum number of triangles generated
   1: fill tesselation buffer
   2: allocate mesh
   3: fill vertices

   returns num verts in chunk
*/

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
	memset(faces, 0, 16*16*16);
	nverts = 0;
	for (iz = 0; iz < 16; ++iz) {
		for (iy = 0; iy < 16; ++iy) {
			for (ix = 0; ix < 16; ++ix) {
				if (CHUNK_AT(chunk, ix, iy, iz) != BLOCK_AIR) {
					printf("(%d, %d, %d) != AIR\n", ix, iy, iz);
					if (ix == 0 || CHUNK_AT(chunk, ix-1, iy, iz) == BLOCK_AIR) {
						FACE_AT(ix, iy, iz) += FACE_LEFT;
						nverts += 6;
					}
					if (ix == 15 || CHUNK_AT(chunk, ix+1, iy, iz) == BLOCK_AIR) {
						FACE_AT(ix, iy, iz) += FACE_RIGHT;
						nverts += 6;
					}
					if (iy == 0 || CHUNK_AT(chunk, ix, iy-1, iz) == BLOCK_AIR) {
						FACE_AT(ix, iy, iz) += FACE_BOTTOM;
						nverts += 6;
					}
					if (iy == 15 || CHUNK_AT(chunk, ix, iy+1, iz) == BLOCK_AIR) {
						FACE_AT(ix, iy, iz) += FACE_TOP;
						nverts += 6;
					}
					if (iz == 0 || CHUNK_AT(chunk, ix, iy, iz-1) == BLOCK_AIR) {
						FACE_AT(ix, iy, iz) += FACE_BACK;
						nverts += 6;
					}
					if (iz == 15 || CHUNK_AT(chunk, ix, iy, iz+1) == BLOCK_AIR) {
						FACE_AT(ix, iy, iz) += FACE_FRONT;
						nverts += 6;
					}
				}
			}
		}
	}

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
						{{ix*16, iy*16, iz*16, CHUNK_AT(chunk, ix, iy, iz)}, {0, 127, 127, 0} },
						{{ix*16, iy*16, (iz+1)*16, CHUNK_AT(chunk, ix, iy, iz)}, {0, 127, 127, 0} },
						{{ix*16, (iy+1)*16, iz*16, CHUNK_AT(chunk, ix, iy, iz)}, {0, 127, 127, 0} },
						{{ix*16, (iy+1)*16, (iz+1)*16, CHUNK_AT(chunk, ix, iy, iz)}, {0, 127, 127, 0} },
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
						{{(ix+1)*16, iy*16, iz*16, CHUNK_AT(chunk, ix, iy, iz)}, {255, 127, 127, 0} },
						{{(ix+1)*16, (iy+1)*16, iz*16, CHUNK_AT(chunk, ix, iy, iz)}, {255, 127, 127, 0} },
						{{(ix+1)*16, (iy+1)*16, (iz+1)*16, CHUNK_AT(chunk, ix, iy, iz)}, {255, 127, 127, 0} },
						{{(ix+1)*16, iy*16, (iz+1)*16, CHUNK_AT(chunk, ix, iy, iz)}, {255, 127, 127, 0} },
					};
					verts[vi + 0] = corners[0];
					verts[vi + 1] = corners[1];
					verts[vi + 2] = corners[2];
					verts[vi + 3] = corners[1];
					verts[vi + 4] = corners[2];
					verts[vi + 5] = corners[3];
					vi += 6;
				}
				if (FACE_AT(ix, iy, iz) & FACE_TOP) {
					game_block_vtx corners[4] = {
						{{ix*16, (iy+1)*16, iz*16, CHUNK_AT(chunk, ix, iy, iz)}, {127, 255, 127, 0} },
						{{ix*16, (iy+1)*16, (iz+1)*16, CHUNK_AT(chunk, ix, iy, iz)}, {127, 255, 127, 0} },
						{{(ix+1)*16, (iy+1)*16, (iz+1)*16, CHUNK_AT(chunk, ix, iy, iz)}, {127, 255, 127, 0} },
						{{(ix+1)*16, (iy+1)*16, iz*16, CHUNK_AT(chunk, ix, iy, iz)}, {127, 255, 127, 0} },
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
						{{ix*16, iy*16, iz*16, CHUNK_AT(chunk, ix, iy, iz)}, {127, 0, 127, 0} },
						{{ix*16, iy*16, (iz+1)*16, CHUNK_AT(chunk, ix, iy, iz)}, {127, 0, 127, 0} },
						{{(ix+1)*16, iy*16, (iz+1)*16, CHUNK_AT(chunk, ix, iy, iz)}, {127, 0, 127, 0} },
						{{(ix+1)*16, iy*16, iz*16, CHUNK_AT(chunk, ix, iy, iz)}, {127, 0, 127, 0} },
					};
					verts[vi + 0] = corners[0];
					verts[vi + 1] = corners[1];
					verts[vi + 2] = corners[2];
					verts[vi + 3] = corners[0];
					verts[vi + 4] = corners[2];
					verts[vi + 5] = corners[3];
					vi += 6;
				}
				if (FACE_AT(ix, iy, iz) & FACE_FRONT) {
					game_block_vtx corners[4] = {
						{{ix*16, iy*16, (iz+1)*16, CHUNK_AT(chunk, ix, iy, iz)}, {127, 127, 255, 0} },
						{{(ix+1)*16, iy*16, (iz+1)*16, CHUNK_AT(chunk, ix, iy, iz)}, {127, 127, 255, 0} },
						{{(ix+1)*16, (iy+1)*16, (iz+1)*16, CHUNK_AT(chunk, ix, iy, iz)}, {127, 127, 255, 0} },
						{{ix*16, (iy+1)*16, (iz+1)*16, CHUNK_AT(chunk, ix, iy, iz)}, {127, 127, 255, 0} },
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
						{{ix*16, iy*16, iz*16, CHUNK_AT(chunk, ix, iy, iz)}, {127, 127, 0, 0} },
						{{(ix+1)*16, iy*16, iz*16, CHUNK_AT(chunk, ix, iy, iz)}, {127, 127, 0, 0} },
						{{(ix+1)*16, (iy+1)*16, iz*16, CHUNK_AT(chunk, ix, iy, iz)}, {127, 127, 0, 0} },
						{{ix*16, (iy+1)*16, iz*16, CHUNK_AT(chunk, ix, iy, iz)}, {127, 127, 0, 0} },
					};
					verts[vi + 0] = corners[0];
					verts[vi + 1] = corners[1];
					verts[vi + 2] = corners[2];
					verts[vi + 3] = corners[0];
					verts[vi + 4] = corners[2];
					verts[vi + 5] = corners[3];
					vi += 6;
				}
			}
		}
	}

	mlCreateMesh(mesh, nverts, verts, ML_POS_4UB | ML_N_4UB | ML_CLR_4UB);
	free(verts);
	return nverts;
}
