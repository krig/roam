/* code that generates geometry (like cube, sphere, etc) */
#include "common.h"
#include "math3d.h"
#include "geometry.h"

/* top = 0xff24C6DC;
   bottom = 0xff514A9D;
*/

void make_cube(mesh_t* mesh, vec3_t size, uint32_t top_clr, uint32_t bottom_clr) {
	posnormalclrvert_t vtx[] = {
		{{-0.5f*size.x,-0.5f*size.y,-0.5f*size.z }, { 0, 0, 0 }, bottom_clr },
		{{ 0.5f*size.x,-0.5f*size.y,-0.5f*size.z }, { 0, 0, 0 }, bottom_clr },
		{{-0.5f*size.x,-0.5f*size.y, 0.5f*size.z }, { 0, 0, 0 }, bottom_clr },
		{{ 0.5f*size.x,-0.5f*size.y, 0.5f*size.z }, { 0, 0, 0 }, bottom_clr },
		{{-0.5f*size.x, 0.5f*size.y,-0.5f*size.z }, { 0, 0, 0 }, top_clr },
		{{-0.5f*size.x, 0.5f*size.y, 0.5f*size.z }, { 0, 0, 0 }, top_clr },
		{{ 0.5f*size.x, 0.5f*size.y,-0.5f*size.z }, { 0, 0, 0 }, top_clr },
		{{ 0.5f*size.x, 0.5f*size.y, 0.5f*size.z }, { 0, 0, 0 }, top_clr }
	};

	posnormalclrvert_t tris[36];
		// bottom
		tris[ 0] = vtx[0], tris[ 1] = vtx[1], tris[ 2] = vtx[2], tris[ 3] = vtx[1], tris[ 4] = vtx[3], tris[ 5] = vtx[2],
		// top
		tris[ 6] = vtx[4], tris[ 7] = vtx[5], tris[ 8] = vtx[6], tris[ 9] = vtx[6], tris[10] = vtx[5], tris[11] = vtx[7],
		// front
		tris[12] = vtx[2], tris[13] = vtx[3], tris[14] = vtx[5], tris[15] = vtx[3], tris[16] = vtx[7], tris[17] = vtx[5],
		// back
		tris[18] = vtx[0], tris[19] = vtx[4], tris[20] = vtx[1], tris[21] = vtx[1], tris[22] = vtx[4], tris[23] = vtx[6],
		// left
		tris[24] = vtx[2], tris[25] = vtx[4], tris[26] = vtx[0], tris[27] = vtx[2], tris[28] = vtx[5], tris[29] = vtx[4],
		// right
		tris[30] = vtx[3], tris[31] = vtx[1], tris[32] = vtx[6], tris[33] = vtx[3], tris[34] = vtx[6], tris[35] = vtx[7];

	vec3_t normals[] = {{ 0,-1, 0 }, { 0, 1, 0 }, { 0, 0, 1 },
	                     { 0, 0,-1 }, {-1, 0, 0 }, { 1, 0, 0 }};

	for (int i = 0; i < 6; ++i) {
		tris[i*6 + 0].n = normals[i];
		tris[i*6 + 1].n = normals[i];
		tris[i*6 + 2].n = normals[i];
		tris[i*6 + 3].n = normals[i];
		tris[i*6 + 4].n = normals[i];
		tris[i*6 + 5].n = normals[i];
	}

	m_create_mesh(mesh, 36, tris, ML_POS_3F | ML_N_3F | ML_CLR_4UB, GL_STATIC_DRAW);
}

// faceverts must be able to hold 20 * 3 verts
static void initial_icosahedron(vec3_t* faceverts) {
	int i;
	vec3_t verts[12];

	double theta = 26.56505117707799 * ML_PI / 180.0; // refer paper for theta value
	double stheta = sin(theta);
	double ctheta = cos(theta);

	// lower vertex
	m_setvec3(verts[0], 0, 0, -1.f);

	// lower pentagon
	double phi = ML_PI / 5.0;
	for (i = 1; i < 6; ++i) {
		m_setvec3(verts[i], ctheta * cos(phi), ctheta * sin(phi), -stheta);
		phi += 2.0 * ML_PI / 5.0;
	}

	// upper pentagon
	phi = 0.0;
	for (i = 6; i < 11; ++i) {
		m_setvec3(verts[i], ctheta * cos(phi), ctheta * sin(phi), stheta);
		phi += 2.0 * ML_PI / 5.0;
	}

	// upper vertex
	m_setvec3(verts[11], 0, 0, 1.f);

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

static void initial_hemi_icosahedron(vec3_t* faceverts) {
	int i;
	vec3_t verts[12];

	double theta = 26.56505117707799 * ML_PI / 180.0; // refer paper for theta value
	double stheta = sin(theta);
	double ctheta = cos(theta);
	double phi = ML_PI / 5.0;

	// lower pentagon
	for (i = 1; i < 6; ++i) {
		m_setvec3(verts[i], ctheta * cos(phi), -stheta, ctheta * sin(phi));
		phi += 2.0 * ML_PI / 5.0;
	}

	// upper pentagon
	phi = 0.0;
	for (i = 6; i < 11; ++i) {
		m_setvec3(verts[i], ctheta * cos(phi), stheta, ctheta * sin(phi));
		phi += 2.0 * ML_PI / 5.0;
	}

	// upper vertex
	m_setvec3(verts[11], 0, 1.f, 0.f);

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


static size_t subdivide_sphere(vec3_t* to, vec3_t* from, size_t nverts) {
	// for each subdivision pass, pop N/3 verts
	// off, push the subdivided verts back
	size_t nout = 0;
	size_t nfaces = nverts / 3;
	vec3_t v1, v2, v3, v4, v5, v6;
	for (size_t i = 0; i < nfaces; ++i) {
		v1 = from[(i*3) + 0];
		v2 = from[(i*3) + 1];
		v3 = from[(i*3) + 2];
		v4 = m_vec3normalize(m_vec3add(v1, v2));
		v5 = m_vec3normalize(m_vec3add(v2, v3));
		v6 = m_vec3normalize(m_vec3add(v3, v1));
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

void make_sphere(mesh_t* mesh, float radius, int subdivisions) {
	int i, v;
	size_t nverts, currverts;
	vec3_t* verts[2];
	// 0 = 60 * 1
	// 1 = 60 * 4
	// 2 = 60 * 4 * 4
	nverts = 60 * pow(4, subdivisions);
	verts[0] = malloc(nverts * sizeof(vec3_t) * 2);
	verts[1] = verts[0] + nverts;
	initial_icosahedron(verts[0]);
	currverts = 60;
	v = 0;
	for (i = 0; i < subdivisions; ++i) {
		currverts = subdivide_sphere(verts[(v + 1) % 2], verts[v], currverts);
		v = (v + 1) % 2;
	}
	m_create_mesh(mesh, currverts, verts[subdivisions % 2], ML_POS_3F, GL_STATIC_DRAW);
	free(verts[0]);
}


void make_hemisphere(mesh_t* mesh, float radius, int subdivisions) {
	int i, v;
	size_t nverts, currverts;
	vec3_t* verts[2];
	// 0 = 45 * 1
	// 1 = 45 * 4
	// 2 = 45 * 4 * 4
	nverts = 45 * pow(4, subdivisions);
	verts[0] = malloc(nverts * sizeof(vec3_t) * 2);
	verts[1] = verts[0] + nverts;
	initial_hemi_icosahedron(verts[0]);
	currverts = 45;
	v = 0;
	for (i = 0; i < subdivisions; ++i) {
		currverts = subdivide_sphere(verts[(v + 1) % 2], verts[v], currverts);
		v = (v + 1) % 2;
	}
	m_create_mesh(mesh, currverts, verts[subdivisions % 2], ML_POS_3F, GL_STATIC_DRAW);
	free(verts[0]);
}
