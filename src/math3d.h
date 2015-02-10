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

#define ML_DEG2RAD(d) (((d) * ML_PI) / 180.0)
#define ML_RAD2DEG(r) (((r) * 180.0) / ML_PI)

#ifdef _MSC_VER
#define ML_SWAP(a, b) do { a=(a+b) - (b=a); } while (0)
#else
#define ML_SWAP(a, b) do { __typeof__ (a) _swap_##__LINE__ = (a); (a) = (b); (b) = _swap_##__LINE__; } while (0)
#endif

enum ml_CollisionResult {
	ML_OUTSIDE,
	ML_INSIDE,
	ML_INTERSECT
};

enum ml_MeshFlags {
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

typedef struct frustum {
	vec4_t planes[6];
	vec4_t absplanes[6];
} frustum_t;

typedef struct aabb_t {
	vec3_t center;
	vec3_t extent;
} aabb_t;


#pragma pack(push, 4)

// order always has to be
// position
// normal
// texcoord
// color

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
typedef struct material {
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
} material_t;


// Revised concept of mesh and render object:
// A mesh is a render object - you need a VAO
// to render a mesh in GL 3.2+ core profile
// anyway.
// The mesh can be bound to different materials,
// and it needs to be bound to a material before
// it is rendered.
typedef struct mesh {
	material_t* material;
	GLuint vao;
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


typedef struct tex2d_t {
	GLuint id;
	uint16_t w;
	uint16_t h;
} tex2d_t;

typedef struct mtxstack {
	int top;
	mat44_t* stack;
} mtxstack_t;


// 3D math

void     m_perspective(mat44_t* m, float fovy, float aspect, float zNear, float zFar);
void     m_setidentity(mat44_t* m);
void     m_lookat(mat44_t* m, vec3_t eye, vec3_t at, vec3_t up);
void     m_copymat(mat44_t* to, const mat44_t* from);
void     m_fps_rotation(float pitch, float yaw, vec3_t* x, vec3_t* y, vec3_t* z);
void     m_fpsmatrix(mat44_t* to, vec3_t eye, float pitch, float yaw);
void     m_matmul(mat44_t* to, const mat44_t* by);
vec4_t   m_matmulvec(const mat44_t* m, const vec4_t* v);
vec3_t   m_matmulvec3(const mat44_t* m, const vec3_t* v);
vec3_t   m_rotatevec3(const mat44_t* m, const vec3_t* v);
void     m_translate(mat44_t* m, float x, float y, float z);
void     m_rotate(mat44_t* m, float angle, float x, float y, float z);
void     m_getmat33(mat33_t* to, const mat44_t* from);
vec3_t   m_matmul33(const mat33_t* m, const vec3_t* v);
void     m_transpose(mat44_t* m);
void     m_transpose33(mat33_t* m);
bool     m_invert(mat44_t* to, const mat44_t* from);
void     m_invert_orthonormal(mat44_t* to, const mat44_t* from);
void     m_makefrustum(frustum_t* frustum, mat44_t* projection, mat44_t* view);


// GL helpers

GLuint   m_compile_shader(GLenum type, const char* source);
GLuint   m_link_program(GLuint vsh, GLuint fsh);
void     m_create_material(material_t* material, const char* vsource, const char* fsource);
void     m_destroy_material(material_t* material);
void     m_create_mesh(mesh_t* mesh, size_t n, void* data, GLenum flags, GLenum usage);
void     m_create_indexed_mesh(mesh_t* mesh, size_t n, void* data, size_t ilen, GLenum indextype, void* indices, GLenum flags);
void     m_destroy_mesh(mesh_t* mesh);
void     m_update_mesh(mesh_t *mesh, GLintptr offset, GLsizeiptr n, const void* data);
void     m_replace_mesh(mesh_t *mesh, GLsizeiptr n, const void* data, GLenum usage);
void     m_set_material(mesh_t* mesh, material_t* material);
void     m_tex2d_load(tex2d_t* tex, const char* filename);
void     m_tex2d_destroy(tex2d_t* tex);
void     m_tex2d_bind(tex2d_t* tex, int index);
void     m_save_screenshot(const char* filename);


// Matrix stack

void     m_mtxstack_init(mtxstack_t* stack, size_t size);
void     m_mtxstack_destroy(mtxstack_t* stack);
void     m_pushmatrix(mtxstack_t* stack);
void     m_loadmatrix(mtxstack_t* stack, mat44_t* m);
void     m_pushidentity(mtxstack_t* stack);
void     m_loadidentity(mtxstack_t* stack);
void     m_popmatrix(mtxstack_t* stack);
mat44_t* m_getmatrix(mtxstack_t* stack);


// Collision routines

int      collide_frustum_aabb(frustum_t* frustum, vec3_t center, vec3_t extent);
int      collide_frustum_aabb_xz(frustum_t* frustum, vec3_t center, vec3_t extent);
int      collide_frustum_aabb_y(frustum_t* frustum, vec3_t center, vec3_t extent);
bool     collide_ray_aabb(vec3_t origin, vec3_t dir, vec3_t center, vec3_t extent);
bool     collide_sphere_aabb(vec3_t pos, float radius, vec3_t center, vec3_t extent);
bool     collide_sphere_aabb_full(vec3_t pos, float radius, vec3_t center, vec3_t extent, vec3_t* hit);
bool     collide_segment_aabb(vec3_t pos, vec3_t delta, vec3_t padding,
                              vec3_t center, vec3_t extent,
                              float* time, vec3_t* hit, vec3_t* hitdelta, vec3_t* normal);
bool     collide_aabb_aabb_full(vec3_t center, vec3_t extent,
                                vec3_t center2, vec3_t extent2, vec3_t *hitpoint, vec3_t *hitdelta, vec3_t *hitnormal);
bool     intersect_moving_aabb_aabb(aabb_t a, aabb_t b, vec3_t va, vec3_t vb, float* tfirst, float* tlast);


// inline functions

#if M_CHECKGL_ENABLED
#define M_CHECKGL(call) do { call; m_checkgl(__FILE__, __LINE__, #call); } while (0)
static inline
void m_checkgl(const char* file, int line, const char* call)
{
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
		static int checked = 0;
		if (err != GL_NO_ERROR && !checked) {
			checked = 1;
			fprintf(stderr, "GL error (%s:%d): (#%x) %s - %s\n", file, line, err, msg, call);
		}
	} while (err != GL_NO_ERROR);
}
#else
#define M_CHECKGL(call) call
#endif

static inline
bool m_fisvalid(float f)
{
	return (f >= -FLT_MAX && f <= FLT_MAX);
}

static inline
float m_sign(float x)
{
	return (x > 0) ? 1.f : (x < 0) ? -1.f : 0;
}

static inline
float m_clamp(float t, float lo, float hi)
{
	return (t < lo) ? lo : ((t > hi) ? hi : t);
}

static inline
double m_clampd(double t, double lo, double hi)
{
	return (t < lo) ? lo : ((t > hi) ? hi : t);
}

static inline
vec3_t m_clampVec3(vec3_t t, float lo, float hi)
{
	vec3_t ret = { m_clamp(t.x, lo, hi),
	                m_clamp(t.y, lo, hi),
	                m_clamp(t.z, lo, hi) };
	return ret;
}

static inline
float m_wrap(float t, float lo, float hi)
{
    while (t < lo)
	    t = hi - (lo - t);
    while (t > hi)
	    t = lo + (hi - t);
    return t;
}

static inline
vec2_t m_vec2(float x, float y)
{
	vec2_t v = { x, y };
	return v;
}

static inline
vec3_t m_vec3(float x, float y, float z)
{
	vec3_t v = { x, y, z };
	return v;
}

static inline
vec4_t m_vec4(float x, float y, float z, float w)
{
	vec4_t v = { x, y, z, w };
	return v;
}

static inline
dvec3_t m_dvec3(double x, double y, double z)
{
	dvec3_t v = { x, y, z };
	return v;
}

static inline
void m_uniform_mat44(GLint index, mat44_t* mat)
{
	if (index != -1)
		glUniformMatrix4fv(index, 1, GL_FALSE, mat->m);
}

static inline
void m_uniform_mat33(GLint index, mat33_t* mat)
{
	if (index != -1)
		glUniformMatrix3fv(index, 1, GL_FALSE, mat->m);
}

static inline
void m_uniform_vec2(GLint index, vec2_t* v)
{
	if (index != -1)
		glUniform2fv(index, 1, (GLfloat*)v);
}

static inline
void m_uniform_vec3(GLint index, vec3_t* v)
{
	if (index != -1)
		glUniform3fv(index, 1, (GLfloat*)v);
}

static inline
void m_uniform_vec4(GLint index, vec4_t* v)
{
	if (index != -1)
		glUniform4fv(index, 1, (GLfloat*)v);
}

static inline
void m_uniform_i(GLint index, int i)
{
	if (index != -1)
		glUniform1i(index, i);
}

static inline
void m_use(material_t* material)
{
	if (material != NULL)
		M_CHECKGL(glUseProgram(material->program));
	else
		M_CHECKGL(glUseProgram(0));
}

static inline
void m_draw(const mesh_t* mesh)
{
	M_CHECKGL(glBindVertexArray(mesh->vao));
	if (mesh->ibo > 0)
		M_CHECKGL(glDrawElements(mesh->mode, mesh->count, mesh->ibotype, 0));
	else
		M_CHECKGL(glDrawArrays(mesh->mode, 0, mesh->count));
	glBindVertexArray(0);
}


static inline
void m_draw_range(const mesh_t* mesh, GLint first, GLsizei count)
{
	M_CHECKGL(glBindVertexArray(mesh->vao));
	if (mesh->ibo > 0)
		M_CHECKGL(glDrawElements(mesh->mode, mesh->count, mesh->ibotype, 0));
	else
		M_CHECKGL(glDrawArrays(mesh->mode, first, count));
	glBindVertexArray(0);
}


static inline
vec3_t m_xaxis44(const mat44_t* from)
{
	return *(vec3_t*)&from->m[0];
}

static inline
vec3_t m_yaxis44(const mat44_t* from)
{
	return *(vec3_t*)&from->m[4];
}

static inline
vec3_t m_zaxis44(const mat44_t* from)
{
	return *(vec3_t*)&from->m[8];
}


#define m_setvec2(v, a, b) { (v).x = (a); (v).y = (b); }
#define m_setvec3(v, a, b, c) { (v).x = (a); (v).y = (b); (v).z = (c); }
#define m_setvec4(v, a, b, c, d) { (v).x = (a); (v).y = (b); (v).z = (c); (v).w = (d); }

static inline
float m_vec3dot(const vec3_t a, const vec3_t b)
{
	return a.x * b.x + a.y * b.y + a.z * b.z;
}

static inline
vec3_t m_vec3cross(const vec3_t a, const vec3_t b)
{
	vec3_t to;
	to.x = a.y*b.z - a.z*b.y;
	to.y = a.z*b.x - a.x*b.z;
	to.z = a.x*b.y - a.y*b.x;
	return to;
}

static inline
vec2_t m_vec2add(const vec2_t a, const vec2_t b)
{
	vec2_t to = { a.x + b.x, a.y + b.y };
	return to;
}

static inline
vec2_t m_vec2addf(vec2_t tc, float by)
{
	tc.x += by;
	tc.y += by;
	return tc;
}

static inline
vec3_t m_vec3add(const vec3_t a, const vec3_t b)
{
	vec3_t to = { a.x + b.x, a.y + b.y, a.z + b.z };
	return to;
}

static inline
vec4_t m_vec4add(const vec4_t a, const vec4_t b)
{
	vec4_t to = { a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w };
	return to;
}

static inline
vec3_t m_vec3sub(const vec3_t a, const vec3_t b)
{
	vec3_t to = { a.x - b.x, a.y - b.y, a.z - b.z };
	return to;
}

static inline
vec3_t m_vec3scale(const vec3_t a, float f)
{
	vec3_t to = { a.x * f, a.y * f, a.z * f };
	return to;
}

static inline
vec4_t m_vec4scale(const vec4_t a, float f)
{
	vec4_t to = { a.x * f, a.y * f, a.z * f, a.w * f };
	return to;
}

static inline
float m_vec3len2(const vec3_t v)
{
	return v.x*v.x + v.y*v.y + v.z*v.z;
}

static inline
float m_vec3len(const vec3_t v)
{
	return sqrtf(v.x*v.x + v.y*v.y + v.z*v.z);
}

static inline
vec3_t m_vec3normalize(vec3_t v)
{
	float invlen = 1.f / m_vec3len(v);
	return m_vec3scale(v, invlen);
}

static inline
vec3_t m_vec3invert(const vec3_t v)
{
	vec3_t to = { -v.x, -v.y, -v.z };
	return to;
}

static inline
 vec4_t m_vec4abs(const vec4_t v)
{
	vec4_t to = { fabs(v.x), fabs(v.y), fabs(v.z), fabs(v.w) };
	return to;
}

static inline
float m_fastinvsqrt(float x) {
	union {
		float f;
		int bits;
	} v;
	float xhalf = 0.5f * x;
	v.f = x;
	v.bits = 0x5f3759df - (v.bits >> 1);  // what the fuck?
	x = v.f;
	x = x*(1.5f-(xhalf*x*x));
	return x;
}

static inline
vec3_t m_vec3fastnormalize(vec3_t v)
{
	return m_vec3scale(v, m_fastinvsqrt(v.x*v.x + v.y*v.y + v.z*v.z));
}

static inline
vec4_t m_normalize_plane(vec4_t plane)
{
	vec3_t n = {plane.x, plane.y, plane.z};
	float len = m_vec3len(n);
	n = m_vec3scale(n, 1.f / len);
	plane.x = n.x;
	plane.y = n.y;
	plane.z = n.z;
	plane.w = plane.w / len;
	return plane;
}

static inline
vec3_t m_dvec3tovec3(dvec3_t v)
{
	return m_vec3(v.x, v.y, v.z);
}

static inline
dvec3_t m_dvec3add(dvec3_t a, dvec3_t b)
{
	return m_dvec3(a.x + b.x, a.y + b.y, a.z + b.z);
}

static inline
dvec3_t m_dvec3sub(dvec3_t a, dvec3_t b)
{
	return m_dvec3(a.x - b.x, a.y - b.y, a.z - b.z);
}

static inline
clr_t m_makergb(uint8_t r, uint8_t g, uint8_t b)
{
	clr_t c = { 0xff, r, g, b };
	return c;
}

static inline
clr_t m_makeargb(uint8_t a, uint8_t r, uint8_t g, uint8_t b)
{
	clr_t c = { a, r, g, b };
	return c;
}

static inline
clr_t m_makergba(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
	clr_t c = { a, r, g, b };
	return c;
}

static inline
int collide_plane_aabb(vec4_t plane, vec3_t center, vec3_t extent)
{
	vec4_t absplane = { fabs(plane.x), fabs(plane.y), fabs(plane.z), fabs(plane.w) };
	float d = center.x * plane.x + center.y * plane.y + center.z * plane.z + plane.w;
	float r = extent.x * absplane.x + extent.y * absplane.y + extent.z * absplane.z + absplane.w;
	if (d + r <= 0) return ML_OUTSIDE;
	if (d - r < 0) return ML_INTERSECT;
	return ML_INSIDE;
}

static inline
bool collide_aabb_aabb(vec3_t center1, vec3_t extent1, vec3_t center2, vec3_t extent2)
{
	return (fabs(center1.x - center2.x) < extent1.x + extent2.x) &&
		(fabs(center1.y - center2.y) < extent1.y + extent2.y) &&
		(fabs(center1.z - center2.z) < extent1.z + extent2.z);
}

static inline
bool collide_point_aabb(vec3_t point, vec3_t center, vec3_t extent)
{
	return (fabs(center.x - point.x) < extent.x) &&
		(fabs(center.y - point.y) < extent.y) &&
		(fabs(center.z - point.z) < extent.z);
}



// constants

extern const vec3_t m_up;
extern const vec3_t m_right;
extern const vec3_t m_forward;


// TODO: GL state stack - track state as a stack of uint64_ts...
