#include "common.h"
#include "math3d.h"
#include "objfile.h"

#define LINEBUF_SIZE 1024

static bool
readLine(char* to, const char** data) {
	char* wp = to;
	const char* buf = *data;
	while (wp == to) {
		if (!*buf)
			return false;
		// skip whitespace
		while (*buf && isspace(*buf))
			++buf;
		// skip comments
		if (*buf == '#') {
			while (*buf && *buf != '\n')
				++buf;
			continue;
		}
		// copy line data
		while (*buf && *buf != '\n') {
			*wp++ = *buf++;
			if (wp - to >= LINEBUF_SIZE)
				roamError("objLoad: Line too long");
		}
	}
	*data = buf;
	*wp = '\0';
	return true;
}

/*
  Based on stb.h: stb_strtok_raw()
 */
static char*
strtok_x(char* output, char* tokens, const char* delim) {
	while (*tokens && strchr(delim, *tokens) != NULL)
		tokens++;
	while (*tokens && strchr(delim, *tokens) == NULL)
		*output++ = *tokens++;
	*output = '\0';
	return *tokens ? tokens + 1 : tokens;
}

// this data

static void
pushVertex(obj_mesh* m, float* v, size_t n) {
	if (n != 3)
		roamError("Only 3D vertices allowed in obj: n = %zu", n);
	while (m->nverts + n > m->vcap) {
		m->vcap *= 2;
		m->verts = realloc(m->verts, m->vcap * sizeof(float));
	}
	memcpy(m->verts + m->nverts, v, n * sizeof(float));
	m->nverts += n;
}

static void
pushFace(obj_mesh* m, uint32_t* f, size_t n) {
	if (n != 3)
		roamError("Only triangles allowed in obj: n = %zu", n);
	while (m->nindices + n > m->fcap) {
		m->fcap *= 2;
		m->indices = realloc(m->indices, m->fcap * sizeof(uint32_t));
	}
	memcpy(m->indices + m->nindices, f, n * sizeof(uint32_t));
	m->nindices += n;
}

void objLoad(obj_mesh* mesh, const char* data, float vscale) {
	char linebuf[LINEBUF_SIZE];
	char tokbuf[LINEBUF_SIZE];
	const char* delim = " \t\r\n";
	float tmpvert[4];
	uint32_t tmpindex[3];
	size_t i;

	memset(mesh, 0, sizeof(obj_mesh));
	mesh->vcap = 1024;
	mesh->fcap = 1024;
	mesh->verts = malloc(mesh->vcap * sizeof(float));
	mesh->indices = malloc(mesh->fcap * sizeof(uint32_t));

	while (readLine(linebuf, &data)) {
		char* tok = strtok_x(tokbuf, linebuf, delim);
		if (strcmp(tokbuf, "v") == 0) {
			// vertex
			i = 0;
			do {
				tok = strtok_x(tokbuf, tok, delim);
				tmpvert[i++] = atof(tokbuf) * vscale;
				if (i > 4)
					roamError("Too many dimensions (%d) in .obj vertex", i);
			} while (*tok != '\0');
			pushVertex(mesh, tmpvert, i);
		} else if (strcmp(tokbuf, "f") == 0) {
			i = 0;
			do {
				tok = strtok_x(tokbuf, tok, delim);
				// - 1 because .obj indices are 1-based
				tmpindex[i++] = (uint32_t)atol(tokbuf) - 1;
				if (i > 3)
					roamError("Too many vertices in face (%d)", i);
			} while (*tok != '\0');
			pushFace(mesh, tmpindex, i);
		} else {
			// TODO: vt, vn, f v/vt/vn
			roamError("Unhandled token: '%s'", tokbuf);
		}
	}
}

void objGenNormalsFn(obj_mesh* obj, void** vertexdata, size_t* vertexsize, GLenum* meshflags) {
	size_t i;
	size_t nvertices = obj->nverts / 3;
	ml_vtx_pos_n* verts = malloc(sizeof(ml_vtx_pos_n) * nvertices);
	for (i = 0; i < nvertices; ++i) {
		memcpy(&verts[i].pos.x, obj->verts + (i * 3), sizeof(float) * 3);
	}

	for (i = 0; i < obj->nindices; i += 3) {
		if (obj->indices[i + 0] > nvertices)
			roamError("index out of bound: %u", obj->indices[i + 0]);
		if (obj->indices[i + 1] > nvertices)
			roamError("index out of bound: %u", obj->indices[i + 1]);
		if (obj->indices[i + 2] > nvertices)
			roamError("index out of bound: %u", obj->indices[i + 2]);
		ml_vec3 t1 = verts[obj->indices[i + 0]].pos;
		ml_vec3 t2 = verts[obj->indices[i + 1]].pos;
		ml_vec3 t3 = verts[obj->indices[i + 2]].pos;
		ml_vec3 n = mlVec3Normalize(mlVec3Cross(mlVec3Sub(t2, t1), mlVec3Sub(t3, t1)));
		verts[obj->indices[i + 0]].n = n;
		verts[obj->indices[i + 1]].n = n;
		verts[obj->indices[i + 2]].n = n;
	}

	*vertexsize = sizeof(ml_vtx_pos_n);
	*vertexdata = verts;
	*meshflags = ML_POS_3F | ML_N_3F;
}

void objCreateMesh(ml_mesh* mesh, obj_mesh* obj, objCreateMeshGenFn fn) {

	void* vtxdata;
	GLenum meshflags;
	size_t vertexsize;
	(*fn)(obj, &vtxdata, &vertexsize, &meshflags);

	mlCreateIndexedMesh(mesh,
	                    obj->nverts / 3,
	                    vtxdata,
	                    obj->nindices,
	                    GL_UNSIGNED_INT,
	                    obj->indices,
	                    meshflags);
	free(vtxdata);
}
