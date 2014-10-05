/* code that generates geometry (like cube, sphere, etc) */
#include "common.h"
#include "math3d.h"
#include "geometry.h"

void makeCube() {
	uint32_t top, bottom;
	top = 0xff24C6DC;
	bottom = 0xff514A9D;
	ml_vtx_pos_n_clr corners[8] = {
		{{-0.5f,-0.5f,-0.5f }, { 0, 0, 0 }, bottom },
		{{ 0.5f,-0.5f,-0.5f }, { 0, 0, 0 }, bottom },
		{{-0.5f,-0.5f, 0.5f }, { 0, 0, 0 }, bottom },
		{{ 0.5f,-0.5f, 0.5f }, { 0, 0, 0 }, bottom },
		{{-0.5f, 0.5f,-0.5f }, { 0, 0, 0 }, top },
		{{-0.5f, 0.5f, 0.5f }, { 0, 0, 0 }, top },
		{{ 0.5f, 0.5f,-0.5f }, { 0, 0, 0 }, top },
		{{ 0.5f, 0.5f, 0.5f }, { 0, 0, 0 }, top }
	};

	ml_vtx_pos_n_clr tris[] = {
		corners[0], // bottom
		corners[1],
		corners[2],
		corners[1],
		corners[3],
		corners[2],

		corners[4], // top
		corners[5],
		corners[6],
		corners[6],
		corners[5],
		corners[7],

		corners[2], // front
		corners[3],
		corners[5],
		corners[3],
		corners[7],
		corners[5],

		corners[0], // back
		corners[4],
		corners[1],
		corners[1],
		corners[4],
		corners[6],

		corners[2], // left
		corners[4],
		corners[0],
		corners[2],
		corners[5],
		corners[4],

		corners[3], // right
		corners[1],
		corners[6],
		corners[3],
		corners[6],
		corners[7]
	};

	ml_vec3 normals[6] = {
		{ 0,-1, 0 },
		{ 0, 1, 0 },
		{ 0, 0, 1 },
		{ 0, 0,-1 },
		{-1, 0, 0 },
		{ 1, 0, 0 }
	};

	for (int i = 0; i < 6; ++i) {
		tris[i*6 + 0].n = normals[i];
		tris[i*6 + 1].n = normals[i];
		tris[i*6 + 2].n = normals[i];
		tris[i*6 + 3].n = normals[i];
		tris[i*6 + 4].n = normals[i];
		tris[i*6 + 5].n = normals[i];
	}

}
