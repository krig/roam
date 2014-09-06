#include "common.h"
#include "math3d.h"

void
mPerspective(mMat44_t* m, float fovy, float aspect, float zNear, float zFar) {
	float f = 1.f / tan(fovy * 0.5f);
	m->m[0] = f / aspect;
	m->m[1] = 0;
	m->m[2] = 0;
	m->m[3] = 0;
	m->m[4] = 0;
	m->m[5] = f;
	m->m[6] = 0;
	m->m[7] = 0;
	m->m[8] = 0;
	m->m[9] = 0;
	m->m[10] = (zFar + zNear) / (zNear - zFar);
	m->m[11] = -1.f;
	m->m[12] = 0;
	m->m[13] = 0;
	m->m[14] = (2.f * zFar * zNear) / (zNear - zFar);
	m->m[15] = 0;
}
