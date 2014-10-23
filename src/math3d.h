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

#define DEG2RAD(d) (((d) * ML_PI) / 180.0)
#define RAD2DEG(r) (((r) * 180.0) / ML_PI)
#define SWAP(a, b) do { __typeof__ (a) _swap_##__LINE__ = (a); (a) = (b); (b) = _swap_##__LINE__; } while (0)

typedef struct vec2 {
	float x, y;
} vec2_t;

typedef struct vec3 {
	float x, y, z;
} vec3_t;

typedef struct vec4 {
	float x, y, z, w;
} vec4_t;

typedef struct clr {
	uint8_t a, r, g, b;
} clr_t;

typedef struct ivec2 {
	int x, y;
} ivec2_t;

typedef struct ivec3 {
	int x, y, z;
} ivec3_t;

typedef struct ivec4 {
	int x, y, z, w;
} ivec4_t;

typedef struct dvec3 {
	double x, y, z;
} dvec3_t;

typedef struct chunkpos {
	int x, z;
} chunkpos_t;

typedef struct mat44 {
	float m[16];
} mat44_t;

typedef struct mat33 {
	float m[9];
} mat33_t;


#pragma pack(push, 4)

typedef struct posclrvert {
	vec3_t pos;
	uint32_t clr;
} posclrvert_t;

typedef struct posnormalclrvert {
	vec3_t pos;
	vec3_t n;
	uint32_t clr;
} posnormalclrvert_t;

typedef struct posnormalvert {
	vec3_t pos;
	vec3_t n;
} posnormalvert_t;

typedef struct uivert {
	vec2_t pos;
	vec2_t tc;
	uint32_t clr;
} uivert_t;

#pragma pack(pop)

// A material identifies a
// shader and any special
// considerations concerning
// that shader
typedef struct material_t {
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
} material_t;


// A mesh is a VBO plus metadata
// that describes how it maps to
// a material
typedef struct mesh_t {
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
} mesh_t;

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
	const material_t* material;
	const mesh_t* mesh;
	GLuint vao;
} ml_renderable;

typedef struct mtxstack_t {
	int top;
	mat44_t* stack;
} mtxstack_t;

enum ml_CollisionResult {
	ML_OUTSIDE,
	ML_INSIDE,
	ML_INTERSECT
};

typedef struct frustum_t {
	vec4_t planes[6];
	vec4_t absplanes[6];
} frustum_t;

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

static inline vec3_t
mlClampVec3(vec3_t t, float lo, float hi) {
	vec3_t ret = { mlClamp(t.x, lo, hi),
	                mlClamp(t.y, lo, hi),
	                mlClamp(t.z, lo, hi) };
	return ret;
}

static inline float
mlWrap(float t, float lo, float hi) {
    while (t < lo)
	    t = hi - (lo - t);
    while (t > hi)
	    t = lo + (hi - t);
    return t;
}

static inline vec2_t
mlMakeVec2(float x, float y) {
	vec2_t v = { x, y };
	return v;
}

static inline vec3_t
mlMakeVec3(float x, float y, float z) {
	vec3_t v = { x, y, z };
	return v;
}

static inline vec4_t
mlMakeVec4(float x, float y, float z, float w) {
	vec4_t v = { x, y, z, w };
	return v;
}

#define mlVec2Assign(v, a, b) { (v).x = (a); (v).y = (b); }
#define mlVec3Assign(v, a, b, c) { (v).x = (a); (v).y = (b); (v).z = (c); }
#define mlVec4Assign(v, a, b, c, d) { (v).x = (a); (v).y = (b); (v).z = (c); (v).w = (d); }

void mlPerspective(mat44_t* m, float fovy, float aspect, float zNear, float zFar);

void mlSetIdentity(mat44_t* m);

void mlLookAt(mat44_t* m,
              float eyeX, float eyeY, float eyeZ,
              float atX, float atY, float atZ,
              float upX, float upY, float upZ);

void mlCopyMatrix(mat44_t* to, const mat44_t* from);

void mlFPSRotation(float pitch, float yaw, vec3_t* x, vec3_t* y, vec3_t* z);
void mlFPSMatrix(mat44_t* to, vec3_t eye, float pitch, float yaw);

void mlMulMatrix(mat44_t* to, const mat44_t* by);

vec4_t mlMulMatVec(const mat44_t* m, const vec4_t* v);
vec3_t mlMulMatVec3(const mat44_t* m, const vec3_t* v);
vec3_t mlVec3RotateBy(const mat44_t* m, const vec3_t* v);

void mlTranslate(mat44_t* m, float x, float y, float z);
void mlRotate(mat44_t* m, float angle, float x, float y, float z);

void mlGetRotationMatrix(mat33_t* to, const mat44_t* from);
vec3_t mlMulMat33Vec3(const mat33_t* m, const vec3_t* v);

void mlTranspose(mat44_t* m);
void mlTranspose33(mat33_t* m);
bool mlInvertMatrix(mat44_t* to, const mat44_t* from);
void mlInvertOrthoMatrix(mat44_t* to, const mat44_t* from);

static inline vec3_t mlGetXAxis(const mat44_t* from) {
	return *(vec3_t*)&from->m[0];
}

static inline vec3_t mlGetYAxis(const mat44_t* from) {
	return *(vec3_t*)&from->m[4];
}

static inline vec3_t mlGetZAxis(const mat44_t* from) {
	return *(vec3_t*)&from->m[8];
}


static inline float
mlVec3Dot(const vec3_t a, const vec3_t b) {
	return a.x * b.x + a.y * b.y + a.z * b.z;
}

static inline vec3_t
mlVec3Cross(const vec3_t a, const vec3_t b) {
	vec3_t to;
	to.x = a.y*b.z - a.z*b.y;
	to.y = a.z*b.x - a.x*b.z;
	to.z = a.x*b.y - a.y*b.x;
	return to;
}

static inline vec2_t
mlVec2Add(const vec2_t a, const vec2_t b) {
	vec2_t to = { a.x + b.x, a.y + b.y };
	return to;
}

static inline vec2_t
mlVec2AddScalar(vec2_t tc, float by) {
	tc.x += by;
	tc.y += by;
	return tc;
}

static inline vec3_t
mlVec3Add(const vec3_t a, const vec3_t b) {
	vec3_t to = { a.x + b.x, a.y + b.y, a.z + b.z };
	return to;
}

static inline vec4_t
mlVec4Add(const vec4_t a, const vec4_t b) {
	vec4_t to = { a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w };
	return to;
}

static inline vec3_t
mlVec3Sub(const vec3_t a, const vec3_t b) {
	vec3_t to = { a.x - b.x, a.y - b.y, a.z - b.z };
	return to;
}

static inline vec3_t
mlVec3Scalef(const vec3_t a, float f) {
	vec3_t to = { a.x * f, a.y * f, a.z * f };
	return to;
}

static inline vec4_t
mlVec4Scalef(const vec4_t a, float f) {
	vec4_t to = { a.x * f, a.y * f, a.z * f, a.w * f };
	return to;
}

static inline float
mlVec3Length2(const vec3_t v) {
	return v.x*v.x + v.y*v.y + v.z*v.z;
}

static inline float
mlVec3Length(const vec3_t v) {
	return sqrtf(v.x*v.x + v.y*v.y + v.z*v.z);
}

static inline vec3_t
mlVec3Normalize(vec3_t v) {
	float invlen = 1.f / mlVec3Length(v);
	return mlVec3Scalef(v, invlen);
}

static inline vec3_t
mlVec3Invert(const vec3_t v) {
	vec3_t to = { -v.x, -v.y, -v.z };
	return to;
}

static inline vec4_t
mlVec4Abs(const vec4_t v) {
	vec4_t to = { fabs(v.x), fabs(v.y), fabs(v.z), fabs(v.w) };
	return to;
}

static inline vec4_t
mlNormalizePlane(vec4_t plane) {
	vec3_t n = {plane.x, plane.y, plane.z};
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
mlUniformMatrix(GLint index, mat44_t* mat) {
	if (index != -1)
		glUniformMatrix4fv(index, 1, GL_FALSE, mat->m);
}

static inline void
mlUniformMatrix33(GLint index, mat33_t* mat) {
	if (index != -1)
		glUniformMatrix3fv(index, 1, GL_FALSE, mat->m);
}

static inline void
mlUniformVec2(GLint index, vec2_t* v) {
	if (index != -1)
		glUniform2fv(index, 1, (GLfloat*)v);
}

static inline void
mlUniformVec3(GLint index, vec3_t* v) {
	if (index != -1)
		glUniform3fv(index, 1, (GLfloat*)v);
}

static inline void
mlUniformVec4(GLint index, vec4_t* v) {
	if (index != -1)
		glUniform4fv(index, 1, (GLfloat*)v);
}

static inline void
mlUniform1i(GLint index, int i) {
	if (index != -1)
		glUniform1i(index, i);
}

static inline void
mlBindMatrix(GLint index, mat44_t* mat) {
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
	const mesh_t* mesh = renderable->mesh;
	if (mesh->ibo > 0)
		glDrawElements(mesh->mode, mesh->count, mesh->ibotype, 0);
	else
		glDrawArrays(mesh->mode, 0, mesh->count);
	glBindVertexArray(0);
	glUseProgram(0);
}

GLuint mlCompileShader(GLenum type, const char* source);

GLuint mlLinkProgram(GLuint vsh, GLuint fsh);

void mlCreateMaterial(material_t* material, const char* vsource, const char* fsource);
void mlDestroyMaterial(material_t* material);

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

void mlCreateMesh(mesh_t* mesh, size_t n, void* data, GLenum flags, GLenum usage);
void mlCreateIndexedMesh(mesh_t* mesh, size_t n, void* data, size_t ilen, GLenum indextype, void* indices, GLenum flags);
void mlDestroyMesh(mesh_t* mesh);
void mlUpdateMesh(mesh_t *mesh, GLintptr offset, GLsizeiptr n, void* data);

void mlCreateRenderable(ml_renderable* renderable, const material_t* material, const mesh_t* mesh);
void mlDestroyRenderable(ml_renderable* renderable);
void mlMapMeshToMaterial(const mesh_t* mesh, const material_t* material);

// Matrix stack

// Allocate a stack with max size. Pushes the identity
// matrix to the bottom of the stack.
void mlInitMatrixStack(mtxstack_t* stack, size_t size);

void mlDestroyMatrixStack(mtxstack_t* stack);

// Push a copy of the top matrix to the stack
void mlPushMatrix(mtxstack_t* stack);

// Assign the given matrix to the top of the stack
void mlLoadMatrix(mtxstack_t* stack, mat44_t* m);

// Push an identity matrix to the stack
void mlPushIdentity(mtxstack_t* stack);

// Set the top of the stack to the identity
void mlLoadIdentity(mtxstack_t* stack);

void mlPopMatrix(mtxstack_t* stack);

mat44_t* mlGetMatrix(mtxstack_t* stack);

void mlLoadTexture2D(ml_tex2d* tex, const char* filename);
void mlDestroyTexture2D(ml_tex2d* tex);
void mlBindTexture2D(ml_tex2d* tex, int index);

static inline
clr_t mlRGB(uint8_t r, uint8_t g, uint8_t b)
{
	clr_t c = { 0xff, r, g, b };
	return c;
}

static inline
clr_t mlARGB(uint8_t a, uint8_t r, uint8_t g, uint8_t b)
{
	clr_t c = { a, r, g, b };
	return c;
}

static inline
clr_t mlRGBA(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
	clr_t c = { a, r, g, b };
	return c;
}

void mlGetFrustum(frustum_t* frustum, mat44_t* projection, mat44_t* view);

int mlTestFrustumAABB(frustum_t* frustum, vec3_t center, vec3_t extent);
int mlTestFrustumAABB_XZ(frustum_t* frustum, vec3_t center, vec3_t extent);
int mlTestFrustumAABB_Y(frustum_t* frustum, vec3_t center, vec3_t extent);
bool mlTestRayAABB(vec3_t origin, vec3_t dir, vec3_t center, vec3_t extent);
bool mlTestSphereAABB(vec3_t pos, float radius, vec3_t center, vec3_t extent);
bool mlTestSphereAABB_Hit(vec3_t pos, float radius, vec3_t center, vec3_t extent, vec3_t* hit);

bool mlTestSegmentAABB(vec3_t pos, vec3_t delta, vec3_t padding, vec3_t center, vec3_t extent,
	float* time, vec3_t* hit, vec3_t* hitdelta, vec3_t* normal);
bool mlTestAABBAABB_2(vec3_t center, vec3_t extent, vec3_t center2, vec3_t extent2, vec3_t *hitpoint, vec3_t *hitdelta, vec3_t *hitnormal);

static inline int mlTestPlaneAABB(vec4_t plane, vec3_t center, vec3_t extent)
{
	vec4_t absplane = { fabs(plane.x), fabs(plane.y), fabs(plane.z), fabs(plane.w) };
	float d = center.x * plane.x + center.y * plane.y + center.z * plane.z + plane.w;
	float r = extent.x * absplane.x + extent.y * absplane.y + extent.z * absplane.z + absplane.w;
	if (d + r <= 0) return ML_OUTSIDE;
	if (d - r < 0) return ML_INTERSECT;
	return ML_INSIDE;
}

static inline bool mlTestAABBAABB(vec3_t center1, vec3_t extent1, vec3_t center2, vec3_t extent2)
{
	return (fabs(center1.x - center2.x) < extent1.x + extent2.x) &&
		(fabs(center1.y - center2.y) < extent1.y + extent2.y) &&
		(fabs(center1.z - center2.z) < extent1.z + extent2.z);
}

static inline bool mlTestPointAABB(vec3_t point, vec3_t center, vec3_t extent)
{
	return (fabs(center.x - point.x) < extent.x) &&
		(fabs(center.y - point.y) < extent.y) &&
		(fabs(center.z - point.z) < extent.z);
}

typedef struct aabb_t {
	vec3_t center;
	vec3_t extent;
} aabb_t;

bool intersect_moving_aabb_aabb(aabb_t a, aabb_t b, vec3_t va, vec3_t vb, float* tfirst, float* tlast);

// TODO: GL state stack - track state as a stack of uint64_ts...
