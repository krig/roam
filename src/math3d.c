#include "common.h"
#include "math3d.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

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
	ml_vec3 yaxis = { sinYaw * sinPitch, cosPitch, cosYaw * sinPitch };
	ml_vec3 zaxis = { sinYaw * cosPitch, -sinPitch, cosPitch * cosYaw };
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
	f = mlVec3Normalize(f);
	mlVec3Assign(u, upX, upY, upZ);
	s = mlVec3Normalize(mlVec3Cross(f, u));
	u = mlVec3Normalize(mlVec3Cross(s, f));

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

void mlGetRotationMatrix(ml_matrix33* to, const ml_matrix* from) {
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

ml_vec3 mlMulMat33Vec(const ml_matrix33* m, const ml_vec3* v) {
	ml_vec3 ret;
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
mlRotate(ml_matrix* m, float angle, float x, float y, float z) {
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
	mlSwap(m->m[1], m->m[4]);
	mlSwap(m->m[2], m->m[8]);
	mlSwap(m->m[3], m->m[12]);
	mlSwap(m->m[6], m->m[9]);
	mlSwap(m->m[7], m->m[13]);
	mlSwap(m->m[11], m->m[14]);
#endif
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


void mlCreateMaterial(ml_material* material, const char* vsource, const char* fsource) {
	GLuint vshader, fshader, program;
	vshader = mlCompileShader(GL_VERTEX_SHADER, vsource);
	fshader = mlCompileShader(GL_FRAGMENT_SHADER, fsource);
	if (vshader == 0)
		roamError("vshader source: %s", vsource);
	if (fshader == 0)
		roamError("fshader source: %s", fsource);
	program = mlLinkProgram(vshader, fshader);
	glDeleteShader(vshader);
	glDeleteShader(fshader);
	material->program = program;
	glUseProgram(program);
	material->projmat = glGetUniformLocation(program, "projmat");
	material->modelview = glGetUniformLocation(program, "modelview");
	material->normalmat = glGetUniformLocation(program, "normalmat");
	material->chunk_offset = glGetUniformLocation(program, "chunk_offset");
	material->amb_color = glGetUniformLocation(program, "amb_color");
	material->fog_color = glGetUniformLocation(program, "fog_color");
	material->light_dir = glGetUniformLocation(program, "light_dir");
	material->position = glGetAttribLocation(program, "position");
	material->texcoord = glGetAttribLocation(program, "texcoord");
	material->color = glGetAttribLocation(program, "color");
	material->normal = glGetAttribLocation(program, "normal");
	glUseProgram(0);
}

void mlCreateMesh(ml_mesh* mesh, size_t n, void* data, GLenum flags) {
	size_t stride =
		((flags & ML_POS_2F) ? 8 : 0) +
		((flags & ML_POS_3F) ? 12 : 0) +
		((flags & ML_POS_4UB) ? 4 : 0) +
		((flags & ML_CLR_4UB) ? 4 : 0) +
		((flags & ML_N_3F) ? 12 : 0) +
		((flags & ML_N_4UB) ? 4 : 0) +
		((flags & ML_TC_2F) ? 8 : 0);
	glGenBuffers(1, &mesh->vbo);
	glBindBuffer(GL_ARRAY_BUFFER, mesh->vbo);
	glBufferData(GL_ARRAY_BUFFER, n * stride, data, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	// no indices in this mesh
	mesh->ibo = 0;
	mesh->ibotype = 0;

	GLint offset = 0;
	mesh->position = (flags & (ML_POS_2F + ML_POS_3F + ML_POS_4UB)) ? offset : -1;
	offset += (flags & ML_POS_2F) ? 8 : ((flags & ML_POS_3F) ? 12 : (flags & ML_POS_4UB ? 4 : 0));
	mesh->normal = (flags & (ML_N_3F + ML_N_4UB)) ? offset : -1;
	offset += (flags & ML_N_3F) ? 12 : ((flags & ML_N_4UB) ? 4 : 0);
	mesh->texcoord = (flags & ML_TC_2F) ? offset : -1;
	offset += (flags & ML_TC_2F) ? 8 : 0;
	mesh->color = (flags & ML_CLR_4UB) ? offset : -1;
	offset += (flags & ML_CLR_4UB) ? 4 : 0;
	mesh->stride = stride;
	mesh->mode = GL_TRIANGLES;
	mesh->count = n;
	mesh->flags = flags;
	glCheck(__LINE__);
}

void mlCreateIndexedMesh(ml_mesh* mesh, size_t n, void* data, size_t ilen, GLenum indextype, void* indices, GLenum flags) {
	size_t stride =
		((flags & ML_POS_2F) ? 8 : 0) +
		((flags & ML_POS_3F) ? 12 : 0) +
		((flags & ML_POS_4UB) ? 4 : 0) +
		((flags & ML_CLR_4UB) ? 4 : 0) +
		((flags & ML_N_3F) ? 12 : 0) +
		((flags & ML_N_4UB) ? 4 : 0) +
		((flags & ML_TC_2F) ? 8 : 0);
	glGenBuffers(1, &mesh->vbo);
	glGenBuffers(1, &mesh->ibo);

	size_t isize = 0;
	switch (indextype) {
	case GL_UNSIGNED_BYTE:
		isize = 1; break;
	case GL_UNSIGNED_SHORT:
		isize = 2; break;
	case GL_UNSIGNED_INT:
		isize = 4; break;
	default:
		roamError("indextype must be one of GL_UNSIGNED_[BYTE|SHORT|INT]");
	}

	glBindBuffer(GL_ARRAY_BUFFER, mesh->vbo);
	glBufferData(GL_ARRAY_BUFFER, n * stride, data, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh->ibo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, ilen * isize, indices, GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	GLint offset = 0;
	mesh->position = (flags & (ML_POS_2F + ML_POS_3F + ML_POS_4UB)) ? offset : -1;
	offset += (flags & ML_POS_2F) ? 8 : ((flags & ML_POS_3F) ? 12 : (flags & ML_POS_4UB ? 4 : 0));
	mesh->normal = (flags & (ML_N_3F + ML_N_4UB)) ? offset : -1;
	offset += (flags & ML_N_3F) ? 12 : ((flags & ML_N_4UB) ? 4 : 0);
	mesh->texcoord = (flags & ML_TC_2F) ? offset : -1;
	offset += (flags & ML_TC_2F) ? 8 : 0;
	mesh->color = (flags & ML_CLR_4UB) ? offset : -1;
	offset += (flags & ML_CLR_4UB) ? 4 : 0;
	mesh->stride = stride;
	mesh->mode = GL_TRIANGLES;
	mesh->count = ilen;
	mesh->flags = flags;
	mesh->ibotype = indextype;
	glCheck(__LINE__);
}


void mlCreateRenderable(ml_renderable* renderable, const ml_material* material, const ml_mesh* mesh) {
	glGenVertexArrays(1, &renderable->vao);
	glBindVertexArray(renderable->vao);
	renderable->material = material;
	renderable->mesh = mesh;

	glBindBuffer(GL_ARRAY_BUFFER, mesh->vbo);

	if (mesh->ibo > 0)
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh->ibo);

	if (material->position > -1) {
		if (mesh->position > -1) {
			if (mesh->flags & ML_POS_2F)
				glVertexAttribPointer(material->position, 2, GL_FLOAT, GL_FALSE, mesh->stride, (void*)((ptrdiff_t)mesh->position));
			else if (mesh->flags & ML_POS_3F)
				glVertexAttribPointer(material->position, 3, GL_FLOAT, GL_FALSE, mesh->stride, (void*)((ptrdiff_t)mesh->position));
			else // 4ub
				glVertexAttribPointer(material->position, 4, GL_UNSIGNED_BYTE, GL_TRUE, mesh->stride, (void*)((ptrdiff_t)mesh->position));
			glEnableVertexAttribArray(material->position);
		} else {
			glDisableVertexAttribArray(material->position);
		}
	}
	if (material->normal > -1) {
		if (mesh->normal > -1) {
			if (mesh->flags & ML_N_3F)
				glVertexAttribPointer(material->normal, 3, GL_FLOAT, GL_FALSE, mesh->stride, (void*)((ptrdiff_t)mesh->normal));
			else // 4ub
				glVertexAttribPointer(material->normal, 4, GL_UNSIGNED_BYTE, GL_TRUE, mesh->stride, (void*)((ptrdiff_t)mesh->normal));
			glEnableVertexAttribArray(material->normal);
		} else {
			glDisableVertexAttribArray(material->normal);
		}
	}
	if (material->texcoord > -1) {
		if (mesh->texcoord > -1) {
			glVertexAttribPointer(material->texcoord, 2, GL_FLOAT, GL_FALSE, mesh->stride, (void*)((ptrdiff_t)mesh->texcoord));
			glEnableVertexAttribArray(material->texcoord);
		} else {
			glDisableVertexAttribArray(material->texcoord);
		}
	}
	if (material->color > -1) {
		if (mesh->color > -1) {
			glVertexAttribPointer(material->color, GL_BGRA, GL_UNSIGNED_BYTE, GL_TRUE, mesh->stride, (void*)((ptrdiff_t)mesh->color));
			glEnableVertexAttribArray(material->color);
		} else {
			glDisableVertexAttribArray(material->color);
			glVertexAttrib4Nub(material->color, 0xff, 0xff, 0xff, 0xff);
		}
	}

	glBindVertexArray(0);
	glCheck(__LINE__);
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

void mlLoadTexture2D(ml_tex2d* tex, const char* filename) {
	int x, y, n;
	unsigned char* data;

	memset(tex, 0, sizeof(ml_tex2d));
	data = stbi_load(filename, &x, &y, &n, 0);
	if (data == NULL) {
		roamError("Failed to load image %s", filename);
	}

	glActiveTexture(GL_TEXTURE0);
	glGenTextures(1, &tex->id);
	glBindTexture(GL_TEXTURE_2D, tex->id);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    tex->w = x;
    tex->h = y;

    switch (n) {
    case 4:
	    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, x, y, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
	    break;
    case 3:
	    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, x, y, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
	    break;
    case 1:
	    glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, x, y, 0, GL_RED, GL_UNSIGNED_BYTE, data);
	    break;
    default:
	    roamError("bad pixel depth %d for %s", n, filename);
    }

	stbi_image_free(data);
	glBindTexture(GL_TEXTURE_2D, 0);
}

void mlFreeTexture2D(ml_tex2d* tex) {
	glDeleteTextures(1, &tex->id);
	memset(tex, 0, sizeof(ml_tex2d));

}

void mlBindTexture2D(ml_tex2d* tex, int index) {
	glActiveTexture(GL_TEXTURE0 + index);
	glBindTexture(GL_TEXTURE_2D, tex->id);
}


