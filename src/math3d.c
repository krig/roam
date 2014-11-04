#include "common.h"
#include "math3d.h"
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image.h"
#include "stb_image_write.h"

const vec3_t m_up = {0, 1, 0};
const vec3_t m_right = {1, 0, 0};
const vec3_t m_forward = {0, 0, -1};


void m_perspective(mat44_t* m, float fovy, float aspect, float zNear, float zFar)
{
	m_setidentity(m);
	float r = fovy * 0.5f;
	float f = cos(r) / sin(r);
	m->m[0] = f / aspect;
	m->m[5] = f;
	m->m[10] = -(zFar + zNear) / (zFar - zNear);
	m->m[11] = -1.f;
	m->m[14] = (-2.f * zFar * zNear) / (zFar - zNear);
	m->m[15] = 0;
}

void m_setidentity(mat44_t* m)
{
	memset(m->m, 0, sizeof(float)*16);
	m->m[0] = m->m[5] = m->m[10] = m->m[15] = 1.f;
}

void m_fps_rotation(float pitch, float yaw, vec3_t* x, vec3_t* y, vec3_t* z)
{
	float cosPitch = cosf(pitch);
	float sinPitch = sinf(pitch);
	float cosYaw = cosf(yaw);
	float sinYaw = sinf(yaw);
	x->x = cosYaw;
	x->y = 0;
	x->z = -sinYaw;
	y->x = sinYaw * sinPitch;
	y->y = cosPitch;
	y->z = cosYaw * sinPitch;
	z->x = sinYaw * cosPitch;
	z->y = -sinPitch;
	z->z = cosPitch * cosYaw;
}

void m_fpsmatrix(mat44_t* to, vec3_t eye, float pitch, float yaw)
{
	/* equivalent to:
	   m_setidentity(to);
	   m_rotate(to, -pitch, 1.f, 0, 0);
	   m_rotate(to, -yaw, 0, 1.f, 0);
	   m_translate(to, -eye.x, -eye.y, -eye.z);
	*/
	float cosPitch = cosf(pitch);
	float sinPitch = sinf(pitch);
	float cosYaw = cosf(yaw);
	float sinYaw = sinf(yaw);
	vec3_t xaxis = { cosYaw, 0, -sinYaw };
	vec3_t yaxis = { sinYaw * sinPitch, cosPitch, cosYaw * sinPitch };
	vec3_t zaxis = { sinYaw * cosPitch, -sinPitch, cosPitch * cosYaw };
	to->m[0] = xaxis.x;
	to->m[1] = yaxis.x;
	to->m[2] = zaxis.x;
	to->m[3] = 0;
	to->m[4] = xaxis.y;
	to->m[5] = yaxis.y;
	to->m[6] = zaxis.y;
	to->m[7] = 0;
	to->m[8] = xaxis.z;
	to->m[9] = yaxis.z;
	to->m[10] = zaxis.z;
	to->m[11] = 0;
	to->m[12] = -m_vec3dot(xaxis, eye);
	to->m[13] = -m_vec3dot(yaxis, eye);
	to->m[14] = -m_vec3dot(zaxis, eye);
	to->m[15] = 1.f;
}


void m_lookat(mat44_t* m, vec3_t eye, vec3_t at, vec3_t up)
{
	mat44_t M;
	vec3_t f, u, s;

	m_setvec3(f, at.x - eye.x, at.y - eye.y, at.z - eye.z);
	f = m_vec3normalize(f);

	// check for when we're looking straight up or down Y
	if (fabsf(f.x) < ML_EPSILON && fabsf(f.z) < ML_EPSILON && fabsf(up.x) < ML_EPSILON && fabsf(up.z) < ML_EPSILON)
		m_setvec3(up, 0, 0, (f.y < 0) ? -1 : 1);

	m_setvec3(u, up.x, up.y, up.z);
	s = m_vec3normalize(m_vec3cross(f, u));
	u = m_vec3normalize(m_vec3cross(s, f));

	M.m[0] = s.x;
	M.m[4] = s.y;
	M.m[8] = s.z;
	M.m[1] = u.x;
	M.m[5] = u.y;
	M.m[9] = u.z;
	M.m[2] = -f.x;
	M.m[6] = -f.y;
	M.m[10] = -f.z;
	M.m[3] = M.m[7] = M.m[11] = M.m[12] = M.m[13] = M.m[14] = 0;
	M.m[15] = 1.f;
	m_copymat(m, &M);
	m_translate(m, -eye.x, -eye.y, -eye.z);
}

void m_copymat(mat44_t* to, const mat44_t* from)
{
	memcpy(to, from, sizeof(mat44_t));
}

void m_getmat33(mat33_t* to, const mat44_t* from)
{
	float*__restrict__ a = to->m;
	const float*__restrict__ b = from->m;
	a[0] = b[0];
	a[1] = b[1];
	a[2] = b[2];
	a[3] = b[4];
	a[4] = b[5];
	a[5] = b[6];
	a[6] = b[8];
	a[7] = b[9];
	a[8] = b[10];
}

void
m_makefrustum(frustum_t* frustum, mat44_t* projection, mat44_t* view)
{
	mat44_t MP;
	m_copymat(&MP, projection);
	m_matmul(&MP, view);
	vec4_t*__restrict__ p = frustum->planes;
	vec4_t*__restrict__ a = frustum->absplanes;
	float* m = MP.m;
	// right
	m_setvec4(p[0], m[3] - m[0], m[7] - m[4], m[11] - m[8], m[15] - m[12]);
	// left
	m_setvec4(p[1], m[3] + m[0], m[7] + m[4], m[11] + m[8], m[15] + m[12]);
	// down
	m_setvec4(p[2], m[3] + m[1], m[7] + m[5], m[11] + m[9], m[15] + m[13]);
	// up
	m_setvec4(p[3], m[3] - m[1], m[7] - m[5], m[11] - m[9], m[15] - m[13]);
	// far
	m_setvec4(p[4], m[3] - m[2], m[7] - m[6], m[11] - m[10], m[15] - m[14]);
	// near
	m_setvec4(p[5], m[3] + m[2], m[7] + m[6], m[11] + m[10], m[15] + m[14]);
	for (int i = 0; i < 6; ++i) {
		p[i] = m_normalize_plane(p[i]);
		a[i] = m_vec4abs(p[i]);
	}
}

// center: (max + min)/2
// extents: (max - min)/2
int collide_frustum_aabb(frustum_t* frustum, vec3_t center, vec3_t extent)
{
	int result, i;
	vec4_t p, a;
	result = ML_INSIDE;
	for (i = 0; i < 6; ++i) {
		p = frustum->planes[i];
		a = frustum->absplanes[i];

		float d = center.x * p.x + center.y * p.y + center.z * p.z + p.w;
		float r = extent.x * a.x + extent.y * a.y + extent.z * a.z + a.w;
		if (d + r <= 0) return ML_OUTSIDE;
		if (d - r < 0) result = ML_INTERSECT;
	}
	return result;
}

// these are very voxelgame-specific
int collide_frustum_aabb_xz(frustum_t* frustum, vec3_t center, vec3_t extent)
{
	int result, i;
	vec4_t p, a;
	result = ML_INSIDE;
	for (i = 0; i < 6; ++i) {
		if (i == 2 || i == 3) continue;
		p = frustum->planes[i];
		a = frustum->absplanes[i];
		float d = center.x * p.x + center.y * p.y + center.z * p.z + p.w;
		float r = extent.x * a.x + extent.y * a.y + extent.z * a.z + a.w;
		if (d + r <= 0) return ML_OUTSIDE;
		if (d - r < 0) result = ML_INTERSECT;
	}
	return result;
}

int collide_frustum_aabb_y(frustum_t* frustum, vec3_t center, vec3_t extent)
{
	int result, i;
	vec4_t p, a;
	result = ML_INSIDE;
	for (i = 2; i < 4; ++i) {
		p = frustum->planes[i];
		a = frustum->absplanes[i];
		float d = center.x * p.x + center.y * p.y + center.z * p.z + p.w;
		float r = extent.x * a.x + extent.y * a.y + extent.z * a.z + a.w;
		if (d + r <= 0) return ML_OUTSIDE;
		if (d - r < 0) result = ML_INTERSECT;
	}
	return result;
}


vec3_t m_rotatevec3(const mat44_t* m, const vec3_t *v)
{
	vec3_t ret;
	ret.x = (v->x * m->m[0]) +
		(v->y * m->m[4]) +
		(v->z * m->m[8]);
	ret.y = (v->x * m->m[1]) +
		(v->y * m->m[5]) +
		(v->z * m->m[9]);
	ret.z = (v->x * m->m[2]) +
		(v->y * m->m[6]) +
		(v->z * m->m[10]);
	return ret;
}


vec3_t m_matmul33(const mat33_t* m, const vec3_t* v)
{
	vec3_t ret;
	ret.x = (v->x * m->m[0]) +
		(v->y * m->m[3]) +
		(v->z * m->m[6]);
	ret.y = (v->x * m->m[1]) +
		(v->y * m->m[4]) +
		(v->z * m->m[7]);
	ret.z = (v->x * m->m[2]) +
		(v->y * m->m[5]) +
		(v->z * m->m[8]);
	return ret;
}

void
m_matmul(mat44_t* to, const mat44_t* by)
{
	const float*__restrict__ a = to->m;
	const float*__restrict__ b = by->m;
	float m[16];

	m[0] = a[0] * b[0] + a[4] * b[1] + a[8] * b[2] + a[12] * b[3];
	m[1] = a[1] * b[0] + a[5] * b[1] + a[9] * b[2] + a[13] * b[3];
	m[2] = a[2] * b[0] + a[6] * b[1] + a[10] * b[2] + a[14] * b[3];
	m[3] = a[3] * b[0] + a[7] * b[1] + a[11] * b[2] + a[15] * b[3];
	m[4] = a[0] * b[4] + a[4] * b[5] + a[8] * b[6] + a[12] * b[7];
	m[5] = a[1] * b[4] + a[5] * b[5] + a[9] * b[6] + a[13] * b[7];
	m[6] = a[2] * b[4] + a[6] * b[5] + a[10] * b[6] + a[14] * b[7];
	m[7] = a[3] * b[4] + a[7] * b[5] + a[11] * b[6] + a[15] * b[7];
	m[8] = a[0] * b[8] + a[4] * b[9] + a[8] * b[10] + a[12] * b[11];
	m[9] = a[1] * b[8] + a[5] * b[9] + a[9] * b[10] + a[13] * b[11];
	m[10] = a[2] * b[8] + a[6] * b[9] + a[10] * b[10] + a[14] * b[11];
	m[11] = a[3] * b[8] + a[7] * b[9] + a[11] * b[10] + a[15] * b[11];
	m[12] = a[0] * b[12] + a[4] * b[13] + a[8] * b[14] + a[12] * b[15];
	m[13] = a[1] * b[12] + a[5] * b[13] + a[9] * b[14] + a[13] * b[15];
	m[14] = a[2] * b[12] + a[6] * b[13] + a[10] * b[14] + a[14] * b[15];
	m[15] = a[3] * b[12] + a[7] * b[13] + a[11] * b[14] + a[15] * b[15];

	memcpy(to->m, m, 16 * sizeof(float));
}

void
m_translate(mat44_t* m, float x, float y, float z)
{
	mat44_t trans;
	m_setidentity(&trans);
	trans.m[12] = x;
	trans.m[13] = y;
	trans.m[14] = z;
	m_matmul(m, &trans);
}

void
m_rotate(mat44_t* m, float angle, float x, float y, float z)
{
	float cosa = cosf(angle);
	float sina = sinf(angle);
	float icosa = 1.f - cosa;
	float r[16] = {
		cosa + x*x*icosa, y*x*icosa + z*sina, z*x*icosa - y*sina, 0.f,
		x*y*icosa - z*sina, cosa + y*y*icosa, z*y*icosa + x*sina, 0.f,
		x*z*icosa + y*sina, y*z*icosa - x*sina, cosa + z*z*icosa, 0.f,
		0.f, 0.f, 0.f, 1.f
	};
	m_matmul(m, (mat44_t*)&r);
}


void
m_transpose(mat44_t* m)
{
#if 0
	__m128 col1 = _mm_load_ps(&m->m[0]);
	__m128 col2 = _mm_load_ps(&m->m[4]);
	__m128 col3 = _mm_load_ps(&m->m[8]);
	__m128 col4 = _mm_load_ps(&m->m[12]);
	_MM_TRANSPOSE4_PS(col1, col2, col3, col4);
	_mm_store_ps(&m->m[0], col1);
	_mm_store_ps(&m->m[4], col2);
	_mm_store_ps(&m->m[8], col3);
	_mm_store_ps(&m->m[12], col4);
#else
	ML_SWAP(m->m[1], m->m[4]);
	ML_SWAP(m->m[2], m->m[8]);
	ML_SWAP(m->m[3], m->m[12]);
	ML_SWAP(m->m[6], m->m[9]);
	ML_SWAP(m->m[7], m->m[13]);
	ML_SWAP(m->m[11], m->m[14]);
#endif
}

void m_transpose33(mat33_t* m)
{
	ML_SWAP(m->m[1], m->m[3]);
	ML_SWAP(m->m[2], m->m[6]);
	ML_SWAP(m->m[5], m->m[7]);
}


bool m_invert(mat44_t* to, const mat44_t* from)
{
	float inv[16];
	float* out = to->m;
	const float* m = from->m;
	int i;
	float det;

	inv[0] = m[5]  * m[10] * m[15] -
		m[5]  * m[11] * m[14] -
		m[9]  * m[6]  * m[15] +
		m[9]  * m[7]  * m[14] +
		m[13] * m[6]  * m[11] -
		m[13] * m[7]  * m[10];

	inv[4] = -m[4]  * m[10] * m[15] +
		m[4]  * m[11] * m[14] +
		m[8]  * m[6]  * m[15] -
		m[8]  * m[7]  * m[14] -
		m[12] * m[6]  * m[11] +
		m[12] * m[7]  * m[10];

	inv[8] = m[4]  * m[9] * m[15] -
		m[4]  * m[11] * m[13] -
		m[8]  * m[5] * m[15] +
		m[8]  * m[7] * m[13] +
		m[12] * m[5] * m[11] -
		m[12] * m[7] * m[9];

	inv[12] = -m[4]  * m[9] * m[14] +
		m[4]  * m[10] * m[13] +
		m[8]  * m[5] * m[14] -
		m[8]  * m[6] * m[13] -
		m[12] * m[5] * m[10] +
		m[12] * m[6] * m[9];

	inv[1] = -m[1]  * m[10] * m[15] +
		m[1]  * m[11] * m[14] +
		m[9]  * m[2] * m[15] -
		m[9]  * m[3] * m[14] -
		m[13] * m[2] * m[11] +
		m[13] * m[3] * m[10];

	inv[5] = m[0]  * m[10] * m[15] -
		m[0]  * m[11] * m[14] -
		m[8]  * m[2] * m[15] +
		m[8]  * m[3] * m[14] +
		m[12] * m[2] * m[11] -
		m[12] * m[3] * m[10];

	inv[9] = -m[0]  * m[9] * m[15] +
		m[0]  * m[11] * m[13] +
		m[8]  * m[1] * m[15] -
		m[8]  * m[3] * m[13] -
		m[12] * m[1] * m[11] +
		m[12] * m[3] * m[9];

	inv[13] = m[0]  * m[9] * m[14] -
		m[0]  * m[10] * m[13] -
		m[8]  * m[1] * m[14] +
		m[8]  * m[2] * m[13] +
		m[12] * m[1] * m[10] -
		m[12] * m[2] * m[9];

	inv[2] = m[1]  * m[6] * m[15] -
		m[1]  * m[7] * m[14] -
		m[5]  * m[2] * m[15] +
		m[5]  * m[3] * m[14] +
		m[13] * m[2] * m[7] -
		m[13] * m[3] * m[6];

	inv[6] = -m[0]  * m[6] * m[15] +
		m[0]  * m[7] * m[14] +
		m[4]  * m[2] * m[15] -
		m[4]  * m[3] * m[14] -
		m[12] * m[2] * m[7] +
		m[12] * m[3] * m[6];

	inv[10] = m[0]  * m[5] * m[15] -
		m[0]  * m[7] * m[13] -
		m[4]  * m[1] * m[15] +
		m[4]  * m[3] * m[13] +
		m[12] * m[1] * m[7] -
		m[12] * m[3] * m[5];

	inv[14] = -m[0]  * m[5] * m[14] +
		m[0]  * m[6] * m[13] +
		m[4]  * m[1] * m[14] -
		m[4]  * m[2] * m[13] -
		m[12] * m[1] * m[6] +
		m[12] * m[2] * m[5];

	inv[3] = -m[1] * m[6] * m[11] +
		m[1] * m[7] * m[10] +
		m[5] * m[2] * m[11] -
		m[5] * m[3] * m[10] -
		m[9] * m[2] * m[7] +
		m[9] * m[3] * m[6];

	inv[7] = m[0] * m[6] * m[11] -
		m[0] * m[7] * m[10] -
		m[4] * m[2] * m[11] +
		m[4] * m[3] * m[10] +
		m[8] * m[2] * m[7] -
		m[8] * m[3] * m[6];

	inv[11] = -m[0] * m[5] * m[11] +
		m[0] * m[7] * m[9] +
		m[4] * m[1] * m[11] -
		m[4] * m[3] * m[9] -
		m[8] * m[1] * m[7] +
		m[8] * m[3] * m[5];

	inv[15] = m[0] * m[5] * m[10] -
		m[0] * m[6] * m[9] -
		m[4] * m[1] * m[10] +
		m[4] * m[2] * m[9] +
		m[8] * m[1] * m[6] -
		m[8] * m[2] * m[5];

	det = m[0] * inv[0] + m[1] * inv[4] + m[2] * inv[8] + m[3] * inv[12];

	if (det == 0)
		return false;

	det = 1.0 / det;

	for (i = 0; i < 16; i++)
		out[i] = inv[i] * det;

	return true;
}

// Must be orthonormal
void m_invert_orthonormal(mat44_t* to, const mat44_t* from)
{
	mat33_t R;
	vec3_t T;
	m_getmat33(&R, from);
	m_transpose33(&R);
	m_setvec3(T, -from->m[12], -from->m[13], -from->m[14]);
	T = m_matmul33(&R, &T);
	to->m[0] = R.m[0];
	to->m[1] = R.m[1];
	to->m[2] = R.m[2];
	to->m[4] = R.m[3];
	to->m[5] = R.m[4];
	to->m[6] = R.m[5];
	to->m[8] = R.m[6];
	to->m[9] = R.m[7];
	to->m[10] = R.m[8];
	to->m[12] = T.x;
	to->m[13] = T.y;
	to->m[14] = T.z;
	to->m[3] = to->m[7] = to->m[11] = 0;
	to->m[15] = 1.f;
}


vec4_t m_matmulvec(const mat44_t* m, const vec4_t* v)
{
	vec4_t ret;
	ret.x = (v->x * m->m[0]) +
		(v->y * m->m[4]) +
		(v->z * m->m[8]) +
		(v->w * m->m[12]);
	ret.y = (v->x * m->m[1]) +
		(v->y * m->m[5]) +
		(v->z * m->m[9]) +
		(v->w * m->m[13]);
	ret.z = (v->x * m->m[2]) +
		(v->y * m->m[6]) +
		(v->z * m->m[10]) +
		(v->w * m->m[14]);
	ret.w = (v->x * m->m[3]) +
		(v->y * m->m[7]) +
		(v->z * m->m[11]) +
		(v->w * m->m[15]);
	return ret;
}

vec3_t m_matmulvec3(const mat44_t* m, const vec3_t* v)
{
	vec3_t ret;
	ret.x = (v->x * m->m[0]) +
		(v->y * m->m[4]) +
		(v->z * m->m[8]) +
		(1.f * m->m[12]);
	ret.y = (v->x * m->m[1]) +
		(v->y * m->m[5]) +
		(v->z * m->m[9]) +
		(1.f * m->m[13]);
	ret.z = (v->x * m->m[2]) +
		(v->y * m->m[6]) +
		(v->z * m->m[10]) +
		(1.f * m->m[14]);
	return ret;
}


GLuint m_compile_shader(GLenum type, const char* source)
{
	GLuint name;
	GLint status;
	name = glCreateShader(type);
	glShaderSource(name, 1, &source, NULL);
	glCompileShader(name);
	glGetShaderiv(name, GL_COMPILE_STATUS, &status);
	if (status == GL_FALSE) {
		GLint length;
		GLchar* msg;
		glGetShaderiv(name, GL_INFO_LOG_LENGTH, &length);
		msg = (GLchar*)malloc(length);
		glGetShaderInfoLog(name, length, NULL, msg);
		fprintf(stderr, "glCompileShader failed: %s\n", msg);
		free(msg);
		glDeleteShader(name);
		name = 0;
	}
	return name;
}

GLuint m_link_program(GLuint vsh, GLuint fsh)
{
	GLuint program;
	GLint status;
	program = glCreateProgram();
	glAttachShader(program, vsh);
	glAttachShader(program, fsh);
	glLinkProgram(program);
	glGetProgramiv(program, GL_LINK_STATUS, &status);
	if (status == GL_FALSE) {
		GLint length;
		GLchar* msg;
		glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);
		msg = (GLchar*)malloc(length);
		glGetProgramInfoLog(program, length, NULL, msg);
		fprintf(stderr, "glLinkProgram failed: %s\n", msg);
		free(msg);
		glDeleteProgram(program);
		program = 0;
	}
	glDetachShader(program, vsh);
	glDetachShader(program, fsh);
	return program;
}


void m_create_material(material_t* material, const char* vsource, const char* fsource)
{
	GLuint vshader, fshader, program;
	vshader = m_compile_shader(GL_VERTEX_SHADER, vsource);
	fshader = m_compile_shader(GL_FRAGMENT_SHADER, fsource);
	if (vshader == 0)
		fatal_error("vshader source: %s", vsource);
	if (fshader == 0)
		fatal_error("fshader source: %s", fsource);
	program = m_link_program(vshader, fshader);
	glDeleteShader(vshader);
	glDeleteShader(fshader);
	material->program = program;
	glUseProgram(program);
	material->projmat = glGetUniformLocation(program, "projmat");
	material->modelview = glGetUniformLocation(program, "modelview");
	material->normalmat = glGetUniformLocation(program, "normalmat");
	material->chunk_offset = glGetUniformLocation(program, "chunk_offset");
	material->amb_light = glGetUniformLocation(program, "amb_light");
	material->fog_color = glGetUniformLocation(program, "fog_color");
	material->light_dir = glGetUniformLocation(program, "light_dir");
	material->sun_dir = glGetUniformLocation(program, "sun_dir");
	material->sun_color = glGetUniformLocation(program, "sun_color");
	material->sky_dark = glGetUniformLocation(program, "sky_dark");
	material->sky_light = glGetUniformLocation(program, "sky_light");
	material->tex0 = glGetUniformLocation(program, "tex0");
	glUseProgram(0);
}

void m_destroy_material(material_t* material)
{
	if (material->program != 0)
		glDeleteProgram(material->program);
	material->program = 0;
}

static inline GLsizei mesh_stride(GLenum flags)
{
	return ((flags & ML_POS_2F) ? 8 : 0) +
		((flags & ML_POS_3F) ? 12 : 0) +
		((flags & ML_POS_4UB) ? 4 : 0) +
		((flags & ML_POS_10_2) ? 4 : 0) +
		((flags & ML_CLR_4UB) ? 4 : 0) +
		((flags & ML_N_3F) ? 12 : 0) +
		((flags & ML_N_4B) ? 4 : 0) +
		((flags & ML_TC_2F) ? 8 : 0) +
		((flags & ML_TC_2US) ? 4 : 0);
}

void m_create_mesh(mesh_t* mesh, size_t n, void* data, GLenum flags, GLenum usage)
{
	M_CHECKGL(glGenVertexArrays(1, &mesh->vao));
	GLsizei stride = mesh_stride(flags);
	M_CHECKGL(glGenBuffers(1, &mesh->vbo));
	M_CHECKGL(glBindBuffer(GL_ARRAY_BUFFER, mesh->vbo));
	M_CHECKGL(glBufferData(GL_ARRAY_BUFFER, (GLsizei)n * stride, data, usage));
	M_CHECKGL(glBindBuffer(GL_ARRAY_BUFFER, 0));

	GLint offset = 0;
	mesh->position = (flags & (ML_POS_2F + ML_POS_3F + ML_POS_4UB + ML_POS_10_2)) ? offset : -1;
	offset += (flags & ML_POS_2F) ? 8 : ((flags & ML_POS_3F) ? 12 : (flags & (ML_POS_4UB + ML_POS_10_2) ? 4 : 0));
	mesh->normal = (flags & (ML_N_3F + ML_N_4B)) ? offset : -1;
	offset += (flags & ML_N_3F) ? 12 : ((flags & ML_N_4B) ? 4 : 0);
	mesh->texcoord = (flags & (ML_TC_2F + ML_TC_2US)) ? offset : -1;
	offset += (flags & ML_TC_2F) ? 8 : ((flags & ML_TC_2US) ? 4 : 0);
	mesh->color = (flags & ML_CLR_4UB) ? offset : -1;
	offset += (flags & ML_CLR_4UB) ? 4 : 0;
	mesh->stride = stride;
	mesh->mode = GL_TRIANGLES; // TODO: allow other modes?
	mesh->count = (GLsizei)n;
	mesh->flags = flags;
	mesh->ibo = 0; // no index buffer yet
	mesh->ibotype = 0;
	mesh->material = NULL; // no material bound yet
}

void m_update_mesh(mesh_t* mesh, GLintptr offset, GLsizeiptr n, const void* data)
{
	glBindBuffer(GL_ARRAY_BUFFER, mesh->vbo);
	glBufferSubData(GL_ARRAY_BUFFER, offset, n, data);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void m_replace_mesh(mesh_t* mesh, GLsizeiptr n, const void* data, GLenum usage)
{
	glBindBuffer(GL_ARRAY_BUFFER, mesh->vbo);
	glBufferData(GL_ARRAY_BUFFER, n, data, usage);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void m_create_indexed_mesh(mesh_t* mesh, size_t n, void* data, size_t ilen, GLenum indextype, void* indices, GLenum flags)
{
	m_create_mesh(mesh, n, data, flags, GL_STATIC_DRAW);
	M_CHECKGL(glGenBuffers(1, &mesh->ibo));

	GLsizei isize = 0;
	switch (indextype) {
	case GL_UNSIGNED_BYTE:
		isize = 1; break;
	case GL_UNSIGNED_SHORT:
		isize = 2; break;
	case GL_UNSIGNED_INT:
		isize = 4; break;
	default:
		fatal_error("indextype must be one of GL_UNSIGNED_[BYTE|SHORT|INT]");
	}

	M_CHECKGL(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh->ibo));
	M_CHECKGL(glBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizei)ilen * isize, indices, GL_STATIC_DRAW));
	M_CHECKGL(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0));

	mesh->count = (GLsizei)ilen;
	mesh->ibotype = indextype;
}

void m_destroy_mesh(mesh_t* mesh)
{
	if (mesh->vbo != 0) { M_CHECKGL(glDeleteBuffers(1, &(mesh->vbo))); mesh->vbo = 0; }
	if (mesh->ibo != 0) { M_CHECKGL(glDeleteBuffers(1, &(mesh->ibo))); mesh->ibo = 0; }
	if (mesh->vao != 0) { M_CHECKGL(glDeleteVertexArrays(1, &(mesh->vao))); mesh->vao = 0; }
	mesh->material = NULL;
}

void m_set_material(mesh_t* mesh, material_t* material)
{
	GLint midx;
	mesh->material = material;
	M_CHECKGL(glBindVertexArray(mesh->vao));
	if (mesh->ibo > 0)
		M_CHECKGL(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh->ibo));
	M_CHECKGL(glBindBuffer(GL_ARRAY_BUFFER, mesh->vbo));
	if ((midx = glGetAttribLocation(material->program, "position")) > -1) {
		if (mesh->position > -1) {
			if (mesh->flags & ML_POS_2F)
				M_CHECKGL(glVertexAttribPointer(midx, 2, GL_FLOAT, GL_FALSE, mesh->stride, (void*)((ptrdiff_t)mesh->position)));
			else if (mesh->flags & ML_POS_3F)
				M_CHECKGL(glVertexAttribPointer(midx, 3, GL_FLOAT, GL_FALSE, mesh->stride, (void*)((ptrdiff_t)mesh->position)));
			else if (mesh->flags & ML_POS_4UB)
				M_CHECKGL(glVertexAttribPointer(midx, 4, GL_UNSIGNED_BYTE, GL_TRUE, mesh->stride, (void*)((ptrdiff_t)mesh->position)));
			else // 10_10_10_2
				M_CHECKGL(glVertexAttribPointer(midx, 4, GL_UNSIGNED_INT_2_10_10_10_REV, GL_TRUE, mesh->stride, (void*)((ptrdiff_t)mesh->position)));
			M_CHECKGL(glEnableVertexAttribArray(midx));
		} else {
			M_CHECKGL(glDisableVertexAttribArray(midx));
		}
	}
	if ((midx = glGetAttribLocation(material->program, "normal")) > -1) {
		if (mesh->normal > -1) {
			if (mesh->flags & ML_N_3F)
				M_CHECKGL(glVertexAttribPointer(midx, 3, GL_FLOAT, GL_FALSE, mesh->stride, (void*)((ptrdiff_t)mesh->normal)));
			else // 4ub
				M_CHECKGL(glVertexAttribPointer(midx, 4, GL_BYTE, GL_TRUE, mesh->stride, (void*)((ptrdiff_t)mesh->normal)));
			M_CHECKGL(glEnableVertexAttribArray(midx));
		} else {
			M_CHECKGL(glDisableVertexAttribArray(midx));
		}
	}
	if ((midx = glGetAttribLocation(material->program, "texcoord")) > -1) {
		if (mesh->texcoord > -1) {
			if (mesh->flags & ML_TC_2F)
				M_CHECKGL(glVertexAttribPointer(midx, 2, GL_FLOAT, GL_FALSE, mesh->stride, (void*)((ptrdiff_t)mesh->texcoord)));
			else // 2US
				M_CHECKGL(glVertexAttribPointer(midx, 2, GL_UNSIGNED_SHORT, GL_TRUE, mesh->stride, (void*)((ptrdiff_t)mesh->texcoord)));
			M_CHECKGL(glEnableVertexAttribArray(midx));
		} else {
			M_CHECKGL(glDisableVertexAttribArray(midx));
		}
	}
	if ((midx = glGetAttribLocation(material->program, "color")) > -1) {
		if (mesh->color > -1) {
			glVertexAttribPointer(midx, GL_BGRA, GL_UNSIGNED_BYTE, GL_TRUE, mesh->stride, (void*)((ptrdiff_t)mesh->color));
			glEnableVertexAttribArray(midx);

		} else {
			glDisableVertexAttribArray(midx);
		}
	}
	M_CHECKGL(glBindVertexArray(0));
}

void m_mtxstack_init(mtxstack_t* stack, size_t size)
{
	if (size == 0)
		fatal_error("Matrix stack too small");
	stack->top = 0;
	stack->stack = malloc(sizeof(mat44_t) * size);
	for (size_t i = 0; i < size; ++i)
		m_setidentity(stack->stack + i);
}

void m_mtxstack_destroy(mtxstack_t* stack)
{
	free(stack->stack);
	stack->top = -1;
	stack->stack = NULL;
}

void m_pushmatrix(mtxstack_t* stack)
{
	++stack->top;
	m_copymat(stack->stack + stack->top, stack->stack + stack->top - 1);
}

void m_loadmatrix(mtxstack_t* stack, mat44_t* m)
{
	m_copymat(stack->stack + stack->top, m);
}

void m_loadidentity(mtxstack_t* stack)
{
	m_setidentity(stack->stack + stack->top);
}

void m_pushidentity(mtxstack_t* stack)
{
	++stack->top;
	m_setidentity(stack->stack + stack->top);
}

void m_popmatrix(mtxstack_t* stack)
{
	if (stack->top < 0)
		fatal_error("Matrix stack underflow");
	--stack->top;
}

mat44_t* m_getmatrix(mtxstack_t* stack)
{
	if (stack->top < 0)
		fatal_error("Matrix stack underflow");
	return stack->stack + stack->top;
}

void m_tex2d_load(tex2d_t* tex, const char* filename)
{
	int x, y, n;
	unsigned char* data;

	memset(tex, 0, sizeof(tex2d_t));
	data = stbi_load(filename, &x, &y, &n, 0);
	if (data == NULL) {
		fatal_error("Failed to load image %s", filename);
	}

	M_CHECKGL(glActiveTexture(GL_TEXTURE0));
	M_CHECKGL(glGenTextures(1, &tex->id));
	M_CHECKGL(glBindTexture(GL_TEXTURE_2D, tex->id));
	M_CHECKGL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
	M_CHECKGL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST));
//	M_CHECKGL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR));
//	M_CHECKGL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
	M_CHECKGL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
	M_CHECKGL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
	tex->w = (uint16_t)x;
	tex->h = (uint16_t)y;

	switch (n) {
	case 4:
		M_CHECKGL(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, x, y, 0, GL_RGBA, GL_UNSIGNED_BYTE, data));
		break;
	case 3:
		M_CHECKGL(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, x, y, 0, GL_RGB, GL_UNSIGNED_BYTE, data));
		break;
	case 1:
		M_CHECKGL(glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, x, y, 0, GL_RED, GL_UNSIGNED_BYTE, data));
		break;
	default:
		fatal_error("bad pixel depth %d for %s", n, filename);
	}

	//M_CHECKGL(glGenerateMipmap(GL_TEXTURE_2D));

	stbi_image_free(data);
	M_CHECKGL(glBindTexture(GL_TEXTURE_2D, 0));
}

void m_save_screenshot(const char* filename)
{
	int ok, x, y, w, h;
	uint8_t* data;

	int viewport[4] = {0};
	M_CHECKGL(glGetIntegerv(GL_VIEWPORT, viewport));
	if (viewport[2] == 0 || viewport[3] == 0) {
		puts("Failed to get viewport information.");
		return;
	}
	printf("viewport: %d,%d - %d,%d\n", viewport[0], viewport[1], viewport[2], viewport[3]);

	data = (uint8_t*)malloc(viewport[2] * viewport[3] * 3);
	M_CHECKGL(glReadBuffer(GL_FRONT));
	M_CHECKGL(glReadPixels(viewport[0], viewport[1], viewport[2], viewport[3], GL_RGB, GL_UNSIGNED_BYTE, data));

	// need to flip the image data
	w = viewport[2]*3;
	h = viewport[3];
	for (y = 0; y < h/2; ++y)
		for (x = 0; x < w; ++x) {
			uint8_t p = data[y * w + x];
			data[y * w + x] = data[(h - y - 1) * w + x];
			data[(h - y - 1) * w + x] = p;
		}

	ok = stbi_write_png(filename, viewport[2], viewport[3], 3, data, 0);
	free(data);
	if (!ok) {
		printf("Failed to write to %s\n", filename);
	}
}

void m_tex2d_destroy(tex2d_t* tex)
{
	M_CHECKGL(glDeleteTextures(1, &tex->id));
	memset(tex, 0, sizeof(tex2d_t));

}

void m_tex2d_bind(tex2d_t* tex, int index)
{
	M_CHECKGL(glActiveTexture(GL_TEXTURE0 + index));
	M_CHECKGL(glBindTexture(GL_TEXTURE_2D, tex->id));
}

/* based on code by Pierre Terdiman
   http://www.codercorner.com/RayAABB.cpp
*/
bool collide_ray_aabb(vec3_t origin, vec3_t dir, vec3_t center, vec3_t extent)
{
	vec3_t diff;

	diff.x = origin.x - center.x;
	if (fabsf(diff.x) > extent.x && diff.x*dir.x >= 0.0f)
		return false;

	diff.y = origin.y - center.y;
	if (fabsf(diff.y) > extent.y && diff.y*dir.y >= 0.0f)
		return false;

	diff.z = origin.z - center.z;
	if (fabsf(diff.z) > extent.z && diff.z*dir.z >= 0.0f)
		return false;

	vec3_t absdir = { fabsf(dir.x), fabsf(dir.y), fabsf(dir.z) };
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
bool collide_sphere_aabb(vec3_t pos, float radius, vec3_t center, vec3_t extent)
{
	float dmin = 0;
	vec3_t bmin = { center.x - extent.x, center.y - extent.y, center.z - extent.z };
	vec3_t bmax = { center.x + extent.x, center.y + extent.y, center.z + extent.z };
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

bool collide_sphere_aabb_full(vec3_t pos, float radius, vec3_t center, vec3_t extent, vec3_t* hit)
{
	vec3_t sphereCenterRelBox;
	sphereCenterRelBox = m_vec3sub(pos, center);
	// Point on surface of box that is closest to the center of the sphere
	vec3_t boxPoint;

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

	vec3_t dist = m_vec3sub(sphereCenterRelBox, boxPoint);

	if (dist.x*dist.x + dist.y*dist.y + dist.z*dist.z < radius*radius) {
		*hit = boxPoint;
		return true;
	}
	return false;
}

bool collide_aabb_aabb_full(vec3_t center, vec3_t extent, vec3_t center2, vec3_t extent2, vec3_t *hitpoint, vec3_t *hitdelta, vec3_t *hitnormal)
{
	float dx = center2.x - center.x;
	float px = (extent2.x + extent.x) - fabs(dx);
	if (px <= 0)
		return false;

	float dy = center2.y - center.y;
	float py = (extent2.y + extent.y) - fabs(dy);
	if (py <= 0)
		return false;

	float dz = center2.z - center.z;
	float pz = (extent2.z + extent.z) - fabs(dz);
	if (pz <= 0)
		return false;

	if (px < py && px < pz) {
		float sx = m_sign(dx);
		hitdelta->x = px * sx;
		hitnormal->x = sx;
		hitdelta->y = hitdelta->z = 0;
		hitnormal->y = hitnormal->z = 0;
		hitpoint->x = center.x + (extent.x * sx);
		hitpoint->y = center.y;
		hitpoint->z = center.z;
	}
	if (py < px && py < pz) {
		float sy = m_sign(dy);
		hitdelta->y = py * sy;
		hitnormal->y = sy;
		hitdelta->x = hitdelta->z = 0;
		hitnormal->x = hitnormal->z = 0;
		hitpoint->y = center.y + (extent.y * sy);
		hitpoint->x = center.x;
		hitpoint->z = center.z;
	}
	if (pz < px && py < py) {
		float sz = m_sign(dz);
		hitdelta->z = pz * sz;
		hitnormal->z = sz;
		hitdelta->x = hitdelta->y = 0;
		hitnormal->x = hitnormal->y = 0;
		hitpoint->z = center.z + (extent.z * sz);
		hitpoint->x = center.x;
		hitpoint->y = center.y;
	}
	return true;
}


bool collide_segment_aabb(vec3_t pos, vec3_t delta, vec3_t padding, vec3_t center, vec3_t extent,
	float* time, vec3_t* hit, vec3_t* hitdelta, vec3_t* normal)
{
	float scaleX = 1.f / delta.x;
	float scaleY = 1.f / delta.y;
	float scaleZ = 1.f / delta.z;
	float signX = m_sign(scaleX);
	float signY = m_sign(scaleY);
	float signZ = m_sign(scaleZ);
	float nearTimeX = (center.x - signX * (extent.x + padding.x) - pos.x) * scaleX;
	float nearTimeY = (center.y - signY * (extent.y + padding.y) - pos.y) * scaleY;
	float nearTimeZ = (center.z - signZ * (extent.z + padding.z) - pos.z) * scaleZ;
	float farTimeX = (center.x + signX * (extent.x + padding.x) - pos.x) * scaleX;
	float farTimeY = (center.y + signY * (extent.y + padding.y) - pos.y) * scaleY;
	float farTimeZ = (center.z + signZ * (extent.z + padding.z) - pos.z) * scaleZ;
	if (nearTimeX > farTimeY || nearTimeY > farTimeX ||
		nearTimeX > farTimeZ || nearTimeZ > farTimeX)
		return false;
	float nearTime = ML_MAX(ML_MAX(nearTimeX, nearTimeY), nearTimeZ);
	float farTime = ML_MIN(ML_MIN(farTimeX, farTimeY), farTimeZ);
	if (nearTime >= 1.f || farTime <= 0.f)
		return false;

	float t = m_clamp(nearTime, 0, 1.f);
	vec3_t dir = delta;
	m_vec3normalize(dir);
	*time = t;
	hitdelta->x = t * delta.x;
	hitdelta->y = t * delta.y;
	hitdelta->z = t * delta.z;
	hit->x = pos.x + hitdelta->x;
	hit->y = pos.y + hitdelta->y;
	hit->z = pos.z + hitdelta->z;
	if (nearTimeX > nearTimeY && nearTimeX > nearTimeZ) {
		normal->x = -signX;
		normal->y = normal->z = 0.f;
	} else if (nearTimeY > nearTimeX && nearTimeY > nearTimeZ) {
		normal->y = -signY;
		normal->x = normal->z = 0.f;
	} else {
		normal->z = -signZ;
		normal->x = normal->y = 0.f;
	}
	return true;
}

bool intersect_moving_aabb_aabb(aabb_t a, aabb_t b, vec3_t va, vec3_t vb, float* tfirst, float* tlast)
{
	if (collide_aabb_aabb(a.center, a.extent, b.center, b.extent)) {
		*tfirst = *tlast = 0.f;
		return true;
	}

	vec3_t v = m_vec3sub(vb, va);
	*tfirst = 0;
	*tlast = 1.f;

	// for each axis, determine time of first and last contact (if any)
	{
		float amax_x = a.center.x + a.extent.x;
		float amin_x = a.center.x - a.extent.x;
		float bmax_x = b.center.x + b.extent.x;
		float bmin_x = b.center.x - b.extent.x;
		if (v.x < 0) {
			if (bmax_x < amin_x) return false;
			if (amax_x < bmin_x) *tfirst = ML_MAX((amax_x - bmin_x) / v.x, *tfirst);
			if (bmax_x > amin_x) *tlast = ML_MIN((amin_x - bmax_x) / v.x, *tlast);
		}
		if (v.x > 0) {
			if (bmin_x > amax_x) return false;
			if (bmax_x < amin_x) *tfirst = ML_MAX((amin_x - bmax_x) / v.x, *tfirst);
			if (amax_x > bmin_x) *tlast = ML_MIN((amax_x - bmin_x) / v.x, *tlast);
		}
		// No overlap possible if time of first contact occurs after time of last contact
		if (*tfirst > *tlast) return false;
	}

	{
		float amax_y = a.center.y + a.extent.y;
		float amin_y = a.center.y - a.extent.y;
		float bmax_y = b.center.y + b.extent.y;
		float bmin_y = b.center.y - b.extent.y;
		if (v.y < 0) {
			if (bmax_y < amin_y) return false;
			if (amax_y < bmin_y) *tfirst = ML_MAX((amax_y - bmin_y) / v.y, *tfirst);
			if (bmax_y > amin_y) *tlast = ML_MIN((amin_y - bmax_y) / v.y, *tlast);
		}
		if (v.y > 0) {
			if (bmin_y > amax_y) return false;
			if (bmax_y < amin_y) *tfirst = ML_MAX((amin_y - bmax_y) / v.y, *tfirst);
			if (amax_y > bmin_y) *tlast = ML_MIN((amax_y - bmin_y) / v.y, *tlast);
		}
		// No overlap possible if time of first contact occurs after time of last contact
		if (*tfirst > *tlast) return false;
	}

	{
		float amax_z = a.center.z + a.extent.z;
		float amin_z = a.center.z - a.extent.z;
		float bmax_z = b.center.z + b.extent.z;
		float bmin_z = b.center.z - b.extent.z;
		if (v.z < 0) {
			if (bmax_z < amin_z) return false;
			if (amax_z < bmin_z) *tfirst = ML_MAX((amax_z - bmin_z) / v.z, *tfirst);
			if (bmax_z > amin_z) *tlast = ML_MIN((amin_z - bmax_z) / v.z, *tlast);
		}
		if (v.z > 0) {
			if (bmin_z > amax_z) return false;
			if (bmax_z < amin_z) *tfirst = ML_MAX((amin_z - bmax_z) / v.z, *tfirst);
			if (amax_z > bmin_z) *tlast = ML_MIN((amax_z - bmin_z) / v.z, *tlast);
		}
		// No overlap possible if time of first contact occurs after time of last contact
		if (*tfirst > *tlast) return false;
	}

	return true;
}
