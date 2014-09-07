#include "common.h"
#include "math3d.h"

void mlPerspective(ml_matrix* m, float fovy, float aspect, float zNear, float zFar) {
	mlLoadIdentity(m);
	float r = fovy * 0.5f;
	float f = cos(r) / sin(r);
	m->m[0] = f / aspect;
	m->m[5] = f;
	m->m[10] = -(zFar + zNear) / (zFar - zNear);
	m->m[11] = -1.f;
	m->m[14] = (-2.f * zFar * zNear) / (zFar - zNear);
	m->m[15] = 0;
}

void mlLoadIdentity(ml_matrix* m) {
	memset(m->m, 0, sizeof(float)*16);
	m->m[0] = m->m[5] = m->m[10] = m->m[15] = 1.f;
}

void mlLoadFPSMatrix(ml_matrix* to, float eyeX, float eyeY, float eyeZ, float pitch, float yaw) {
	float cosPitch = cosf(pitch);
	float sinPitch = sinf(pitch);
	float cosYaw = cosf(yaw);
	float sinYaw = sinf(yaw);
	ml_vec3 eye = {eyeX, eyeY, eyeZ };
	ml_vec3 xaxis = { cosYaw, 0, -sinYaw };
	ml_vec3 yaxis = {sinYaw * sinPitch, cosPitch, cosYaw * sinPitch };
	ml_vec3 zaxis = {sinYaw * cosPitch, -sinPitch, cosPitch * cosYaw };
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
	to->m[12] = -mlVec3Dot(xaxis, eye);
	to->m[13] = -mlVec3Dot(yaxis, eye);
	to->m[14] = -mlVec3Dot(zaxis, eye);
	to->m[15] = 1.f;
}


/* I don't think this is right. */
void mlLookAt(ml_matrix* m,
              float eyeX, float eyeY, float eyeZ,
              float atX, float atY, float atZ,
              float upX, float upY, float upZ) {
	ml_matrix M;
	ml_vec3 f, u, s;

	mlVec3Assign(f, atX - eyeX, atY - eyeY, atZ - eyeZ);
	mlVec3Normalize(&f);
	mlVec3Assign(u, upX, upY, upZ);
	s = mlVec3Cross(f, u);
	mlVec3Normalize(&s);
	u = mlVec3Cross(s, f);
	mlVec3Normalize(&u);

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
	mlLoadMatrix(m, &M);
	mlTranslate(m, -eyeX, -eyeY, -eyeZ);
}

void
mlLoadMatrix(ml_matrix* to, const ml_matrix* from) {
	memcpy(to, from, sizeof(ml_matrix));
}

void
mlMulMatrix(ml_matrix* to, const ml_matrix* by) {
	float m[16];
	const float* a = to->m;
	const float* b = by->m;

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
mlTranslate(ml_matrix* m, float x, float y, float z) {
	ml_matrix trans;
	mlLoadIdentity(&trans);
	trans.m[12] = x;
	trans.m[13] = y;
	trans.m[14] = z;
	mlMulMatrix(m, &trans);
}

ml_vec4 mlMulMatVec(const ml_matrix* m, const ml_vec4* v) {
	ml_vec4 ret;
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
