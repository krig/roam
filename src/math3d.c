#include "common.h"
#include "math3d.h"

void mlPerspective(ml_matrix* m, float fovy, float aspect, float zNear, float zFar) {
	mlSetIdentity(m);
	float r = fovy * 0.5f;
	float f = cos(r) / sin(r);
	m->m[0] = f / aspect;
	m->m[5] = f;
	m->m[10] = -(zFar + zNear) / (zFar - zNear);
	m->m[11] = -1.f;
	m->m[14] = (-2.f * zFar * zNear) / (zFar - zNear);
	m->m[15] = 0;
}

void mlSetIdentity(ml_matrix* m) {
	memset(m->m, 0, sizeof(float)*16);
	m->m[0] = m->m[5] = m->m[10] = m->m[15] = 1.f;
}

void mlFPSRotation(float pitch, float yaw, ml_vec3* x, ml_vec3* y, ml_vec3* z) {
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

void mlFPSMatrix(ml_matrix* to, ml_vec3 eye, float pitch, float yaw) {
	float cosPitch = cosf(pitch);
	float sinPitch = sinf(pitch);
	float cosYaw = cosf(yaw);
	float sinYaw = sinf(yaw);
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
	mlCopyMatrix(m, &M);
	mlTranslate(m, -eyeX, -eyeY, -eyeZ);
}

void
mlCopyMatrix(ml_matrix* to, const ml_matrix* from) {
	memcpy(to, from, sizeof(ml_matrix));
}

void
mlMulMatrix(ml_matrix* to, const ml_matrix* by) {
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
mlTranslate(ml_matrix* m, float x, float y, float z) {
	ml_matrix trans;
	mlSetIdentity(&trans);
	trans.m[12] = x;
	trans.m[13] = y;
	trans.m[14] = z;
	mlMulMatrix(m, &trans);
}

void
mlRotate(ml_matrix* m, float x, float y, float z, float angle) {
	float cosa = cosf(angle);
	float sina = sinf(angle);
	float icosa = 1.f - cosa;
	float r[16] = {
		cosa + x*x*icosa, y*x*icosa + z*sina, z*x*icosa - y*sina, 0.f,
		x*y*icosa - z*sina, cosa + y*y*icosa, z*y*icosa + x*sina, 0.f,
		x*z*icosa + y*sina, y*z*icosa - x*sina, cosa + z*z*icosa, 0.f,
		0.f, 0.f, 0.f, 1.f
	};
	mlMulMatrix(m, (ml_matrix*)&r);
}


void
mlTranspose(ml_matrix* m) {
	__m128 col1 = _mm_load_ps(&m->m[0]);
    __m128 col2 = _mm_load_ps(&m->m[4]);
    __m128 col3 = _mm_load_ps(&m->m[8]);
    __m128 col4 = _mm_load_ps(&m->m[12]);
     _MM_TRANSPOSE4_PS(col1, col2, col3, col4);
     _mm_store_ps(&m->m[0], col1);
     _mm_store_ps(&m->m[4], col2);
     _mm_store_ps(&m->m[8], col3);
     _mm_store_ps(&m->m[12], col4);
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

GLuint mlCompileShader(GLenum type, const char* source) {
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

GLuint mlLinkProgram(GLuint vsh, GLuint fsh) {
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


void mlCreateMaterial(material_t* material, const char* vsource, const char* fsource) {
	GLuint vshader, fshader, program;
	vshader = mlCompileShader(GL_VERTEX_SHADER, vsource);
	fshader = mlCompileShader(GL_FRAGMENT_SHADER, fsource);
	if (vshader > 0 && fshader > 0) {
		program = mlLinkProgram(vshader, fshader);
	}
	if (vshader != 0)
		glDeleteShader(vshader);
	if (fshader != 0)
		glDeleteShader(fshader);
	material->program = program;
	glUseProgram(program);
	material->projmat = glGetUniformLocation(program, "projmat");
	material->modelview = glGetUniformLocation(program, "modelview");
	material->normalmat = glGetUniformLocation(program, "normalmat");
	material->position = glGetAttribLocation(program, "position");
	material->color = glGetAttribLocation(program, "color");
	material->normal = glGetAttribLocation(program, "normal");
	glUseProgram(0);
}

void mlCreateMesh(mesh_t* mesh, size_t n, vtx_pos_clr_t* data) {
	glGenBuffers(1, &mesh->vbo);
	glBindBuffer(GL_ARRAY_BUFFER, mesh->vbo);
	glBufferData(GL_ARRAY_BUFFER, n * sizeof(vtx_pos_clr_t), data, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	mesh->position = 0;
	mesh->color = 3 * sizeof(float);
	mesh->normal = -1;
	mesh->stride = sizeof(vtx_pos_clr_t);
	mesh->mode = GL_TRIANGLES;
	mesh->count = n;
}

void mlCreateRenderable(renderable_t* renderable, const material_t* material, const mesh_t* mesh) {
	glGenVertexArrays(1, &renderable->vao);
	glBindVertexArray(renderable->vao);
	renderable->material = material;
	renderable->mesh = mesh;

	glBindBuffer(GL_ARRAY_BUFFER, mesh->vbo);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, mesh->stride, 0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, GL_BGRA, GL_UNSIGNED_BYTE, GL_TRUE, mesh->stride, (const void*)((ptrdiff_t)mesh->color));
	glEnableVertexAttribArray(1);
	glBindVertexArray(0);
}

void mlInitMatrixStack(ml_matrixstack* stack, size_t size) {
	if (size == 0)
		roamError("Matrix stack too small");
	stack->top = 0;
	stack->stack = malloc(sizeof(ml_matrix) * size);
	for (size_t i = 0; i < size; ++i)
		mlSetIdentity(stack->stack + i);
}

void mlFreeMatrixStack(ml_matrixstack* stack) {
	free(stack->stack);
	stack->top = -1;
	stack->stack = NULL;
}

void mlPushMatrix(ml_matrixstack* stack) {
	++stack->top;
	mlCopyMatrix(stack->stack + stack->top, stack->stack + stack->top - 1);
}

void mlLoadMatrix(ml_matrixstack* stack, ml_matrix* m) {
	mlCopyMatrix(stack->stack + stack->top, m);
}

void mlLoadIdentity(ml_matrixstack* stack) {
	mlSetIdentity(stack->stack + stack->top);
}

void mlPushIdentity(ml_matrixstack* stack) {
	++stack->top;
	mlSetIdentity(stack->stack + stack->top);
}

void mlPopMatrix(ml_matrixstack* stack) {
	if (stack->top < 0)
		roamError("Matrix stack underflow");
	--stack->top;
}

ml_matrix* mlGetMatrix(ml_matrixstack* stack) {
	if (stack->top < 0)
		roamError("Matrix stack underflow");
	return stack->stack + stack->top;
}
