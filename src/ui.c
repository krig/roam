#include "common.h"
#include "math3d.h"
#include "ui.h"

// UI drawing
static ml_material* ui_material = NULL;
static ml_tex2d ui_8x8font;
static GLint ui_screensize_index = -1;
static GLint ui_tex0_index = -1;
static GLuint ui_vao = 0;
static GLuint ui_vbo = 0;
static GLsizei ui_count = 0;
static float ui_scale = 1;
#define MAX_UI_VERTICES 2048
static ml_vtx_ui ui_vertices[MAX_UI_VERTICES];

// debug 3d drawing
#define MAX_DEBUG_LINEVERTS 2048
static ml_material* debug_material = NULL;
static GLint debug_projmat_index = -1;
static GLint debug_modelview_index = -1;
static ml_vtx_pos_clr debug_lines[MAX_DEBUG_LINEVERTS];
static size_t debug_linevertcount = 0;
static GLuint debug_vao = -1;
static GLuint debug_vbo = -1;


void uiInit(ml_material* uimat, ml_material* debugmat) {
	ui_material = uimat;
	ui_screensize_index = glGetUniformLocation(uimat->program, "screensize");
	ui_tex0_index = glGetUniformLocation(uimat->program, "tex0");

	debug_material = debugmat;
	debug_projmat_index = glGetUniformLocation(debugmat->program, "projmat");
	debug_modelview_index = glGetUniformLocation(debugmat->program, "modelview");
	printf("projmat: %d, modelview: %d\n", debug_projmat_index, debug_modelview_index);

	mlLoadTexture2D(&ui_8x8font, "data/8x8font.png");

	glGenBuffers(1, &ui_vbo);
	glGenVertexArrays(1, &ui_vao);
	glBindVertexArray(ui_vao);
	glBindBuffer(GL_ARRAY_BUFFER, ui_vbo);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(ml_vtx_ui), 0);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(ml_vtx_ui), (void*)(1 * sizeof(ml_vec2)));
	glVertexAttribPointer(2, GL_BGRA, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(ml_vtx_ui), (void*)(2 * sizeof(ml_vec2)));
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glEnableVertexAttribArray(2);
	glBindVertexArray(0);
	ui_count = 0;

	glGenBuffers(1, &debug_vbo);
	glGenVertexArrays(1, &debug_vao);
	glBindVertexArray(debug_vao);
	glBindBuffer(GL_ARRAY_BUFFER, debug_vbo);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(ml_vtx_pos_clr), 0);
	glVertexAttribPointer(1, GL_BGRA, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(ml_vtx_pos_clr), (void*)(sizeof(ml_vec3)));
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glBindVertexArray(0);
	debug_linevertcount = 0;
}

void uiExit() {
	mlDestroyTexture2D(&ui_8x8font);
	glDeleteBuffers(1, &ui_vbo);
	glDeleteVertexArrays(1, &ui_vao);
	glDeleteBuffers(1, &debug_vbo);
	glDeleteVertexArrays(1, &debug_vao);
}

void uiDraw(SDL_Point* viewport) {
	if (ui_count > 0) {
		ml_vec2 screensize = { (float)viewport->x, (float)viewport->y };
		glEnable(GL_TEXTURE_2D);
		glEnable(GL_BLEND);
//		glDisable(GL_CULL_FACE);
		glDisable(GL_DEPTH_TEST);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glUseProgram(ui_material->program);
		mlBindTexture2D(&ui_8x8font, 0);
		glUniform2fv(ui_screensize_index, 1, (float*)&screensize);
		glUniform1i(ui_tex0_index, 0);
		glBindVertexArray(ui_vao);
		glBindBuffer(GL_ARRAY_BUFFER, ui_vbo);
		glBufferData(GL_ARRAY_BUFFER, (unsigned long)(ui_count)*sizeof(ml_vtx_ui), ui_vertices, GL_DYNAMIC_DRAW);
		glDrawArrays(GL_TRIANGLES, 0, ui_count);
		glBindVertexArray(0);
		glUseProgram(0);
		glDisable(GL_BLEND);
//		glEnable(GL_CULL_FACE);
		glEnable(GL_DEPTH_TEST);
		glDisable(GL_TEXTURE_2D);
		ui_count = 0;
	}
}

void uiSetScale(float scale) {
	ui_scale = scale;
}

#define MAX_TEXT_LEN 256

void uiTextMeasure(int* x, int* y, const char* str, ...) {
	size_t len;
	char buf[MAX_TEXT_LEN];
	va_list va_args;
	va_start(va_args, str);
	vsnprintf(buf, MAX_TEXT_LEN, str, va_args);
	va_end(va_args);
	len = strlen(buf);
	int cx = 0, cy = 0, mx = 0;
	for (size_t i = 0; i < len; ++i) {
		if (buf[i] == '\n') {
			mx = (cx > mx) ? cx : mx;
			cx = 0;
			cy += 8;
		}
		cx += 8;
	}
	*x = mx;
	*y = cy;
}


void uiText(float x, float y, uint32_t clr, const char* str, ...) {
	char buf[MAX_TEXT_LEN];
	size_t len;
	float scale;
	float d;
	ml_vtx_ui* ptr = ui_vertices + ui_count;
	va_list va_args;
	va_start(va_args, str);
	vsnprintf(buf, MAX_TEXT_LEN, str, va_args);
	va_end(va_args);
	len = strlen(buf);
	if (ui_count + len*6 > MAX_UI_VERTICES)
		return;

	scale = ui_scale * 8.f;
	d = 8.f / 1024.f;

	ml_vec2 rpos = { x, y };
	for (size_t i = 0; i < len; ++i) {
		if (buf[i] < 0 || buf[i] > 127)
			continue;
		if (buf[i] == '\n') {
			rpos.x = x;
			rpos.y += scale;
			continue;
		}
		ml_vec2 tc = { buf[i] * d, 0.f };
		ml_vec2 p1 = { rpos.x, rpos.y };
		ml_vec2 p2 = { rpos.x + scale, rpos.y + scale };
		ml_vec2 t1 = { tc.x, 0.f };
		ml_vec2 t2 = { tc.x + d, 1.f };
		ml_vtx_ui quad[6] = {
			{ p1, { t1.x, t2.y }, clr },
			{ p2, { t2.x, t1.y }, clr },
			{ { p1.x, p2.y }, t1, clr },
			{ p2, { t2.x, t1.y }, clr },
			{ p1, { t1.x, t2.y }, clr },
			{ { p2.x, p1.y }, t2, clr },
		};

		memcpy(ptr, quad, sizeof(quad));
		ptr += 6;
		ui_count += 6;
		rpos.x += scale;
	}
}

void uiRect(float x, float y, float w, float h, uint32_t clr) {
	float tl = 0.5f / 1024.f;
	float br = 7.5f / 1024.f;
	if (ui_count + 6 > MAX_UI_VERTICES)
		return;
	ml_vtx_ui quad[6] = {
		{ { x, y }, { tl, 1.f }, clr },
		{ { x + w, y }, { br, 1.f }, clr },
		{ { x, y + h }, { tl, 0.f }, clr },
		{ { x, y + h }, { tl, 0.f }, clr },
		{ { x + w, y }, { br, 1.f }, clr },
		{ { x + w, y + h }, { br, 0.f }, clr },
	};
	memcpy(ui_vertices + ui_count, quad, sizeof(quad));
	ui_count += 6;
}


void uiDebugLine(ml_vec3 p1, ml_vec3 p2, uint32_t clr) {
	if (debug_linevertcount + 2 > MAX_DEBUG_LINEVERTS)
		return;
	debug_lines[debug_linevertcount + 0].pos = p1;
	debug_lines[debug_linevertcount + 0].clr = clr;
	debug_lines[debug_linevertcount + 1].pos = p2;
	debug_lines[debug_linevertcount + 1].clr = clr;
	debug_linevertcount += 2;

	//printf("(%.1f, %.1f, %.1f) - (%.1f, %.1f, %.1f)\n",
	//       p1.x, p1.y, p1.z, p2.x, p2.y, p2.z);
}

void uiDebugAABB(ml_vec3 minp, ml_vec3 maxp, uint32_t clr) {
	ml_vec3 corners[8] = {
		{ minp.x, minp.y, minp.z },
		{ maxp.x, minp.y, minp.z },
		{ minp.x, minp.y, maxp.z },
		{ maxp.x, minp.y, maxp.z },

		{ minp.x, maxp.y, minp.z },
		{ maxp.x, maxp.y, minp.z },
		{ minp.x, maxp.y, maxp.z },
		{ maxp.x, maxp.y, maxp.z },
	};
	uiDebugLine(corners[0], corners[1], clr);
	uiDebugLine(corners[1], corners[2], clr);
	uiDebugLine(corners[2], corners[3], clr);
	uiDebugLine(corners[3], corners[0], clr);
	uiDebugLine(corners[4], corners[5], clr);
	uiDebugLine(corners[5], corners[6], clr);
	uiDebugLine(corners[6], corners[7], clr);
	uiDebugLine(corners[7], corners[4], clr);
	uiDebugLine(corners[0], corners[4], clr);
	uiDebugLine(corners[1], corners[5], clr);
	uiDebugLine(corners[2], corners[6], clr);
	uiDebugLine(corners[3], corners[7], clr);
}

void uiDebugPoint(ml_vec3 p, uint32_t clr) {
	ml_vec3 p0, p1, p2, p3, p4, p5;
	p0 = p1 = p2 = p3 = p4 = p5 = p;
	p0.x -= 0.05f;
	p1.x += 0.05f;
	p2.y -= 0.05f;
	p3.y += 0.05f;
	p4.z -= 0.05f;
	p5.z += 0.05f;
	uiDebugLine(p0, p1, clr);
	uiDebugLine(p2, p3, clr);
	uiDebugLine(p4, p5, clr);
}

void uiDebugSphere(ml_vec3 p, float r, uint32_t clr) {
	#define SPHERE_NDIV 7

	uiDebugPoint(p, clr);

	for (int i = 0; i < SPHERE_NDIV; ++i) {
		float st = sin(((float)i / SPHERE_NDIV) * ML_TWO_PI);
		float ct = cos(((float)i / SPHERE_NDIV) * ML_TWO_PI);
		int i2 = i + 1;
		if (i2 == SPHERE_NDIV)
			i2 = 0;
		float st1 = sin(((float)i2 / SPHERE_NDIV) * ML_TWO_PI);
		float ct1 = cos(((float)i2 / SPHERE_NDIV) * ML_TWO_PI);
		ml_vec3 p1 = { p.x + r * st, p.y, p.z + r * ct };
		ml_vec3 p2 = { p.x + r * st1, p.y, p.z + r * ct1 };
		ml_vec3 p3 = { p.x + r * st, p.y + r * ct, p.z };
		ml_vec3 p4 = { p.x + r * st1, p.y + r * ct1, p.z };
		uiDebugLine(p1, p2, clr);
		uiDebugLine(p3, p4, clr);
	}
}

void uiDrawDebug(ml_matrixstack* projection, ml_matrixstack* modelview) {
	if (debug_linevertcount > 0) {
		glEnable(GL_BLEND);
		glEnable(GL_DEPTH_TEST);
		glDepthMask(GL_FALSE);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glUseProgram(debug_material->program);
		mlUniformMatrix(debug_projmat_index, mlGetMatrix(projection));
		mlUniformMatrix(debug_modelview_index, mlGetMatrix(modelview));
		glBindVertexArray(debug_vao);
		glBindBuffer(GL_ARRAY_BUFFER, debug_vbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(ml_vtx_pos_clr)*debug_linevertcount, debug_lines, GL_DYNAMIC_DRAW);
		glDrawArrays(GL_LINES, 0, debug_linevertcount);
		glBindVertexArray(0);
		glUseProgram(0);
		glDisable(GL_BLEND);
		glEnable(GL_DEPTH_TEST);
		glDepthMask(GL_TRUE);
		debug_linevertcount = 0;
	}
}

