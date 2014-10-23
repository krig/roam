#pragma once

typedef struct obj_t {
	float* verts;
	uint32_t* indices;
	size_t nverts;
	size_t vcap;
	size_t nindices;
	size_t fcap;
} obj_t;

typedef void (*obj_meshgenfn)(obj_t* obj, void** vertexdata, size_t* vertexsize, GLenum* meshflags);


void obj_load(obj_t* mesh, const char* data, float vscale);
void obj_free(obj_t* mesh);
void obj_normals(obj_t* obj, void** vertexdata, size_t* vertexsize, GLenum* meshflags);
void obj_createmesh(mesh_t* mesh, obj_t* obj, obj_meshgenfn fn);

