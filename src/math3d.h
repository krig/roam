#pragma once

#include <stdbool.h>

#define ML_PI 3.14159265358979323846	/* pi */
#define ML_TWO_PI (3.14159265358979323846 * 2.0)
#define ML_EPSILON (1.0e-4f)
#define ML_EPSILON2 (1.0e-5f)
#define ML_E 2.71828182845904523536
#define ML_LOG2E 1.44269504088896340736
#define ML_PI_2 1.57079632679489661923
#define ML_PI_4 0.785398163397448309616

typedef union ml_vec2 {
	float v[2];
	struct {
		float x, y;
	};
} ml_vec2;

typedef union ml_vec3 {
	float v[3];
	struct {
		float x, y, z;
	};
} ml_vec3;

typedef union ml_vec4 {
	float v[4];
	struct {
		float x, y, z, w;
	};
} ml_vec4;

typedef union ml_clr {
	uint32_t argb;
	struct {
		uint8_t a, r, g, b;
	};
} ml_clr;


typedef struct ml_matrix {
	float m[16];
} ml_matrix;

#pragma pack(push, 4)

typedef struct ml_vtx_pos_clr {
	ml_vec3 pos;
	uint32_t clr;
} ml_vtx_pos_clr;

typedef struct ml_vtx_pos_clr_n {
	ml_vec3 pos;
	uint32_t clr;
	ml_vec3 n;
} ml_vtx_pos_clr_n;

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
	GLint position;
	GLint color;
	GLint normal;
} ml_material;


// A mesh is a VBO plus metadata
// that describes how it maps to
// a material
typedef struct ml_mesh {
	GLuint vbo;
	GLint position; // -1 if not present, else offset
	GLint color;
	GLint normal;
	GLsizei stride;
	GLenum mode;
	GLsizei count;
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


#define mlDeg2Rad(d) (((d) * ML_PI) / 180.f)
#define mlRad2Deg(r) (((r) * 180.f) / ML_PI)
#define mlSwap(a, b) do { __typeof__ (a) _swap_##__LINE__ = (a); (a) = (b); (b) = _swap_##__LINE__; } while (0)
#define mlMax(a, b) do { __typeof__ (a) _a = (a); __typeof__ (b) _b = (b); _a > _b ? _a : _b; } while(0)
#define mlMin(a, b) do { __typeof__ (a) _a = (a); __typeof__ (b) _b = (b); _a > _b ? _b : _a; } while(0)

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

static inline float
mlWrap(float t, float lo, float hi) {
    while (t < lo)
	    t = hi - (lo - t);
    while (t > hi)
	    t = lo + (hi - t);
    return t;
}


#define mlVec3Assign(v, a, b, c) { (v).x = (a); (v).y = (b); (v).z = (c); }

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

void mlTranslate(ml_matrix* m, float x, float y, float z);
void mlRotate(ml_matrix* m, float angle, float x, float y, float z);

void mlTranspose(ml_matrix* m);

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

static inline float
mlVec3Length2(const ml_vec3 v) {
	return v.x*v.x + v.y*v.y + v.z*v.z;
}

static inline float
mlVec3Length(const ml_vec3 v) {
	return sqrtf(v.x*v.x + v.y*v.y + v.z*v.z);
}

static inline void
mlVec3Normalize(ml_vec3* v) {
	float invlen = 1.f / mlVec3Length(*v);
	v->x *= invlen;
	v->y *= invlen;
	v->z *= invlen;
}

static inline ml_vec3
mlVec3Invert(const ml_vec3 v) {
	ml_vec3 to = { -v.x, -v.y, -v.z };
	return to;
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
mlBindProjection(ml_renderable* renderable, ml_matrix* projection) {
	GLint index = renderable->material->projmat;
	if (index != -1)
		glUniformMatrix4fv(index, 1, GL_FALSE, projection->m);
}

static inline void
mlBindModelView(ml_renderable* renderable, ml_matrix* modelview) {
	GLint index = renderable->material->modelview;
	if (index != -1)
		glUniformMatrix4fv(index, 1, GL_FALSE, modelview->m);
}

static inline void
mlDrawBegin(ml_renderable* renderable) {
	glUseProgram(renderable->material->program);
	glBindVertexArray(renderable->vao);
}

static inline void
mlDrawEnd(ml_renderable* renderable) {
	glDrawArrays(renderable->mesh->mode, 0, renderable->mesh->count);
	glBindVertexArray(0);
	glUseProgram(0);
}

GLuint mlCompileShader(GLenum type, const char* source);

GLuint mlLinkProgram(GLuint vsh, GLuint fsh);

void mlCreateMaterial(ml_material* material, const char* vsource, const char* fsource);

void mlCreateMesh(ml_mesh* mesh, size_t n, ml_vtx_pos_clr* data);

void mlCreateRenderable(ml_renderable* renderable, const ml_material* material, const ml_mesh* mesh);

// Matrix stack

// Allocate a stack with max size. Pushes the identity
// matrix to the bottom of the stack.
void mlInitMatrixStack(ml_matrixstack* stack, size_t size);

void mlFreeMatrixStack(ml_matrixstack* stack);

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
void mlFreeTexture2D(ml_tex2d* tex);
void mlBindTexture2D(ml_tex2d* tex, int index);


// TODO: GL state stack - track state as a stack of uint64_ts...
