#ifndef ROAM_MATH3D_H_
#define ROAM_MATH3D_H_

typedef float mVec2_t[2];
typedef float mVec3_t[3];
typedef float mVec4_t[4];

typedef struct mMat33_t {
	float m[9];
} mMat33_t;

typedef struct mMat44_t {
	float m[16];
} mMat44_t;


void
mPerspective(mMat44_t* m, float fovy, float aspect, float zNear, float zFar);

#endif/*ROAM_MATH3D_H_*/
