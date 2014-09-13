#include "common.h"
#include "math3d.h"
#include "ui.h"

static ml_material* ui_material = NULL;
static ml_tex2d ui_8x8font;
static GLint ui_screensize_index = -1;
static GLint ui_tex0_index = -1;
static GLuint ui_vao = -1;
static GLuint ui_vbo = -1;
static size_t ui_count = 0;

#define MAX_UI_VERTICES 2048
static ml_vtx_ui ui_vertices[MAX_UI_VERTICES];

void uiInit(ml_material* m) {
	memset(ui_vertices, 0, sizeof(ml_vtx_ui)*MAX_UI_VERTICES);
	ui_material = m;
	ui_screensize_index = glGetUniformLocation(m->program, "screensize");
	ui_tex0_index = glGetUniformLocation(m->program, "tex0");
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
}

void uiExit() {
	mlFreeTexture2D(&ui_8x8font);
	glDeleteBuffers(1, &ui_vbo);
	glDeleteVertexArrays(1, &ui_vao);
}

void uiBegin() {
	ui_count = 0;
}

void uiDraw(SDL_Point* viewport) {
	if (ui_count <= 0)
		return;

	ml_vec2 screensize = {(float)viewport->x, (float)viewport->y};

	glEnable(GL_BLEND);
	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glUseProgram(ui_material->program);
	mlBindTexture2D(&ui_8x8font, 0);
	glUniform2fv(ui_screensize_index, 1, (float*)&screensize);
	glUniform1i(ui_tex0_index, 0);
	glBindVertexArray(ui_vao);
	glBindBuffer(GL_ARRAY_BUFFER, ui_vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(ml_vtx_ui)*ui_count, ui_vertices, GL_DYNAMIC_DRAW);
	glDrawArrays(GL_TRIANGLES, 0, ui_count);
	glBindVertexArray(0);
	glUseProgram(0);
	glDisable(GL_BLEND);
	glEnable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);
}

void uiText(float x, float y, uint32_t clr, const char* str, ...) {
	char buf[256];
	size_t len;
	float scale;
	float d;
	ml_vtx_ui* ptr = ui_vertices + ui_count;
	va_list va_args;
	va_start(va_args, str);
	vsnprintf(buf, 256, str, va_args);
	va_end(va_args);
	len = strlen(buf);
	if (ui_count + len*6 > MAX_UI_VERTICES)
		return;

	scale = 16.f;
	d = 8.f / 1024.f;

	ml_vec2 rpos = {x, y};
	for (size_t i = 0; i < len; ++i) {
		if (buf[i] < 0 || buf[i] > 127)
			continue;
		if (buf[i] == '\n') {
			rpos.x = x;
			rpos.y += scale;
			continue;
		}
		ml_vec2 tc = {buf[i] * d, 0.f};
		ml_vec2 p1 = {rpos.x, rpos.y};
		ml_vec2 p2 = {rpos.x + scale, rpos.y + scale};
		ml_vec2 t1 = {tc.x, 0.f};
		ml_vec2 t2 = {tc.x + d, 1.f};

		ptr->pos = p1; ptr->tc.x = t1.x; ptr->tc.y = t2.y; ptr->clr = clr;
		++ptr; ++ui_count;
		ptr->pos = p2; ptr->tc.x = t2.x; ptr->tc.y = t1.y; ptr->clr = clr;
		++ptr; ++ui_count;
		ptr->pos.x = p1.x; ptr->pos.y = p2.y; ptr->tc = t1; ptr->clr = clr;
		++ptr; ++ui_count;

		ptr->pos = p2; ptr->tc.x = t2.x; ptr->tc.y = t1.y; ptr->clr = clr;
		++ptr; ++ui_count;
		ptr->pos = p1; ptr->tc.x = t1.x; ptr->tc.y = t2.y; ptr->clr = clr;
		++ptr; ++ui_count;
		ptr->pos.x = p2.x; ptr->pos.y = p1.y; ptr->tc = t2; ptr->clr = clr;
		++ptr; ++ui_count;

		rpos.x += scale;
	}
}
