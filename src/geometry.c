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

// faceverts must be able to hold 20 * 3 verts
static void initial_icosahedron(ml_vec3* faceverts) {
	int i;
	ml_vec3 verts[12];

	double theta = 26.56505117707799 * ML_PI / 180.0; // refer paper for theta value
	double stheta = sin(theta);
	double ctheta = cos(theta);

	// lower vertex
	mlVec3Assign(verts[0], 0, 0, -1.f);

	// lower pentagon
	double phi = ML_PI / 5.0;
	for (i = 1; i < 6; ++i) {
		mlVec3Assign(verts[i], ctheta * cos(phi), ctheta * sin(phi), -stheta);
		phi += 2.0 * ML_PI / 5.0;
	}

	// upper pentagon
	phi = 0.0;
	for (i = 6; i < 11; ++i) {
		mlVec3Assign(verts[i], ctheta * cos(phi), ctheta * sin(phi), stheta);
		phi += 2.0 * ML_PI / 5.0;
	}

	// upper vertex
	mlVec3Assign(verts[11], 0, 0, 1.f);

	i = 0;
#undef FACE
#define FACE(v0, v1, v2) { faceverts[i++] = verts[v0]; faceverts[i++] = verts[v1]; faceverts[i++] = verts[v2]; }
	FACE(0, 2, 1);
	FACE(0, 3, 2);
	FACE(0, 4, 3);
	FACE(0, 5, 4);
	FACE(0, 1, 5);
	FACE(1, 2, 7);
	FACE(2, 3, 8);
	FACE(3, 4, 9);
	FACE(4, 5, 10);
	FACE(5, 1, 6);
	FACE(1, 7, 6);
	FACE(2, 8, 7);
	FACE(3, 9, 8);
	FACE(4, 10, 9);
	FACE(5, 6, 10);
	FACE(6, 7, 11);
	FACE(7, 8, 11);
	FACE(8, 9, 11);
	FACE(9, 10, 11);
	FACE(10, 6, 11);
#undef FACE
}

static void initial_hemi_icosahedron(ml_vec3* faceverts) {
	int i;
	ml_vec3 verts[12];

	double theta = 26.56505117707799 * ML_PI / 180.0; // refer paper for theta value
	double stheta = sin(theta);
	double ctheta = cos(theta);
	double phi = ML_PI / 5.0;

	// lower pentagon
	for (i = 1; i < 6; ++i) {
		mlVec3Assign(verts[i], ctheta * cos(phi), -stheta, ctheta * sin(phi));
		phi += 2.0 * ML_PI / 5.0;
	}

	// upper pentagon
	phi = 0.0;
	for (i = 6; i < 11; ++i) {
		mlVec3Assign(verts[i], ctheta * cos(phi), stheta, ctheta * sin(phi));
		phi += 2.0 * ML_PI / 5.0;
	}

	// upper vertex
	mlVec3Assign(verts[11], 0, 1.f, 0.f);

	i = 0;
#undef FACE
#define FACE(v0, v1, v2) { faceverts[i++] = verts[v0]; faceverts[i++] = verts[v1]; faceverts[i++] = verts[v2]; }
	FACE(2, 1, 7);
	FACE(3, 2, 8);
	FACE(4, 3, 9);
	FACE(5, 4, 10);
	FACE(1, 5, 6);
	FACE(7, 1, 6);
	FACE(8, 2, 7);
	FACE(9, 3, 8);
	FACE(10, 4, 9);
	FACE(6, 5, 10);
	FACE(7, 6, 11);
	FACE(8, 7, 11);
	FACE(9, 8, 11);
	FACE(10, 9, 11);
	FACE(6, 10, 11);
#undef FACE
}


static size_t subdivide_sphere(ml_vec3* to, ml_vec3* from, size_t nverts) {
	// for each subdivision pass, pop N/3 verts
	// off, push the subdivided verts back
	size_t nout = 0;
	size_t nfaces = nverts / 3;
	ml_vec3 v1, v2, v3, v4, v5, v6;
	for (size_t i = 0; i < nfaces; ++i) {
		v1 = from[(i*3) + 0];
		v2 = from[(i*3) + 1];
		v3 = from[(i*3) + 2];
		v4 = mlVec3Normalize(mlVec3Add(v1, v2));
		v5 = mlVec3Normalize(mlVec3Add(v2, v3));
		v6 = mlVec3Normalize(mlVec3Add(v3, v1));
		to[nout++] = v1;
		to[nout++] = v4;
		to[nout++] = v6;
		to[nout++] = v4;
		to[nout++] = v2;
		to[nout++] = v5;
		to[nout++] = v6;
		to[nout++] = v5;
		to[nout++] = v3;
		to[nout++] = v6;
		to[nout++] = v4;
		to[nout++] = v5;
	}
	return nout;
}

void makeSphere(ml_mesh* mesh, float radius, int subdivisions) {
	int i, v;
	size_t nverts, currverts;
	ml_vec3* verts[2];
	// 0 = 60 * 1
	// 1 = 60 * 4
	// 2 = 60 * 4 * 4
	nverts = 60 * pow(4, subdivisions);
	verts[0] = malloc(nverts * sizeof(ml_vec3));
	verts[1] = malloc(nverts * sizeof(ml_vec3));
	initial_icosahedron(verts[0]);
	currverts = 60;
	v = 0;
	for (i = 0; i < subdivisions; ++i) {
		currverts = subdivide_sphere(verts[(v + 1) % 2], verts[v], currverts);
		v = (v + 1) % 2;
	}
	mlCreateMesh(mesh, currverts, verts[subdivisions % 2], ML_POS_3F);
}


void makeHemisphere(ml_mesh* mesh, float radius, int subdivisions) {
	int i, v;
	size_t nverts, currverts;
	ml_vec3* verts[2];
	// 0 = 45 * 1
	// 1 = 45 * 4
	// 2 = 45 * 4 * 4
	nverts = 45 * pow(4, subdivisions);
	verts[0] = malloc(nverts * sizeof(ml_vec3));
	verts[1] = malloc(nverts * sizeof(ml_vec3));
	initial_hemi_icosahedron(verts[0]);
	currverts = 45;
	v = 0;
	for (i = 0; i < subdivisions; ++i) {
		currverts = subdivide_sphere(verts[(v + 1) % 2], verts[v], currverts);
		v = (v + 1) % 2;
	}
	mlCreateMesh(mesh, currverts, verts[subdivisions % 2], ML_POS_3F);
}
