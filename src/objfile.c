#include "common.h"
#include "math3d.h"
#include "objfile.h"

#define LINEBUF_SIZE 1024

static
bool read_line(char* to, const char** data)
{
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
				fatal_error("objLoad: Line too long");
		}
	}
	*data = buf;
	*wp = '\0';
	return true;
}

/*
  Based on stb.h: stb_strtok_raw()
 */
static
char* strtok_x(char* output, char* tokens, const char* delim)
{
	while (*tokens && strchr(delim, *tokens) != NULL)
		tokens++;
	while (*tokens && strchr(delim, *tokens) == NULL)
		*output++ = *tokens++;
	*output = '\0';
	return *tokens ? tokens + 1 : tokens;
}

// this data

static
void push_vert(obj_t* m, float* v, size_t n)
{
	if (n != 3)
		fatal_error("Only 3D vertices allowed in obj: n = %zu", n);
	while (m->nverts + n > m->vcap) {
		m->vcap *= 2;
		m->verts = realloc(m->verts, m->vcap * sizeof(float));
	}
	memcpy(m->verts + m->nverts, v, n * sizeof(float));
	m->nverts += n;
}

static
void push_face(obj_t* m, uint32_t* f, size_t n)
{
	if (n != 3)
		fatal_error("Only triangles allowed in obj: n = %zu", n);
	while (m->nindices + n > m->fcap) {
		m->fcap *= 2;
		m->indices = realloc(m->indices, m->fcap * sizeof(uint32_t));
	}
	memcpy(m->indices + m->nindices, f, n * sizeof(uint32_t));
	m->nindices += n;
}

void obj_load(obj_t* mesh, const char* data, float vscale)
{
	char linebuf[LINEBUF_SIZE];
	char tokbuf[LINEBUF_SIZE];
	const char* delim = " \t\r\n";
	float tmpvert[4];
	uint32_t tmpindex[3];
	size_t i;

	memset(mesh, 0, sizeof(obj_t));
	mesh->vcap = 1024;
	mesh->fcap = 1024;
	mesh->verts = malloc(mesh->vcap * sizeof(float));
	mesh->indices = malloc(mesh->fcap * sizeof(uint32_t));

	while (read_line(linebuf, &data)) {
		char* tok = strtok_x(tokbuf, linebuf, delim);
		if (strcmp(tokbuf, "v") == 0) {
			// vertex
			i = 0;
			do {
				tok = strtok_x(tokbuf, tok, delim);
				tmpvert[i++] = atof(tokbuf) * vscale;
				if (i > 4)
					fatal_error("Too many dimensions (%d) in .obj vertex", i);
			} while (*tok != '\0');
			push_vert(mesh, tmpvert, i);
		} else if (strcmp(tokbuf, "f") == 0) {
			i = 0;
			do {
				tok = strtok_x(tokbuf, tok, delim);
				// - 1 because .obj indices are 1-based
				tmpindex[i++] = (uint32_t)atol(tokbuf) - 1;
				if (i > 3)
					fatal_error("Too many vertices in face (%d)", i);
			} while (*tok != '\0');
			push_face(mesh, tmpindex, i);
		} else {
			// TODO: vt, vn, f v/vt/vn
			fatal_error("Unhandled token: '%s'", tokbuf);
		}
	}
}

void obj_free(obj_t* mesh)
{
	free(mesh->verts);
	free(mesh->indices);
	memset(mesh, 0, sizeof(obj_t));
}


void obj_normals(obj_t* obj, void** vertexdata, size_t* vertexsize, GLenum* meshflags)
{
	size_t i;
	size_t nvertices = obj->nverts / 3;
	posnormalvert_t* verts = malloc(sizeof(posnormalvert_t) * nvertices);
	for (i = 0; i < nvertices; ++i) {
		memcpy(&verts[i].pos.x, obj->verts + (i * 3), sizeof(float) * 3);
	}

	for (i = 0; i < obj->nindices; i += 3) {
		if (obj->indices[i + 0] > nvertices)
			fatal_error("index out of bound: %u", obj->indices[i + 0]);
		if (obj->indices[i + 1] > nvertices)
			fatal_error("index out of bound: %u", obj->indices[i + 1]);
		if (obj->indices[i + 2] > nvertices)
			fatal_error("index out of bound: %u", obj->indices[i + 2]);
		vec3_t t1 = verts[obj->indices[i + 0]].pos;
		vec3_t t2 = verts[obj->indices[i + 1]].pos;
		vec3_t t3 = verts[obj->indices[i + 2]].pos;
		vec3_t n = m_vec3normalize(m_vec3cross(m_vec3sub(t2, t1), m_vec3sub(t3, t1)));
		verts[obj->indices[i + 0]].n = n;
		verts[obj->indices[i + 1]].n = n;
		verts[obj->indices[i + 2]].n = n;
	}

	*vertexsize = sizeof(posnormalvert_t);
	*vertexdata = verts;
	*meshflags = ML_POS_3F | ML_N_3F;
}

void obj_createmesh(mesh_t* mesh, obj_t* obj, obj_meshgenfn fn)
{

	void* vtxdata;
	GLenum meshflags;
	size_t vertexsize;
	(*fn)(obj, &vtxdata, &vertexsize, &meshflags);

	m_create_indexed_mesh(mesh,
	                    obj->nverts / 3,
	                    vtxdata,
	                    obj->nindices,
	                    GL_UNSIGNED_INT,
	                    obj->indices,
	                    meshflags);
	free(vtxdata);
}
