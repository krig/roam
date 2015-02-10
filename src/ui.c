#include "common.h"
#include "math3d.h"
#include "ui.h"
#include "game.h"
#include "easing.h"
#include "stb.h"
#include "script.h"

// UI drawing
static material_t* ui_material = NULL;
static tex2d_t ui_font;
static GLint ui_screensize_index = -1;
static GLint ui_tex0_index = -1;
static GLuint ui_vao = 0;
static GLuint ui_vbo = 0;
static GLsizei ui_count = 0;
static float ui_scale = 1;
#define MAX_UI_VERTICES 8192
static uivert_t ui_vertices[MAX_UI_VERTICES];
static GLsizei ui_maxcount = 0;

// debug 3d drawing
#define MAX_DEBUG_LINEVERTS 8192
static material_t* debug_material = NULL;
static GLint debug_projmat_index = -1;
static GLint debug_modelview_index = -1;
static posclrvert_t debug_lines[MAX_DEBUG_LINEVERTS];
static size_t debug_linevertcount = 0;
static GLuint debug_vao = -1;
static GLuint debug_vbo = -1;
static size_t debug_maxcount = 0;

static bool console_enabled = false;
static bool console_first_char = true; // hack to discard toggle key
static float console_fade = 0.0f;
#define MAX_CONSOLE_INPUT 1024
#define CONSOLE_SCROLLBACK 100
static char console_cmdline[MAX_CONSOLE_INPUT];
static char console_scrollback[CONSOLE_SCROLLBACK][MAX_CONSOLE_INPUT];
static int console_scrollback_pos = 0;

#define UI_CHAR_W (9)
#define UI_CHAR_H (9)


void ui_init(material_t* uimat, material_t* debugmat)
{
	ui_material = uimat;
	ui_screensize_index = glGetUniformLocation(uimat->program, "screensize");
	ui_tex0_index = glGetUniformLocation(uimat->program, "tex0");

	debug_material = debugmat;
	debug_projmat_index = glGetUniformLocation(debugmat->program, "projmat");
	debug_modelview_index = glGetUniformLocation(debugmat->program, "modelview");
	printf("projmat: %d, modelview: %d\n", debug_projmat_index, debug_modelview_index);

	m_tex2d_load(&ui_font, "data/font.png");

	glGenBuffers(1, &ui_vbo);
	glGenVertexArrays(1, &ui_vao);
	glBindVertexArray(ui_vao);
	glBindBuffer(GL_ARRAY_BUFFER, ui_vbo);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(uivert_t), 0);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(uivert_t), (void*)(1 * sizeof(vec2_t)));
	glVertexAttribPointer(2, GL_BGRA, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(uivert_t), (void*)(2 * sizeof(vec2_t)));
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glEnableVertexAttribArray(2);
	glBindVertexArray(0);
	ui_count = 0;

	glGenBuffers(1, &debug_vbo);
	glGenVertexArrays(1, &debug_vao);
	glBindVertexArray(debug_vao);
	glBindBuffer(GL_ARRAY_BUFFER, debug_vbo);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(posclrvert_t), 0);
	glVertexAttribPointer(1, GL_BGRA, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(posclrvert_t), (void*)(sizeof(vec3_t)));
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glBindVertexArray(0);
	debug_linevertcount = 0;
}

void ui_exit()
{
	m_tex2d_destroy(&ui_font);
	glDeleteBuffers(1, &ui_vbo);
	glDeleteVertexArrays(1, &ui_vao);
	glDeleteBuffers(1, &debug_vbo);
	glDeleteVertexArrays(1, &debug_vao);
}

void ui_tick(float dt)
{
	if (console_enabled)
		console_fade = m_clamp(console_fade + dt*1.5f, 0, 1.f);
	else
		console_fade = m_clamp(console_fade - dt*1.5f, 0, 1.f);
}

bool ui_console_enabled()
{
	return console_enabled;
}


void ui_draw(SDL_Point* viewport)
{
	const int NLINES = 32;
	if (console_enabled || console_fade > 0) {
		int console_width = (640 > viewport->x) ? viewport->x : 640;
		size_t max_display = console_width/UI_CHAR_W - 1;
		size_t len, offset;
		float elastic_fade = enQuinticInOut(console_fade);
		uint32_t alpha = (uint32_t)(ML_MAX(elastic_fade, 0.1f)*255.5f);
		float yoffs = (float)(NLINES*UI_CHAR_H) * elastic_fade;
		ui_rect(0, viewport->y - (int)yoffs, console_width, NLINES*UI_CHAR_H, ((alpha * 5 / 6)<<24)|0x2c3e50);
		ui_rect(0, viewport->y - (int)yoffs - UI_CHAR_H - 4, console_width, UI_CHAR_H + 4, (alpha<<24)|0x34495e);
		int sby = viewport->y - (int)yoffs + 2;
		int sbpos = (console_scrollback_pos - 1) % CONSOLE_SCROLLBACK;
		if (sbpos < 0)
			sbpos = CONSOLE_SCROLLBACK - 1;
		while (sbpos != console_scrollback_pos && sby < viewport->y) {
			if (console_scrollback[sbpos] == '\0')
				break;
			len = strlen(console_scrollback[sbpos]);
			offset = (len > max_display) ? (len - max_display) : 0;
			ui_text(2, sby, (alpha<<24)|0xbdc3c7, "%s", console_scrollback[sbpos] + offset);
			sby += UI_CHAR_H;
			sbpos = (sbpos - 1) % CONSOLE_SCROLLBACK;
			if (sbpos < 0)
				sbpos = CONSOLE_SCROLLBACK - 1;
		}
		len = strlen(console_cmdline);
		offset = (len > (max_display - 1)) ? (len - (max_display - 1)) : 0;
		ui_text(2, viewport->y - (int)yoffs - UI_CHAR_H - 2, (alpha<<24)|0xeeeeec, "#%s", console_cmdline + offset);
	}

	if (ui_count > 0) {
		vec2_t screensize = { (float)viewport->x, (float)viewport->y };
		glEnable(GL_BLEND);
//		glDisable(GL_CULL_FACE);
		glDisable(GL_DEPTH_TEST);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glUseProgram(ui_material->program);
		m_tex2d_bind(&ui_font, 0);
		glUniform2fv(ui_screensize_index, 1, (float*)&screensize);
		glUniform1i(ui_tex0_index, 0);
		glBindVertexArray(ui_vao);
		glBindBuffer(GL_ARRAY_BUFFER, ui_vbo);
		if (ui_count > ui_maxcount) {
			glBufferData(GL_ARRAY_BUFFER, (unsigned long)(ui_count)*sizeof(uivert_t), ui_vertices, GL_DYNAMIC_DRAW);
			ui_maxcount = ui_count;
		} else {
			glBufferSubData(GL_ARRAY_BUFFER, 0, (unsigned long)(ui_count)*sizeof(uivert_t), ui_vertices);
		}
		glDrawArrays(GL_TRIANGLES, 0, ui_count);
		glBindVertexArray(0);
		glUseProgram(0);
		glDisable(GL_BLEND);
//		glEnable(GL_CULL_FACE);
		glEnable(GL_DEPTH_TEST);
		ui_count = 0;
	}
}

void ui_set_scale(float scale)
{
	ui_scale = scale;
}

#define MAX_TEXT_LEN 256

void ui_text_measure(int* w, int* h, const char* str, ...)
{
	char buf[MAX_TEXT_LEN];
	va_list va_args;
	va_start(va_args, str);
	vsnprintf(buf, MAX_TEXT_LEN, str, va_args);
	va_end(va_args);
	int scale = ui_scale * UI_CHAR_H;

	size_t len;
	len = strlen(buf);
	int cx = 0, cy = scale, mx = 0;
	for (size_t i = 0; i < len; ++i) {
		if (buf[i] == '\n') {
			mx = (cx > mx) ? cx : mx;
			cx = 0;
			cy += scale;
		}
		cx += scale;
	}
	*w = mx;
	*h = cy;
}


void ui_text(float x, float y, uint32_t clr, const char* str, ...)
{
	char buf[MAX_TEXT_LEN];
	size_t len;
	int scale;
	float d, v;
	uivert_t* ptr = ui_vertices + ui_count;
	va_list va_args;
	va_start(va_args, str);
	vsnprintf(buf, MAX_TEXT_LEN, str, va_args);
	va_end(va_args);
	len = strlen(buf);
	if (ui_count + len*6 > MAX_UI_VERTICES)
		return;

	scale = ui_scale * UI_CHAR_H;

	int cx = 0, w = 0, h = scale;
	for (size_t i = 0; i < len; ++i) {
		if (buf[i] == '\n') {
			w = (cx > w) ? cx : w;
			cx = 0;
			h += scale;
		}
		cx += scale;
	}

	d = (float)UI_CHAR_W / (float)ui_font.w;
	v = (float)UI_CHAR_H / (float)ui_font.h;

	vec2_t rpos = { x, y };
	for (size_t i = 0; i < len; ++i) {
		if (buf[i] < 0 || buf[i] > 127)
			continue;
		if (buf[i] == '\n') {
			rpos.x = x;
			rpos.y -= scale;
			continue;
		} else if (buf[i] == ' ') {
			rpos.x += scale;
			continue;
		}
		vec2_t tc = { (buf[i] - ' ') * d, 0.f };
		vec2_t p1 = { rpos.x, rpos.y };
		vec2_t p2 = { rpos.x + scale, rpos.y + scale };
		vec2_t t1 = { tc.x, 0.f };
		vec2_t t2 = { tc.x + d, v };
		uivert_t quad[6];
		quad[0].pos = p1,                           quad[0].tc.x = t1.x, quad[0].tc.y = t2.y, quad[0].clr = clr;
		quad[1].pos = p2,                           quad[1].tc.x = t2.x, quad[1].tc.y = t1.y, quad[1].clr = clr;
		quad[2].pos.x = p1.x, quad[2].pos.y = p2.y, quad[2].tc = t1,                          quad[2].clr = clr;
		quad[3].pos = p2,                           quad[3].tc.x = t2.x, quad[3].tc.y = t1.y, quad[3].clr = clr;
		quad[4].pos = p1,                           quad[4].tc.x = t1.x, quad[4].tc.y = t2.y, quad[4].clr = clr;
		quad[5].pos.x = p2.x, quad[5].pos.y = p1.y, quad[5].tc = t2,                          quad[5].clr = clr;

		memcpy(ptr, quad, sizeof(quad));
		ptr += 6;
		ui_count += 6;
		rpos.x += scale;
	}
}

void ui_rect(float x, float y, float w, float h, uint32_t clr)
{
	float tl = 0.5f / (float)ui_font.w;
	float br = 7.5f / (float)ui_font.w;
	float v = 2.f / (float)ui_font.h;
	if (ui_count + 6 > MAX_UI_VERTICES)
		return;
	uivert_t quad[6] = {
		{ { x, y }, { tl, v }, clr },
		{ { x + w, y }, { br, v }, clr },
		{ { x, y + h }, { tl, 0 }, clr },
		{ { x, y + h }, { tl, 0 }, clr },
		{ { x + w, y }, { br, v }, clr },
		{ { x + w, y + h }, { br, 0 }, clr },
	};
	memcpy(ui_vertices + ui_count, quad, sizeof(quad));
	ui_count += 6;
}


void ui_debug_line(vec3_t p1, vec3_t p2, uint32_t clr)
{
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

void ui_debug_aabb(vec3_t center, vec3_t extent, uint32_t clr)
{
	vec3_t maxp = m_vec3add(center, extent);
	vec3_t minp = m_vec3sub(center, extent);
	vec3_t corners[8] = {
		{ minp.x, minp.y, minp.z },
		{ maxp.x, minp.y, minp.z },
		{ maxp.x, minp.y, maxp.z },
		{ minp.x, minp.y, maxp.z },

		{ minp.x, maxp.y, minp.z },
		{ maxp.x, maxp.y, minp.z },
		{ maxp.x, maxp.y, maxp.z },
		{ minp.x, maxp.y, maxp.z },
	};
	ui_debug_line(corners[0], corners[1], clr);
	ui_debug_line(corners[1], corners[2], clr);
	ui_debug_line(corners[2], corners[3], clr);
	ui_debug_line(corners[3], corners[0], clr);

	ui_debug_line(corners[4], corners[5], clr);
	ui_debug_line(corners[5], corners[6], clr);
	ui_debug_line(corners[6], corners[7], clr);
	ui_debug_line(corners[7], corners[4], clr);

	ui_debug_line(corners[0], corners[4], clr);
	ui_debug_line(corners[1], corners[5], clr);
	ui_debug_line(corners[2], corners[6], clr);
	ui_debug_line(corners[3], corners[7], clr);
}

void ui_debug_block(ivec3_t block, uint32_t clr)
{
	vec3_t pos;
	vec3_t ext;
	chunkpos_t camera = player_chunk();
	m_setvec3(pos,
	             block.x - camera.x*CHUNK_SIZE,
	             block.y,
	             block.z - camera.z*CHUNK_SIZE);
	m_setvec3(ext, 0.5f, 0.5f, 0.5f);
	ui_debug_aabb(pos, ext, clr);
}


void ui_debug_point(vec3_t p, uint32_t clr)
{
	vec3_t p0, p1, p2, p3, p4, p5;
	p0 = p1 = p2 = p3 = p4 = p5 = p;
	p0.x -= 0.05f;
	p1.x += 0.05f;
	p2.y -= 0.05f;
	p3.y += 0.05f;
	p4.z -= 0.05f;
	p5.z += 0.05f;
	ui_debug_line(p0, p1, clr);
	ui_debug_line(p2, p3, clr);
	ui_debug_line(p4, p5, clr);
}

void ui_debug_sphere(vec3_t p, float r, uint32_t clr)
{
	#define SPHERE_NDIV 7

	ui_debug_point(p, clr);

	for (int i = 0; i < SPHERE_NDIV; ++i) {
		float st = sin(((float)i / SPHERE_NDIV) * ML_TWO_PI);
		float ct = cos(((float)i / SPHERE_NDIV) * ML_TWO_PI);
		int i2 = i + 1;
		if (i2 == SPHERE_NDIV)
			i2 = 0;
		float st1 = sin(((float)i2 / SPHERE_NDIV) * ML_TWO_PI);
		float ct1 = cos(((float)i2 / SPHERE_NDIV) * ML_TWO_PI);
		vec3_t p1 = { p.x + r * st, p.y, p.z + r * ct };
		vec3_t p2 = { p.x + r * st1, p.y, p.z + r * ct1 };
		vec3_t p3 = { p.x + r * st, p.y + r * ct, p.z };
		vec3_t p4 = { p.x + r * st1, p.y + r * ct1, p.z };
		ui_debug_line(p1, p2, clr);
		ui_debug_line(p3, p4, clr);
	}
}

void ui_draw_debug(mtxstack_t* projection, mtxstack_t* modelview)
{
	if (debug_linevertcount > 0) {
		glEnable(GL_BLEND);
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LEQUAL);
		glDepthMask(GL_FALSE);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glUseProgram(debug_material->program);
		m_uniform_mat44(debug_projmat_index, m_getmatrix(projection));
		m_uniform_mat44(debug_modelview_index, m_getmatrix(modelview));
		glBindVertexArray(debug_vao);
		glBindBuffer(GL_ARRAY_BUFFER, debug_vbo);
		if (debug_linevertcount > debug_maxcount) {
			glBufferData(GL_ARRAY_BUFFER, sizeof(posclrvert_t)*debug_linevertcount, debug_lines, GL_DYNAMIC_DRAW);
			debug_maxcount = debug_linevertcount;
		} else {
			glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(posclrvert_t)*debug_linevertcount, debug_lines);
		}
		glDrawArrays(GL_LINES, 0, debug_linevertcount);
		glBindVertexArray(0);
		glUseProgram(0);
		glDisable(GL_BLEND);
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LESS);
		glDepthMask(GL_TRUE);
		debug_linevertcount = 0;
	}
}

void ui_console_toggle(bool enable)
{
	console_enabled = enable;
	if (console_enabled) {
		SDL_StartTextInput();
	} else {
		SDL_StopTextInput();
	}
}

void ui_add_console_line(const char* txt)
{
	strmcpy(console_scrollback[console_scrollback_pos], txt, MAX_CONSOLE_INPUT);
	console_scrollback_pos = (console_scrollback_pos + 1) % CONSOLE_SCROLLBACK;
	printf("> %s\n", txt);
}

void ui_execute_console_command(char* cmd)
{
	int ret = script_exec(cmd);
	if (ret != 0) {
		printf("TODO: handle ret != 0 from script command (ret = %d)\n", ret);
	}
}

bool ui_console_handle_event(SDL_Event* event)
{
	if (!console_enabled)
		return false;
	switch (event->type) {
	case SDL_KEYDOWN:
		if (event->key.keysym.sym == SDLK_BACKQUOTE || event->key.keysym.sym == SDLK_ESCAPE) {
			ui_console_toggle(false);
		} else if (event->key.keysym.sym == SDLK_RETURN) {
			ui_execute_console_command(console_cmdline);
			memset(console_cmdline, 0, MAX_CONSOLE_INPUT);
		} else if (event->key.keysym.sym == SDLK_BACKSPACE) {
			char* s = console_cmdline;
			size_t len = strlen(s);
			while (len > 0) {
				if ((s[len-1] & 0x80) == 0) {
					s[len-1] = 0;
					break;
				}
				if ((s[len-1] & 0xc0) == 0x80) {
					/* byte from a multibyte sequence */
					s[len-1] = 0;
					--len;
				}
				if ((s[len-1] & 0xc0) == 0xc0) {
					/* first byte of a multibyte sequence */
					s[len-1] = 0;
					break;
				}
			}
		}
		return true;
	case SDL_TEXTINPUT:
		//printf("%s (%c) (%d)\n", event->text.text, event->text.text[0], event->text.text[0]);
		if (console_first_char) {
			console_first_char = false;
			printf("Discarding %s\n", event->text.text);
			return true;
		}
		if (event->text.text[0] < 0) {
			strmcat(console_cmdline, "?", MAX_CONSOLE_INPUT);
		} else {
			strmcat(console_cmdline, event->text.text, MAX_CONSOLE_INPUT);
		}
		return true;
	}
	return false;
}

