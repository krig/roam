/* code that generates geometry (like cube, sphere, etc) */
#include "common.h"
#include "math3d.h"
#include "geometry.h"

/* top = 0xff24C6DC;
   bottom = 0xff514A9D;
*/

void makeCube(ml_mesh* mesh, ml_vec3 size, uint32_t top_clr, uint32_t bottom_clr) {
	ml_vtx_pos_n_clr vtx[] = {
		{{-0.5f*size.x,-0.5f*size.y,-0.5f*size.z }, { 0, 0, 0 }, bottom_clr },
		{{ 0.5f*size.x,-0.5f*size.y,-0.5f*size.z }, { 0, 0, 0 }, bottom_clr },
		{{-0.5f*size.x,-0.5f*size.y, 0.5f*size.z }, { 0, 0, 0 }, bottom_clr },
		{{ 0.5f*size.x,-0.5f*size.y, 0.5f*size.z }, { 0, 0, 0 }, bottom_clr },
		{{-0.5f*size.x, 0.5f*size.y,-0.5f*size.z }, { 0, 0, 0 }, top_clr },
		{{-0.5f*size.x, 0.5f*size.y, 0.5f*size.z }, { 0, 0, 0 }, top_clr },
		{{ 0.5f*size.x, 0.5f*size.y,-0.5f*size.z }, { 0, 0, 0 }, top_clr },
		{{ 0.5f*size.x, 0.5f*size.y, 0.5f*size.z }, { 0, 0, 0 }, top_clr }
	};

	ml_vtx_pos_n_clr tris[] = {
		// bottom
		vtx[0], vtx[1], vtx[2], vtx[1], vtx[3], vtx[2],
		// top
		vtx[4], vtx[5], vtx[6], vtx[6], vtx[5], vtx[7],
		// front
		vtx[2], vtx[3], vtx[5], vtx[3], vtx[7], vtx[5],
		// back
		vtx[0], vtx[4], vtx[1], vtx[1], vtx[4], vtx[6],
		// left
		vtx[2], vtx[4], vtx[0], vtx[2], vtx[5], vtx[4],
		// right
		vtx[3], vtx[1], vtx[6], vtx[3], vtx[6], vtx[7]
	};

	ml_vec3 normals[] = {{ 0,-1, 0 }, { 0, 1, 0 }, { 0, 0, 1 },
	                     { 0, 0,-1 }, {-1, 0, 0 }, { 1, 0, 0 }};

	for (int i = 0; i < 6; ++i) {
		tris[i*6 + 0].n = normals[i];
		tris[i*6 + 1].n = normals[i];
		tris[i*6 + 2].n = normals[i];
		tris[i*6 + 3].n = normals[i];
		tris[i*6 + 4].n = normals[i];
		tris[i*6 + 5].n = normals[i];
	}

	mlCreateMesh(mesh, 36, tris, ML_POS_3F | ML_N_3F | ML_CLR_4UB);
}
