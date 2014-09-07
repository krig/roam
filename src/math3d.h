#ifndef ROAM_MATH3D_H_
#define ROAM_MATH3D_H_

#ifndef ML_PI
#define ML_PI 3.14159265358979323846	/* pi */
#endif

#define mlDeg2Rad(d) (((d) * ML_PI) / 180.f)
#define mlRad2Deg(r) (((r) * 180.f) / ML_PI)

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

typedef struct ml_matrix {
	float m[16];
} ml_matrix;

#define mlVec3Assign(v, a, b, c) { (v).x = (a); (v).y = (b); (v).z = (c); }

void mlPerspective(ml_matrix* m, float fovy, float aspect, float zNear, float zFar);

void mlLoadIdentity(ml_matrix* m);

void mlLookAt(ml_matrix* m,
              float eyeX, float eyeY, float eyeZ,
              float atX, float atY, float atZ,
              float upX, float upY, float upZ);

void mlLoadMatrix(ml_matrix* to, const ml_matrix* from);

void mlLoadFPSMatrix(ml_matrix* to, float eyeX, float eyeY, float eyeZ, float pitch, float yaw);

void mlMulMatrix(ml_matrix* to, const ml_matrix* by);

ml_vec4 mlMulMatVec(const ml_matrix* m, const ml_vec4* v);

void mlTranslate(ml_matrix* m, float x, float y, float z);

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
mlVec3Length(const ml_vec3 v) {
	return sqrtf(mlVec3Dot(v, v));
}

static inline float
mlVec3Normalize(ml_vec3* v) {
	float len = mlVec3Length(*v);
	v->x *= 1.f / len;
	v->y *= 1.f / len;
	v->z *= 1.f / len;
	return len;
}

#endif/*ROAM_MATH3D_H_*/
