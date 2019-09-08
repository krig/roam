#include <time.h>
#include "common.h"
#include "math3d.h"
#include "shaders.h"
#include "ui.h"
#include "objfile.h"
#include "noise.h"
#include "map.h"
#include "game.h"
#include "geometry.h"
#include "u8.h"
#include "sky.h"
#include "stb.h"
#include "easing.h"
#include "script.h"


static SDL_Window* window;
static SDL_GLContext context;
static bool mouse_captured = false;
struct game game;
tex2d_t blocks_texture;
extern bool alpha_sort_chunks;
SDL_Point game_viewport;


static
void vsync_onoff(int argc, char** argv)
{
	if (argc != 1)
		return;
	const char* onoff = argv[1];
	if (stb_stricmp(onoff, "on") == 0)
		SDL_GL_SetSwapInterval(1);
	else if (stb_stricmp(onoff, "off") == 0)
		SDL_GL_SetSwapInterval(0);
	else if (stb_stricmp(onoff, "tear") == 0) {
		int sw = SDL_GL_SetSwapInterval(-1);
		if (sw == -1) SDL_GL_SetSwapInterval(1);
	}
}


static
void game_init()
{
	script_init();
	script_defun("vsync", vsync_onoff);
	game.camera.pitch = 0;
	game.camera.yaw = 0;
	m_setvec3(game.camera.pos, 0, 0, 0);
	game.fast_day_mode = false;
	game.debug_mode = true;
	game.collisions_on = true;
	game.camera.mode = CAMERA_FPS;
	game.enable_ground = true;
	game.wireframe = false;
	script_dofile("data/boot.script");

	struct controls default_controls = {
		.left = SDLK_a,
		.right = SDLK_d,
		.forward = SDLK_w,
		.backward = SDLK_s,
		.jump = SDLK_SPACE,
		.sprint = SDLK_LCTRL,
		.crouch = SDLK_LSHIFT,
		.interact = SDLK_e,
		.primary_action = SDL_BUTTON_LEFT,
		.secondary_action = SDL_BUTTON_RIGHT,
		.wireframe = SDLK_o,
		.debuginfo = SDLK_p
	};
	game.controls = default_controls;

	memset(&game.input, 0, sizeof(struct inputstate));

	printf("* Load materials + UI\n");
	m_create_material(&game.materials[MAT_BASIC], basic_vshader, basic_fshader);
	m_create_material(&game.materials[MAT_UI], ui_vshader, ui_fshader);
	m_create_material(&game.materials[MAT_DEBUG], debug_vshader, debug_fshader);
	m_create_material(&game.materials[MAT_CHUNK], chunk_vshader, chunk_fshader);
	m_create_material(&game.materials[MAT_CHUNK_ALPHA], chunk_vshader, chunkalpha_fshader);
	m_create_material(&game.materials[MAT_SKY], sky_vshader, sky_fshader);
	ui_init(game.materials + MAT_UI, game.materials + MAT_DEBUG);
	m_tex2d_load(&blocks_texture, "data/blocks8-v1.png");

	game.day = 0;
	game.time_of_day = 0;

	sky_init();
	player_init();
	map_init();
	player_move_to_spawn();

	mouse_captured = false;
}


static
void game_exit()
{
	map_exit();
	sky_exit();
	ui_exit();
	for (int i = 0; i < MAX_MATERIALS; ++i)
		m_destroy_material(game.materials + i);
	m_mtxstack_destroy(&game.projection);
	m_mtxstack_destroy(&game.modelview);
	m_tex2d_destroy(&blocks_texture);
	script_exit();
}


static
void capture_mouse(bool capture)
{
	int cval = capture ? SDL_TRUE : SDL_FALSE;
	SDL_SetRelativeMouseMode(cval);
	SDL_SetWindowGrab(window, cval);
	mouse_captured = capture;
}

static
bool handle_event(SDL_Event* event)
{
	if (ui_console_handle_event(event))
		return true;

	switch (event->type) {
	case SDL_QUIT:
		return false;
	case SDL_KEYDOWN: {
		SDL_Keycode sym = event->key.keysym.sym;
		if (sym == SDLK_ESCAPE) {
			if (!mouse_captured)
				return false;
			else
				capture_mouse(false);
		}
		else if (sym == game.controls.wireframe)
			game.wireframe = !game.wireframe;
		else if (sym == SDLK_F1)
			game.debug_mode = !game.debug_mode;
		else if (sym == SDLK_F3)
			game.enable_ground = !game.enable_ground;
		else if (sym == SDLK_F4)
			game.camera.mode = (game.camera.mode + 1) % NUM_CAMERA_MODES;
		else if (sym == SDLK_F5)
			game.collisions_on = !game.collisions_on;
		else if (sym == SDLK_F6) {
			alpha_sort_chunks = !alpha_sort_chunks;
			printf("sort chunks: %d\n", alpha_sort_chunks);
			ui_add_console_line(stb_sprintf("sort chunks: %d", alpha_sort_chunks));
		}
		else if (sym == SDLK_F10) {
			char name[512];
			char date[64];
			static int cnt = 0;
			time_t t;
			struct tm *tmp;
			t = time(NULL);
			tmp = localtime(&t);
			strftime(date, sizeof(date), "%Y-%m-%d", tmp);
			do {
				snprintf(name, sizeof(name), "roam-%s-%d.png", date, cnt);
				++cnt;
			} while (sys_isfile(name));
			m_save_screenshot(name);
			ui_add_console_line(stb_sprintf("Saved %s.", name));
		}
		else if (sym == SDLK_BACKQUOTE)
			ui_console_toggle(true);
		else if (sym == SDLK_F2)
			game.fast_day_mode = true;
		else if (sym == game.controls.sprint)
			game.input.move_sprint = true;
		else if (sym == game.controls.left)
			game.input.move_left = true;
		else if (sym == game.controls.right)
			game.input.move_right = true;
		else if (sym == game.controls.forward)
			game.input.move_forward = true;
		else if (sym == game.controls.backward)
			game.input.move_backward = true;
		else if (sym == game.controls.jump)
			game.input.move_jump = true;
		else if (sym == game.controls.crouch)
			game.input.move_crouch = true;
	} break;
	case SDL_KEYUP: {
		SDL_Keycode sym = event->key.keysym.sym;
		if (sym == SDLK_F2)
			game.fast_day_mode = false;
		else if (sym == game.controls.sprint)
			game.input.move_sprint = false;
		else if (sym == game.controls.left)
			game.input.move_left = false;
		else if (sym == game.controls.right)
			game.input.move_right = false;
		else if (sym == game.controls.forward)
			game.input.move_forward = false;
		else if (sym == game.controls.backward)
			game.input.move_backward = false;
		else if (sym == game.controls.jump)
			game.input.move_jump = false;
		else if (sym == game.controls.crouch)
			game.input.move_crouch = false;
	} break;
	case SDL_MOUSEBUTTONDOWN:
		switch (event->button.button) {
		case SDL_BUTTON_LEFT: {
			if (!mouse_captured) {
				printf("focus gained\n");
				capture_mouse(true);
			} else {
				printf("deleting picked block (%d, %d, %d)\n",
				       game.input.picked_block.x,
				       game.input.picked_block.y,
				       game.input.picked_block.z);
				if (blocktype_by_coord(game.input.picked_block) != BLOCK_AIR) {
					printf("can delete.\n");
					map_update_block(game.input.picked_block, BLOCK_AIR);
				}
			}
		} break;
		case SDL_BUTTON_RIGHT: {
			printf("creating block (%d, %d, %d)\n",
				game.input.prepicked_block.x,
				game.input.prepicked_block.y,
				game.input.prepicked_block.z);

			ivec3_t feet = player_block();
			ivec3_t head = feet;
			head.y += 1;
			if (blocktype_by_coord(game.input.prepicked_block) == BLOCK_AIR &&
			    !block_eq(head, game.input.prepicked_block) &&
			    !block_eq(feet, game.input.prepicked_block)) {
				printf("can create.\n");
				map_update_block(game.input.prepicked_block, BLOCK_TEST_ALPHA);
			}
		} break;
		} break;
	case SDL_MOUSEMOTION: {
		if (mouse_captured) {
			game.input.mouse_xrel += event->motion.xrel;
			game.input.mouse_yrel += event->motion.yrel;
		}
	} break;
	default:
		break;
	}
	return true;
}


static
void camera_tick(float dt)
{
	// offset camera from position
	struct player *p = &game.player;
	struct camera *cam = &game.camera;
	struct playervars* pv = player_vars();
	vec3_t offset = {0, 0, 0};
	switch (cam->mode) {
	case CAMERA_FLIGHT:
		offset.y = pv->camoffset;
		break;
	case CAMERA_3RDPERSON: {
		vec3_t x, y, z;
		m_fps_rotation(cam->pitch, cam->yaw, &x, &y, &z);
		offset = m_vec3scale(z, 6.f);
	} break;
	default: {
		float d = enCubicInOut(p->crouch_fade);
		offset.y = pv->camoffset * (1.f - d) + pv->crouchcamoffset * d;
	} break;
	}
	cam->pos.x = p->pos.x + offset.x;
	cam->pos.y = p->pos.y + offset.y;
	cam->pos.z = p->pos.z + offset.z;
}


static
void game_tick(float dt)
{
	player_tick(dt);
	script_tick();
	camera_tick(dt);
	ui_tick(dt);
	map_tick();
	sky_tick(dt);
	// update player/input
	// update blocks
	// update creatures
	// update effects

}

static
char* read_file(const char* name)
{
	FILE* f = fopen(name, "r");
	char* data = NULL;
	if (f == NULL)
		return NULL;
	fseek(f, 0, SEEK_END);
	long len = ftell(f);
	fseek(f, 0, SEEK_SET);
	data = calloc(len+1, 1);
	if (data == NULL)
		goto exit;
	size_t r = fread(data, 1, len, f);
	if (r != (size_t)len) {
		free(data);
		data = NULL;
	}
exit:
	fclose(f);
	return data;
}

GLuint fbo, fbo_texture, rbo_depth;
GLuint vbo_fbo_vertices, vao_quad;
GLint program_postproc, attribute_v_coord_postproc, uniform_fbo_texture;

static
void init_fbo_resources(void)
{
	GLuint vs;
	GLuint fs;
	GLint link_ok, validate_ok;
	char* vshader = read_file("data/postproc.vert");
	char* fshader = read_file("data/postproc.frag");


	M_CHECKGL(glActiveTexture(GL_TEXTURE0));
	M_CHECKGL(glGenTextures(1, &fbo_texture));
	M_CHECKGL(glBindTexture(GL_TEXTURE_2D, fbo_texture));
	M_CHECKGL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
	M_CHECKGL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
	M_CHECKGL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
	M_CHECKGL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
	M_CHECKGL(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, game_viewport.x, game_viewport.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL));
	M_CHECKGL(glBindTexture(GL_TEXTURE_2D, 0));
	M_CHECKGL(glGenRenderbuffers(1, &rbo_depth));
	M_CHECKGL(glBindRenderbuffer(GL_RENDERBUFFER, rbo_depth));
	M_CHECKGL(glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, game_viewport.x, game_viewport.y));
	M_CHECKGL(glBindRenderbuffer(GL_RENDERBUFFER, 0));

	M_CHECKGL(glGenFramebuffers(1, &fbo));
	M_CHECKGL(glBindFramebuffer(GL_FRAMEBUFFER, fbo));
	M_CHECKGL(glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, fbo_texture, 0));
	M_CHECKGL(glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rbo_depth));

	GLenum DrawBuffers[1] = {GL_COLOR_ATTACHMENT0};
	M_CHECKGL(glDrawBuffers(1, DrawBuffers));

	GLenum status;
	if ((status = glCheckFramebufferStatus(GL_FRAMEBUFFER)) != GL_FRAMEBUFFER_COMPLETE) {
		fprintf(stderr, "glCheckFramebufferStatus: error %x", status);
		return;
	}
	M_CHECKGL(glBindFramebuffer(GL_FRAMEBUFFER, 0));

	static const GLfloat g_quad_vertex_buffer_data[] = {
		-1.0f, -1.0f,
		1.0f, -1.0f,
		-1.0f,  1.0f,
		-1.0f,  1.0f,
		1.0f, -1.0f,
		1.0f,  1.0f,
	};

	glGenVertexArrays(1, &vao_quad);
	M_CHECKGL(glGenBuffers(1, &vbo_fbo_vertices));
	M_CHECKGL(glBindBuffer(GL_ARRAY_BUFFER, vbo_fbo_vertices));
	M_CHECKGL(glBufferData(GL_ARRAY_BUFFER, sizeof(g_quad_vertex_buffer_data), g_quad_vertex_buffer_data, GL_STATIC_DRAW));

	vs = m_compile_shader(GL_VERTEX_SHADER, vshader);
	fs = m_compile_shader(GL_FRAGMENT_SHADER, fshader);
	free(vshader);
	free(fshader);

	program_postproc = glCreateProgram();
	M_CHECKGL(glAttachShader(program_postproc, vs));
	M_CHECKGL(glAttachShader(program_postproc, fs));
	glLinkProgram(program_postproc);
	glGetProgramiv(program_postproc, GL_LINK_STATUS, &link_ok);
	if (!link_ok) {
		fprintf(stderr, "glLinkProgram:");
		return;
	}
	glValidateProgram(program_postproc);
	glGetProgramiv(program_postproc, GL_VALIDATE_STATUS, &validate_ok);
	if (!validate_ok) {
		fprintf(stderr, "glValidateProgram:");
	}

	const char* attribute_name = "v_coord";
	attribute_v_coord_postproc = glGetAttribLocation(program_postproc, attribute_name);
	if (attribute_v_coord_postproc == -1) {
		fprintf(stderr, "Could not bind attribute %s\n", attribute_name);
		return;
	}

	const char* uniform_name = "fbo_texture";
	uniform_fbo_texture = glGetUniformLocation(program_postproc, uniform_name);
	if (uniform_fbo_texture == -1) {
		fprintf(stderr, "Could not bind uniform %s\n", uniform_name);
		return;
	}

	M_CHECKGL(glBindBuffer(GL_ARRAY_BUFFER, vbo_fbo_vertices));
	glBindVertexArray(vao_quad);
	glEnableVertexAttribArray(attribute_v_coord_postproc);
	glVertexAttribPointer(
		attribute_v_coord_postproc,  // attribute
		2,                  // number of elements per vertex, here (x,y)
		GL_FLOAT,           // the type of each element
		GL_FALSE,           // take our values as-is
		0,                  // no extra data between each position
		0                   // offset of first element
		);
	M_CHECKGL(glBindBuffer(GL_ARRAY_BUFFER, 0));
}

static
void game_draw(SDL_Point* viewport)
{

	M_CHECKGL(glBindFramebuffer(GL_FRAMEBUFFER, fbo));
	M_CHECKGL(glViewport(0, 0, viewport->x, viewport->y));

	mat44_t view;
	frustum_t frustum;

	M_CHECKGL(glEnable(GL_DEPTH_TEST));
	M_CHECKGL(glLogicOp(GL_INVERT));
	M_CHECKGL(glDepthFunc(GL_LESS));
	M_CHECKGL(glEnable(GL_CULL_FACE));
	M_CHECKGL(glCullFace(GL_BACK));
	M_CHECKGL(glClearColor(game.sky_light.x, game.sky_light.y, game.sky_light.z, 1.f));
	M_CHECKGL(glClearDepth(1.f));
	M_CHECKGL(glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT));

	if (game.wireframe)
		M_CHECKGL(glPolygonMode(GL_FRONT_AND_BACK, GL_LINE));

	chunkpos_t camera = player_chunk();
	vec3_t viewcenter = camera_offset();

	if (game.camera.mode != CAMERA_3RDPERSON) {
		m_fpsmatrix(&view, viewcenter, game.camera.pitch, game.camera.yaw);
	}
	else {
		m_lookat(&view,
		         viewcenter,
		         camera_offset(),
		         m_up);
	}

	m_copymat(m_getmatrix(&game.modelview), &view);
	m_makefrustum(&frustum, m_getmatrix(&game.projection), &view);

	// crosshair
	ui_rect(viewport->x/2 - 1, viewport->y/2 - 5, 2, 10, 0x4fffffff);
	ui_rect(viewport->x/2 - 5, viewport->y/2 - 1, 10, 2, 0x4fffffff);

	if (game.enable_ground)
		map_draw(&frustum);

	if (game.wireframe)
		M_CHECKGL(glPolygonMode(GL_FRONT_AND_BACK, GL_FILL));

	sky_draw();

	if (game.wireframe)
		M_CHECKGL(glPolygonMode(GL_FRONT_AND_BACK, GL_LINE));

	if (game.enable_ground)
		map_draw_alphapass();

	if (game.wireframe)
		M_CHECKGL(glPolygonMode(GL_FRONT_AND_BACK, GL_FILL));

	mat44_t invview;
	m_invert_orthonormal(&invview, &view);


	{
		dvec3_t pp = game.camera.pos;
		vec3_t v = {frustum.planes[5].x, frustum.planes[5].y, frustum.planes[5].z};
		if (map_raycast(pp, v, 16, &game.input.picked_block, &game.input.prepicked_block))
			if (game.camera.mode == CAMERA_FPS)
				ui_debug_block(game.input.picked_block, 0xcff1c40f);
	}

	if (game.camera.mode == CAMERA_3RDPERSON) {
		struct playervars* pv = player_vars();
		float ext = (game.player.crouching ? pv->crouchheight : pv->height) * 0.5f;
		float offs = (game.player.crouching ? CROUCHCENTEROFFSET : CENTEROFFSET);
		vec3_t center = { game.player.pos.x - camera.x*CHUNK_SIZE,
		                   game.player.pos.y + offs,
		                   game.player.pos.z - camera.z*CHUNK_SIZE };
		vec3_t extent = { 0.4f, ext, 0.4f };
		ui_debug_aabb(center, extent, 0xffffffff);
	}

	if (game.debug_mode) {
		vec3_t origo = { 0.0f, -0.25f, -0.4f };
		vec3_t xaxis = { 0.025, 0, 0 };
		vec3_t yaxis = { 0, 0.025, 0 };
		vec3_t zaxis = { 0, 0, 0.025 };
		origo = m_matmulvec3(&invview, &origo);
		xaxis = m_vec3add(origo, xaxis);
		yaxis = m_vec3add(origo, yaxis);
		zaxis = m_vec3add(origo, zaxis);
		ui_debug_line(origo, xaxis, 0xff00ff00);
		ui_debug_line(origo, yaxis, 0xffff0000);
		ui_debug_line(origo, zaxis, 0xff0000ff);

		static double ft[4] = {0};
		ft[game.stats.frames % 4] = 1.0 / ((double)(game.stats.frametime) / 1000.0);
		double fps = 0.0;
		for (int fi = 0; fi < 4; ++fi)
			fps += ft[fi] * 0.25;

		ui_text(4, viewport->y - 20, 0xffffffff,
			"pos: (%+4.4g, %+4.4g, %+4.4g)\n"
			"cam: (%+4.4g, %+4.4g, %+4.4g) p: %+.3g, y: %.3g\n"
			"vel: (%+4.4f, %+4.4f, %+4.4f)\n"
			"chunk: (%d, %d)\n"
			"%s%s%s\n"
			"fps: %g, t: %4.4f",
			game.player.pos.x, game.player.pos.y, game.player.pos.z,
			game.camera.pos.x, game.camera.pos.y, game.camera.pos.z,
			ML_RAD2DEG(game.camera.pitch), ML_RAD2DEG(game.camera.yaw),
			game.player.vel.x, game.player.vel.y, game.player.vel.z,
			camera.x, camera.z,
			game.player.walking ? "+walk " : "",
			game.player.crouching ? "+crouch " : "",
		        game.input.move_sprint ? "+sprint " : "",
		        round(fps), game.time_of_day);
	}
	ui_draw_debug(&game.projection, &game.modelview);
	ui_draw(viewport);

	M_CHECKGL(glBindFramebuffer(GL_FRAMEBUFFER, 0));

	//M_CHECKGL(glDisable(GL_CULL_FACE));

	glClearColor(1.0, 0.0, 0.0, 1.0);
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

	M_CHECKGL(glDisable(GL_DEPTH_TEST));
	M_CHECKGL(glUseProgram(program_postproc));
	M_CHECKGL(glActiveTexture(GL_TEXTURE0));
	M_CHECKGL(glBindTexture(GL_TEXTURE_2D, fbo_texture));
	M_CHECKGL(glUniform1i(uniform_fbo_texture, 0));
	M_CHECKGL(glBindVertexArray(vao_quad));

	glDrawArrays(GL_TRIANGLES, 0, 6);

	SDL_GL_SwapWindow(window);
}

static
void game_window_resize(void)
{
	SDL_GetWindowSize(window, &game_viewport.x, &game_viewport.y);
	M_CHECKGL(glViewport(0, 0, game_viewport.x, game_viewport.y));

	printf("resize: %d, %d\n", game_viewport.x, game_viewport.y);

	// Rescale FBO and RBO as well
	glBindTexture(GL_TEXTURE_2D, fbo_texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, game_viewport.x, game_viewport.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glBindTexture(GL_TEXTURE_2D, 0);

	glBindRenderbuffer(GL_RENDERBUFFER, rbo_depth);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, game_viewport.x, game_viewport.y);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);
}

static
void game_loop(int64_t dt)
{
	SDL_Event event;
	while (SDL_PollEvent(&event)) {
		if (event.type == SDL_WINDOWEVENT) {
			if (event.window.event == SDL_WINDOWEVENT_RESIZED) {
				game_window_resize();
			} else if (event.window.event == SDL_WINDOWEVENT_MAXIMIZED) {
				game_window_resize();
				m_perspective(m_getmatrix(&game.projection), ML_DEG2RAD(70.f),
				              (float)game_viewport.x / (float)game_viewport.y,
				              0.1f, 1024.f);
			} else if (event.window.event == SDL_WINDOWEVENT_FOCUS_LOST) {
				printf("focus lost\n");
				capture_mouse(false);
			} else if (event.window.event == SDL_WINDOWEVENT_CLOSE) {
				game.game_active = false;
			}
		} else if (!handle_event(&event)) {
			game.game_active = false;
		}
	}

	float frametime = (float)dt / 1000.f;
	game_tick(frametime);
}


int roam_main(int argc, char* argv[])
{
	GLenum rc;

	if (argc == 3 && strcmp(argv[1], "objtest") == 0) {
		char* fdata = sys_readfile(argv[2]);
		obj_t obj;
		obj_load(&obj, fdata, 0.1f);
		printf("loaded %s: %zu verts, %zu faces\n", argv[2], obj.nverts / 3, obj.nindices / 3);
		free(fdata);
		obj_free(&obj);
		exit(0);
	}

	if (SDL_Init(SDL_INIT_EVERYTHING) < 0)
		return 1;

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
	SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);

	window = SDL_CreateWindow("roam",
	                               SDL_WINDOWPOS_UNDEFINED,
	                               SDL_WINDOWPOS_UNDEFINED,
	                               1280, 720,
	                               SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
	if (window == NULL)
		fatal_error(SDL_GetError());

	context = SDL_GL_CreateContext(window);
	if (context == NULL)
		fatal_error(SDL_GetError());

	SDL_GL_MakeCurrent(window, context);
	{
		int sw = SDL_GL_SetSwapInterval(-1); // late swap tearing
		if (sw == -1) sw = SDL_GL_SetSwapInterval(0); // vsync off
		if (sw == -1) sw = SDL_GL_SetSwapInterval(1);
	}

	glewExperimental = GL_TRUE;
	if ((rc = glewInit()) != GLEW_OK)
		fatal_error((const char*)glewGetErrorString(rc));

	const GLubyte *version, *vendor, *renderer, *glslversion;
	M_CHECKGL(version = glGetString(GL_VERSION));
	#if M_CHECKGL_ENABLED
	fprintf(stderr, "(if there was a GL error reported before, it is due to a bug in GLEW)\n");
	#endif
	M_CHECKGL(vendor = glGetString(GL_VENDOR));
	M_CHECKGL(renderer = glGetString(GL_RENDERER));
	M_CHECKGL(glslversion = glGetString(GL_SHADING_LANGUAGE_VERSION));

	printf("Version: %s\n"
	       "Vendor: %s\n"
	       "Renderer: %s\n"
	       "GLSL Version: %s\n",
	       version, vendor, renderer, glslversion);

	SDL_GetWindowSize(window, &game_viewport.x, &game_viewport.y);
	M_CHECKGL(glViewport(0, 0, game_viewport.x, game_viewport.y));

	m_mtxstack_init(&game.projection, 3);
	m_mtxstack_init(&game.modelview, 16);

	m_perspective(m_getmatrix(&game.projection), ML_DEG2RAD(70.f),
	              (float)game_viewport.x / (float)game_viewport.y,
	              0.1f, 1024.f);

	game_init();
	init_fbo_resources();

	int64_t currenttime, newtime, frametime;
	int64_t t, dt, accumulator ;
	game.stats.frametime = (int64_t)((1.0 / 60.0) * 1000.0);

	t = 0;
	dt = 15;
	accumulator = 0;
	currenttime = sys_timems();
	game.game_active = true;
	while (game.game_active) {
		newtime = sys_timems();
		frametime = newtime - currenttime;
		if (frametime > 250)
			frametime = 250;
		currenttime = newtime;

		accumulator += frametime;
		while (accumulator >= dt) {
			// timestep
			game_loop(dt);
			t += dt;
			accumulator -= dt;
		}
		game_draw(&game_viewport);
		game.stats.frames++;
		game.stats.frametime = frametime;
	}
	return 0;
}
