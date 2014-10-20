#pragma once

#include <GL/glew.h>
#include <SDL.h>
#include <SDL_opengl.h>

#define ML_PI 3.14159265358979323846	/* pi */
#define ML_TWO_PI (3.14159265358979323846 * 2.0)
#define ML_EPSILON (1.0e-4f)
#define ML_EPSILON2 (1.0e-5f)
#define ML_E 2.71828182845904523536
#define ML_LOG2E 1.44269504088896340736
#define ML_PI_2 1.57079632679489661923
#define ML_PI_4 0.785398163397448309616
#define ML_MIN(a, b) (((b) < (a)) ? (b) : (a))
#define ML_MAX(a, b) (((b) > (a)) ? (b) : (a))

typedef struct ml_vec2 {
	float x, y;
} ml_vec2;

typedef struct ml_vec3 {
	float x, y, z;
} ml_vec3;

typedef struct ml_vec4 {
	float x, y, z, w;
} ml_vec4;

typedef struct ml_clr {
	uint8_t a, r, g, b;
} ml_clr;

typedef struct ml_ivec2 {
	int x, y;
} ml_ivec2;

typedef struct ml_ivec3 {
	int x, y, z;
} ml_ivec3;

typedef struct ml_ivec4 {
	int x, y, z, w;
} ml_ivec4;

typedef struct ml_dvec3 {
	double x, y, z;
} ml_dvec3;

typedef struct ml_chunk {
	int x, z;
} ml_chunk;

typedef struct ml_matrix {
	float m[16];
} ml_matrix;

typedef struct ml_matrix33 {
	float m[9];
} ml_matrix33;


#pragma pack(push, 4)

typedef struct ml_vtx_pos_clr {
	ml_vec3 pos;
	uint32_t clr;
} ml_vtx_pos_clr;

typedef struct ml_vtx_pos_n_clr {
	ml_vec3 pos;
	ml_vec3 n;
	uint32_t clr;
} ml_vtx_pos_n_clr;

typedef struct ml_vtx_pos_n {
	ml_vec3 pos;
	ml_vec3 n;
} ml_vtx_pos_n;

typedef struct ml_vtx_ui {
	ml_vec2 pos;
	ml_vec2 tc;
	uint32_t clr;
} ml_vtx_ui;

#pragma pack(pop)

// A material identifies a
// shader and any special
// considerations concerning
// that shader
typedef struct ml_material {
	GLuint program;
	GLint projmat;
	GLint modelview;
	GLint normalmat;
	GLint chunk_offset;
	GLint amb_light;
	GLint fog_color;
	GLint light_dir;
	GLint tex0;
	GLint sun_dir;
	GLint sun_color;
	GLint sky_dark;
	GLint sky_light;
	GLint position;
	GLint texcoord;
	GLint color;
	GLint normal;
} ml_material;


// A mesh is a VBO plus metadata
// that describes how it maps to
// a material
typedef struct ml_mesh {
	GLuint vbo; // vertex buffer
	GLuint ibo; // index buffer (may be 0 if not used)
	GLint position; // -1 if not present, else offset
	GLint normal;
	GLint texcoord;
	GLint color;
	GLsizei stride;
	GLenum mode;
	GLenum ibotype;
	GLsizei count;
	GLenum flags;
} ml_mesh;

typedef struct ml_tex2d {
	GLuint id;
	uint16_t w;
	uint16_t h;
} ml_tex2d;

// a renderable combines a
// particular material and a
// particular mesh into a
// a renderable object
typedef struct ml_renderable {
	const ml_material* material;
	const ml_mesh* mesh;
	GLuint vao;
} ml_renderable;

typedef struct ml_matrixstack {
	int top;
	ml_matrix* stack;
} ml_matrixstack;


#define mlDeg2Rad(d) (((d) * ML_PI) / 180.0)
#define mlRad2Deg(r) (((r) * 180.0) / ML_PI)
#define mlSwap(a, b) do { __typeof__ (a) _swap_##__LINE__ = (a); (a) = (b); (b) = _swap_##__LINE__; } while (0)
#define mlMax(a, b) ((b) > (a) ? (b) : (a))
#define mlMin(a, b) ((b) < (a) ? (b) : (a))

static inline bool
mlFIsValid(float f) {
	return (f >= -FLT_MAX && f <= FLT_MAX);
}

static inline float
mlAbs(float x) {
	return (x >= 0) ? x : -x;
}

static inline float
mlSign(float x) {
	return (x >= 0) ? 1.f : -1.f;
}

static inline float
mlClamp(float t, float lo, float hi) {
	return (t < lo) ? lo : ((t > hi) ? hi : t);
}

static inline double
mlClampd(double t, double lo, double hi) {
	return (t < lo) ? lo : ((t > hi) ? hi : t);
}

static inline float
mlWrap(float t, float lo, float hi) {
    while (t < lo)
	    t = hi - (lo - t);
    while (t > hi)
	    t = lo + (hi - t);
    return t;
}

static inline ml_vec2
mlMakeVec2(float x, float y) {
	ml_vec2 v = { x, y };
	return v;
}

static inline ml_vec3
mlMakeVec3(float x, float y, float z) {
	ml_vec3 v = { x, y, z };
	return v;
}

static inline ml_vec4
mlMakeVec4(float x, float y, float z, float w) {
	ml_vec4 v = { x, y, z, w };
	return v;
}

#define mlVec2Assign(v, a, b) { (v).x = (a); (v).y = (b); }
#define mlVec3Assign(v, a, b, c) { (v).x = (a); (v).y = (b); (v).z = (c); }
#define mlVec4Assign(v, a, b, c, d) { (v).x = (a); (v).y = (b); (v).z = (c); (v).w = (d); }

void mlPerspective(ml_matrix* m, float fovy, float aspect, float zNear, float zFar);

void mlSetIdentity(ml_matrix* m);

void mlLookAt(ml_matrix* m,
              float eyeX, float eyeY, float eyeZ,
              float atX, float atY, float atZ,
              float upX, float upY, float upZ);

void mlCopyMatrix(ml_matrix* to, const ml_matrix* from);

void mlFPSRotation(float pitch, float yaw, ml_vec3* x, ml_vec3* y, ml_vec3* z);
void mlFPSMatrix(ml_matrix* to, ml_vec3 eye, float pitch, float yaw);

void mlMulMatrix(ml_matrix* to, const ml_matrix* by);

ml_vec4 mlMulMatVec(const ml_matrix* m, const ml_vec4* v);
ml_vec3 mlMulMatVec3(const ml_matrix* m, const ml_vec3* v);
ml_vec3 mlVec3RotateBy(const ml_matrix* m, const ml_vec3* v);

void mlTranslate(ml_matrix* m, float x, float y, float z);
void mlRotate(ml_matrix* m, float angle, float x, float y, float z);

void mlGetRotationMatrix(ml_matrix33* to, const ml_matrix* from);
ml_vec3 mlMulMat33Vec3(const ml_matrix33* m, const ml_vec3* v);

void mlTranspose(ml_matrix* m);
void mlTranspose33(ml_matrix33* m);
bool mlInvertMatrix(ml_matrix* to, const ml_matrix* from);
void mlInvertOrthoMatrix(ml_matrix* to, const ml_matrix* from);

static inline ml_vec3 mlGetXAxis(const ml_matrix* from) {
	return *(ml_vec3*)&from->m[0];
}

static inline ml_vec3 mlGetYAxis(const ml_matrix* from) {
	return *(ml_vec3*)&from->m[4];
}

static inline ml_vec3 mlGetZAxis(const ml_matrix* from) {
	return *(ml_vec3*)&from->m[8];
}


static inline float
mlVec3Dot(const ml_vec3 a, const ml_vec3 b) {
	return a.x * b.x + a.y * b.y + a.z * b.z;
}

static inline ml_vec3
mlVec3Cross(const ml_vec3 a, const ml_vec3 b) {
	ml_vec3 to;
	to.x = a.y*b.z - a.z*b.y;
	to.y = a.z*b.x - a.x*b.z;
	to.z = a.x*b.y - a.y*b.x;
	return to;
}

static inline ml_vec2
mlVec2Add(const ml_vec2 a, const ml_vec2 b) {
	ml_vec2 to = { a.x + b.x, a.y + b.y };
	return to;
}

static inline ml_vec2
mlVec2AddScalar(ml_vec2 tc, float by) {
	tc.x += by;
	tc.y += by;
	return tc;
}

static inline ml_vec3
mlVec3Add(const ml_vec3 a, const ml_vec3 b) {
	ml_vec3 to = { a.x + b.x, a.y + b.y, a.z + b.z };
	return to;
}

static inline ml_vec4
mlVec4Add(const ml_vec4 a, const ml_vec4 b) {
	ml_vec4 to = { a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w };
	return to;
}

static inline ml_vec3
mlVec3Sub(const ml_vec3 a, const ml_vec3 b) {
	ml_vec3 to = { a.x - b.x, a.y - b.y, a.z - b.z };
	return to;
}

static inline ml_vec3
mlVec3Scalef(const ml_vec3 a, float f) {
	ml_vec3 to = { a.x * f, a.y * f, a.z * f };
	return to;
}

static inline ml_vec4
mlVec4Scalef(const ml_vec4 a, float f) {
	ml_vec4 to = { a.x * f, a.y * f, a.z * f, a.w * f };
	return to;
}

static inline float
mlVec3Length2(const ml_vec3 v) {
	return v.x*v.x + v.y*v.y + v.z*v.z;
}

static inline float
mlVec3Length(const ml_vec3 v) {
	return sqrtf(v.x*v.x + v.y*v.y + v.z*v.z);
}

static inline ml_vec3
mlVec3Normalize(ml_vec3 v) {
	float invlen = 1.f / mlVec3Length(v);
	return mlVec3Scalef(v, invlen);
}

static inline ml_vec3
mlVec3Invert(const ml_vec3 v) {
	ml_vec3 to = { -v.x, -v.y, -v.z };
	return to;
}

static inline ml_vec4
mlVec4Abs(const ml_vec4 v) {
	ml_vec4 to = { fabs(v.x), fabs(v.y), fabs(v.z), fabs(v.w) };
	return to;
}

static inline ml_vec4
mlNormalizePlane(ml_vec4 plane) {
	ml_vec3 n = {plane.x, plane.y, plane.z};
	float len = mlVec3Length(n);
	n = mlVec3Scalef(n, 1.f / len);
	plane.x = n.x;
	plane.y = n.y;
	plane.z = n.z;
	plane.w = plane.w / len;
	return plane;
}

/*
  TODO: disable in debug
 */
static inline void
glCheck(int line) {
	GLenum err;
	char* msg;
	do {
		err = glGetError();
		switch (err) {
		case GL_INVALID_ENUM: msg = "GL_INVALID_ENUM"; break;
		case GL_INVALID_VALUE: msg = "GL_INVALID_VALUE"; break;
		case GL_INVALID_OPERATION: msg = "GL_INVALID_OPERATION"; break;
		case GL_INVALID_FRAMEBUFFER_OPERATION: msg = "GL_INVALID_FRAMEBUFFER_OPERATION"; break;
		case GL_OUT_OF_MEMORY: msg = "GL_OUT_OF_MEMORY"; break;
		default: msg = "(other)"; break;
		}
		if (err != GL_NO_ERROR)
			fprintf(stderr, "GL error (%d): (0x%x) %s\n", line, err, msg);
	} while (err != GL_NO_ERROR);
}

static inline void
mlUniformMatrix(GLint index, ml_matrix* mat) {
	if (index != -1)
		glUniformMatrix4fv(index, 1, GL_FALSE, mat->m);
}

static inline void
mlUniformMatrix33(GLint index, ml_matrix33* mat) {
	if (index != -1)
		glUniformMatrix3fv(index, 1, GL_FALSE, mat->m);
}

static inline void
mlUniformVec2(GLint index, ml_vec2* v) {
	if (index != -1)
		glUniform2fv(index, 1, (GLfloat*)v);
}

static inline void
mlUniformVec3(GLint index, ml_vec3* v) {
	if (index != -1)
		glUniform3fv(index, 1, (GLfloat*)v);
}

static inline void
mlUniformVec4(GLint index, ml_vec4* v) {
	if (index != -1)
		glUniform4fv(index, 1, (GLfloat*)v);
}

static inline void
mlUniform1i(GLint index, int i) {
	if (index != -1)
		glUniform1i(index, i);
}

static inline void
mlBindMatrix(GLint index, ml_matrix* mat) {
	if (index != -1)
		glUniformMatrix4fv(index, 1, GL_FALSE, mat->m);
}

static inline void
mlDrawBegin(ml_renderable* renderable) {
	glUseProgram(renderable->material->program);
	glBindVertexArray(renderable->vao);
}

static inline void
mlDrawEnd(ml_renderable* renderable) {
	const ml_mesh* mesh = renderable->mesh;
	if (mesh->ibo > 0)
		glDrawElements(mesh->mode, mesh->count, mesh->ibotype, 0);
	else
		glDrawArrays(mesh->mode, 0, mesh->count);
	glBindVertexArray(0);
	glUseProgram(0);
}

GLuint mlCompileShader(GLenum type, const char* source);

GLuint mlLinkProgram(GLuint vsh, GLuint fsh);

void mlCreateMaterial(ml_material* material, const char* vsource, const char* fsource);
void mlDestroyMaterial(ml_material* material);

enum ML_MeshFlags {
	ML_POS_2F  = 0x01,
	ML_POS_3F  = 0x02,
	ML_POS_4UB = 0x04,
	ML_POS_10_2 = 0x08,
	ML_N_3F    = 0x10,
	ML_N_4B   = 0x20,
	ML_TC_2F   = 0x40,
	ML_TC_2US  = 0x80,
	ML_CLR_4UB = 0x100
};

void mlCreateMesh(ml_mesh* mesh, size_t n, void* data, GLenum flags);
void mlCreateIndexedMesh(ml_mesh* mesh, size_t n, void* data, size_t ilen, GLenum indextype, void* indices, GLenum flags);
void mlDestroyMesh(ml_mesh* mesh);

void mlCreateRenderable(ml_renderable* renderable, const ml_material* material, const ml_mesh* mesh);
void mlDestroyRenderable(ml_renderable* renderable);
void mlMapMeshToMaterial(const ml_mesh* mesh, const ml_material* material);

// Matrix stack

// Allocate a stack with max size. Pushes the identity
// matrix to the bottom of the stack.
void mlInitMatrixStack(ml_matrixstack* stack, size_t size);

void mlDestroyMatrixStack(ml_matrixstack* stack);

// Push a copy of the top matrix to the stack
void mlPushMatrix(ml_matrixstack* stack);

// Assign the given matrix to the top of the stack
void mlLoadMatrix(ml_matrixstack* stack, ml_matrix* m);

// Push an identity matrix to the stack
void mlPushIdentity(ml_matrixstack* stack);

// Set the top of the stack to the identity
void mlLoadIdentity(ml_matrixstack* stack);

void mlPopMatrix(ml_matrixstack* stack);

ml_matrix* mlGetMatrix(ml_matrixstack* stack);

void mlLoadTexture2D(ml_tex2d* tex, const char* filename);
void mlDestroyTexture2D(ml_tex2d* tex);
void mlBindTexture2D(ml_tex2d* tex, int index);


static inline ml_clr
mlRGB(uint8_t r, uint8_t g, uint8_t b) {
	ml_clr c = { 0xff, r, g, b };
	return c;
}

static inline ml_clr
mlARGB(uint8_t a, uint8_t r, uint8_t g, uint8_t b) {
	ml_clr c = { a, r, g, b };
	return c;
}

static inline ml_clr
mlRGBA(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
	ml_clr c = { a, r, g, b };
	return c;
}

typedef struct ml_frustum {
	ml_vec4 planes[6];
	ml_vec4 absplanes[6];
} ml_frustum;

void
mlGetFrustum(ml_frustum* frustum, ml_matrix* projection, ml_matrix* view);

enum ml_CollisionResult {
	ML_OUTSIDE,
	ML_INSIDE,
	ML_INTERSECT
};

int mlTestFrustumAABB(ml_frustum* frustum, ml_vec3 center, ml_vec3 extent);

static inline int mlTestPlaneAABB(ml_vec4 plane, ml_vec3 center, ml_vec3 extent) {
	ml_vec4 absplane = { fabs(plane.x), fabs(plane.y), fabs(plane.z), fabs(plane.w) };
	float d = center.x * plane.x + center.y * plane.y + center.z * plane.z + plane.w;
	float r = extent.x * absplane.x + extent.y * absplane.y + extent.z * absplane.z + absplane.w;
	if (d + r <= 0) return ML_OUTSIDE;
	if (d - r < 0) return ML_INTERSECT;
	return ML_INSIDE;
}

static inline bool mlTestAABBAABB(ml_vec3 center1, ml_vec3 extent1, ml_vec3 center2, ml_vec3 extent2) {
	return (fabs(center1.x - center2.x) < extent1.x + extent2.x) &&
		(fabs(center1.y - center2.y) < extent1.y + extent2.y) &&
		(fabs(center1.z - center2.z) < extent1.z + extent2.z);
}

static inline bool mlTestPointAABB(ml_vec3 point, ml_vec3 center, ml_vec3 extent) {
	return (fabs(center.x - point.x) < extent.x) &&
		(fabs(center.y - point.y) < extent.y) &&
		(fabs(center.z - point.z) < extent.z);
}

/* based on code by Pierre Terdiman
   http://www.codercorner.com/RayAABB.cpp
 */
static inline bool mlTestRayAABB(ml_vec3 origin, ml_vec3 dir, ml_vec3 center, ml_vec3 extent) {
	ml_vec3 diff;

	diff.x = origin.x - center.x;
	if (fabsf(diff.x) > extent.x && diff.x*dir.x >= 0.0f)
		return false;

	diff.y = origin.y - center.y;
	if (fabsf(diff.y) > extent.y && diff.y*dir.y >= 0.0f)
		return false;

	diff.z = origin.z - center.z;
	if (fabsf(diff.z) > extent.z && diff.z*dir.z >= 0.0f)
		return false;

	ml_vec3 absdir = { fabsf(dir.x), fabsf(dir.y), fabsf(dir.z) };
	float f;
	f = dir.y * diff.z - dir.z * diff.y;
	if (fabsf(f) > extent.y*absdir.z + extent.z*absdir.y)
		return false;
	f = dir.z * diff.x - dir.x * diff.z;
	if (fabsf(f) > extent.x*absdir.z + extent.z*absdir.x)
		return false;
	f = dir.x * diff.y - dir.y * diff.x;
	if (fabsf(f) > extent.x*absdir.y + extent.y*absdir.x)
		return false;
	return true;
}

/* http://tog.acm.org/resources/GraphicsGems/gems/BoxSphere.c */
static inline bool mlTestSphereAABB(ml_vec3 pos, float radius, ml_vec3 center, ml_vec3 extent) {
	float dmin = 0;
	ml_vec3 bmin = { center.x - extent.x, center.y - extent.y, center.z - extent.z };
	ml_vec3 bmax = { center.x + extent.x, center.y + extent.y, center.z + extent.z };
	if (pos.x < bmin.x) dmin = pos.x - bmin.x;
	else if (pos.x > bmax.x) dmin = pos.x - bmax.x;
	if (dmin <= radius) return true;
	if (pos.y < bmin.y) dmin = pos.y - bmin.y;
	else if (pos.y > bmax.y) dmin = pos.y - bmax.y;
	if (dmin <= radius) return true;
	if (pos.z < bmin.z) dmin = pos.z - bmin.z;
	else if (pos.z > bmax.z) dmin = pos.z - bmax.z;
	if (dmin <= radius) return true;
	return false;
}

static inline bool mlTestSphereAABB_Hit(ml_vec3 pos, float radius, ml_vec3 center, ml_vec3 extent, ml_vec3* hit) {
	ml_vec3 sphereCenterRelBox;
	sphereCenterRelBox = mlVec3Sub(pos, center);
	// Point on surface of box that is closest to the center of the sphere
	ml_vec3 boxPoint;

	// Check sphere center against box along the X axis alone.
	// If the sphere is off past the left edge of the box,
	// then the left edge is closest to the sphere.
	// Similar if it's past the right edge. If it's between
	// the left and right edges, then the sphere's own X
	// is closest, because that makes the X distance 0,
	// and you can't get much closer than that :)

	if (sphereCenterRelBox.x < -extent.x) boxPoint.x = -extent.x;
	else if (sphereCenterRelBox.x > extent.x) boxPoint.x = extent.x;
	else boxPoint.x = sphereCenterRelBox.x;

	if (sphereCenterRelBox.y < -extent.y) boxPoint.y = -extent.y;
	else if (sphereCenterRelBox.y > extent.y) boxPoint.y = extent.y;
	else boxPoint.y = sphereCenterRelBox.y;

	if (sphereCenterRelBox.z < -extent.z) boxPoint.z = -extent.z;
	else if (sphereCenterRelBox.x > extent.z) boxPoint.z = extent.z;
	else boxPoint.z = sphereCenterRelBox.z;

	// Now we have the closest point on the box, so get the distance from
	// that to the sphere center, and see if it's less than the radius

	ml_vec3 dist = mlVec3Sub(sphereCenterRelBox, boxPoint);

	if (dist.x*dist.x + dist.y*dist.y + dist.z*dist.z < radius*radius) {
		*hit = boxPoint;
		return true;
	}
	return false;
}


// TODO: GL state stack - track state as a stack of uint64_ts...
