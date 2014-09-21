#pragma once

typedef struct obj_mesh {
	float* verts;
	uint32_t* indices;
	size_t nverts;
	size_t vcap;
	size_t nindices;
	size_t fcap;
} obj_mesh;

char* osReadWholeFile(const char* filename);

void objLoad(obj_mesh* mesh, const char* data, float vscale);

typedef void (*objCreateMeshGenFn)(obj_mesh* obj, void** vertexdata, size_t* vertexsize, GLenum* meshflags);

void objGenNormalsFn(obj_mesh* obj, void** vertexdata, size_t* vertexsize, GLenum* meshflags);

void objCreateMesh(ml_mesh* mesh, obj_mesh* obj, objCreateMeshGenFn fn);

static inline void
objFree(obj_mesh* mesh) {
	free(mesh->verts);
	free(mesh->indices);
	memset(mesh, 0, sizeof(obj_mesh));
}

