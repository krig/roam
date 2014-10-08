#include <time.h>
#include "common.h"
#include "math3d.h"
#include "shaders.h"
#include "ui.h"
#include "objfile.h"
#include "noise.h"
#include "voxelmap.h"
#include "game.h"
#include "geometry.h"

static SDL_Window* window;
static SDL_GLContext context;
static bool mouse_captured = false;
static bool wireframe_mode = false;

struct game_t game;

ml_tex2d blocks_texture;

struct blockinfo_t blockinfo[] = {
	{ .name = "air", .tex = { 0, 0, 0, 0, 0, 0 } },
	{ .name = "grass", .tex = { 1, 3, 2, 2, 2, 2 } },
	{ .name = "dirt", .tex = { 3, 3, 3, 3, 3, 3 } },
	{ .name = "sand", .tex = { 4, 4, 4, 4, 4, 4 } },
	{ .name = "stone", .tex = { 5, 5, 5, 5, 5, 5 } },
	{ .name = "darkstone", .tex = { 6, 6, 6, 6, 6, 6 } },
	{ .name = "bone", .tex = { 7, 7, 7, 7, 7, 7 } },
	{ .name = "skin", .tex = { 8, 8, 8, 8, 8, 8 } },
	{ .name = "diamond ore", .tex = { 9, 9, 9, 9, 9, 9 } },
	{ .name = "clay", .tex = { 10, 10, 10, 10, 10, 10 } },
	{ .name = "cobblestone", .tex = { 11, 11, 11, 11, 11, 11 } },
	{ .name = "lava", .tex = { 12, 12, 12, 12, 12, 12 } },
	{ .name = "water", .tex = { 13, 13, 13, 13, 13, 13 } },
	{ .name = "treetrunk", .tex = { 14, 14, 14, 14, 14, 14 } },
	{ .name = "deadskin", .tex = { 15, 15, 15, 15, 15, 15 } },
	{ .name = "shroom", .tex = { 16, 16, 16, 16, 16, 16 } },
	{ .name = "grass billboard", .tex = { 0, 0, 17, 17, 17, 17 } },
	{ .name = "leaves", .tex = { 18, 18, 18, 18, 18, 18 } },
	{ .name = "moss", .tex = { 20, 21, 19, 19, 19, 19 } },
	{ .name = "snowy spruce", .tex = { 23, 24, 22, 22, 22, 22 } },
};

static void
gameInit() {
	mlLoadTexture2D(&blocks_texture, "data/blocks.png");

	game.camera.cx = game.camera.cz = 0;
	ml_vec3 offs = { 2.537648, GROUND_LEVEL + 1.336000, 0.514751 };
	game.camera.offset = offs;
	game.camera.pitch = -0.544628;
	game.camera.yaw = 1.056371;
	game.fast_day_mode = false;
	game.single_chunk_mode = false;

	struct controls_t default_controls = {
		.left = SDL_SCANCODE_A,
		.right = SDL_SCANCODE_D,
		.forward = SDL_SCANCODE_W,
		.backward = SDL_SCANCODE_S,
		.jump = SDL_SCANCODE_SPACE,
		.crouch = SDL_SCANCODE_LSHIFT,
		.interact = SDL_SCANCODE_E,
		.primary_action = SDL_BUTTON_LEFT,
		.secondary_action = SDL_BUTTON_RIGHT,
		.wireframe = SDLK_o,
		.debuginfo = SDLK_p,
		.exitgame = SDLK_ESCAPE
	};
	game.controls = default_controls;

	game.time_of_day = 0.f; // (0 - 1 looping: 0 is midday, 0.5 is midnight)
#define H2F(r) (float)(0x##r)/255.f

	mlVec4Assign(game.ambient_color, H2F(61), H2F(04), H2F(5F), 0.4f);
	mlVec4Assign(game.fog_color, H2F(28), H2F(30), H2F(48), 0.0075f);

	gameInitMap();

	unsigned long lcg = time(NULL);
	osnInit((unsigned long (*)(void*))&lcg_rand, &lcg);
	simplexInit((unsigned long (*)(void*))&lcg_rand, &lcg);

	printf("* Load materials + UI\n");
	mlCreateMaterial(&game.materials[MAT_BASIC], basic_vshader, basic_fshader);
	mlCreateMaterial(&game.materials[MAT_UI], ui_vshader, ui_fshader);
	mlCreateMaterial(&game.materials[MAT_DEBUG], debug_vshader, debug_fshader);
	mlCreateMaterial(&game.materials[MAT_CHUNK], chunk_vshader, chunk_fshader);
	uiInit(game.materials + MAT_UI, game.materials + MAT_DEBUG);

	glCheck(__LINE__);

	mouse_captured = true;
	SDL_SetRelativeMouseMode(SDL_TRUE);
}

static void
gameExit() {
	uiExit();
	for (int i = 0; i < MAX_MATERIALS; ++i)
		mlDestroyMaterial(game.materials + i);
	mlDestroyMatrixStack(&game.projection);
	mlDestroyMatrixStack(&game.modelview);
	gameFreeMap();
	mlDestroyTexture2D(&blocks_texture);
}

static bool
gameHandleEvent(SDL_Event* event) {
	switch (event->type) {
	case SDL_QUIT:
		return false;
	case SDL_KEYDOWN:
		if (event->key.keysym.sym == game.controls.exitgame)
			return false;
		else if (event->key.keysym.sym == game.controls.debuginfo)
			printf("(%d, %d) + (%f, %f, %f) p:%f, y:%f\n",
			       game.camera.cx, game.camera.cz,
			       game.camera.offset.x, game.camera.offset.y, game.camera.offset.z,
			       game.camera.pitch, game.camera.yaw);
		else if (event->key.keysym.sym == game.controls.wireframe)
			wireframe_mode = !wireframe_mode;
		else if (event->key.keysym.sym == SDLK_i)
			game.single_chunk_mode = !game.single_chunk_mode;
		else if (event->key.keysym.sym == SDLK_l)
			game.fast_day_mode = true;
		break;
	case SDL_KEYUP:
		if (event->key.keysym.sym == SDLK_l)
			game.fast_day_mode = false;
	default:
		break;
	}
	return true;
}

static void
playerLook(float dyaw, float dpitch) {
	game.camera.yaw = mlWrap(game.camera.yaw - dyaw, 0.f, ML_TWO_PI);
	game.camera.pitch = mlClamp(game.camera.pitch - dpitch, -ML_PI_2, ML_PI_2);
}

static void
playerMove(float right, float forward) {
	ml_vec3 xaxis, yaxis, zaxis;
	mlFPSRotation(0, game.camera.yaw, &xaxis, &yaxis, &zaxis);
	game.camera.offset.x += xaxis.x * right;
	game.camera.offset.z += xaxis.z * right;
	game.camera.offset.x += zaxis.x * -forward;
	game.camera.offset.z += zaxis.z * -forward;
}

static void
playerJump(float speed) {
	game.camera.offset.y += speed;
}

static void
gameUpdate(float dt) {
	const float xsens = 1.f / ML_TWO_PI;
	const float ysens = 1.f / ML_TWO_PI;
	int mouse_dx, mouse_dy;
	const Uint8* state;
	float speed = 2.f * dt;

	state = SDL_GetKeyboardState(NULL);
	if (state[game.controls.left])
		playerMove(-speed, 0.f);
	if (state[game.controls.right])
		playerMove(speed, 0.f);
	if (state[game.controls.forward])
		playerMove(0.f, speed);
	if (state[game.controls.backward])
		playerMove(0.f, -speed);
	if (state[game.controls.jump])
		playerJump(speed);
	if (state[game.controls.crouch])
		playerJump(-speed);

	if (mouse_captured) {
		SDL_GetRelativeMouseState(&mouse_dx, &mouse_dy);
		playerLook(mouse_dx * dt * xsens, mouse_dy * dt * ysens);
	} else if (SDL_GetMouseState(0, 0) & SDL_BUTTON(SDL_BUTTON_LEFT)) {
			mouse_captured = true;
			SDL_SetRelativeMouseMode(SDL_TRUE);
	}

	gameUpdateMap();
	// update player/input
	// update blocks
	// update creatures
	// update effects

	if (game.fast_day_mode)
		game.time_of_day = fmod(game.time_of_day + (dt * (1.0 / 5.0)), 1.f);
	else
		game.time_of_day = fmod(game.time_of_day + (dt * (1.0 / DAY_LENGTH)), 1.f);
}

static void
printMatrix(ml_matrix* m) {
	printf("%.1f %.1f %.1f %.1f ", m->m[0], m->m[1], m->m[2], m->m[3]);
	printf("%.1f %.1f %.1f %.1f ", m->m[4], m->m[5], m->m[6], m->m[7]);
	printf("%.1f %.1f %.1f %.1f ", m->m[8], m->m[9], m->m[10], m->m[11]);
	printf("%.1f %.1f %.1f %.1f\n", m->m[12], m->m[13], m->m[14], m->m[15]);
}

static void
gameRender(SDL_Point* viewport, float frametime) {
	glCheck(__LINE__);

	glEnable(GL_DEPTH_TEST);
	glLogicOp(GL_INVERT);
	glDepthFunc(GL_LESS);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glClearColor(H2F(28), H2F(30), H2F(48), 1.f);
	glClearDepth(1.f);
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

	if (wireframe_mode)
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

	mlFPSMatrix(mlGetMatrix(&game.modelview), game.camera.offset, game.camera.pitch, game.camera.yaw);

	// draw blocks (tesselate in parallel?)
	// draw objects
	// draw creatures
	// draw players
	// draw effects
	// draw alphablended (sort in parallel?)
	// fbo effects?
	// draw ui


	double toffs = fmod((game.time_of_day * ML_TWO_PI) + ML_PI_2, ML_TWO_PI);
	mlVec3Assign(game.light_dir, -cos(toffs), sin(toffs), cos(toffs));
	game.light_dir = mlVec3Normalize(game.light_dir);

	gameDrawMap();

	if (wireframe_mode)
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	ml_vec3 mc = mlVec3Scalef(mlVec3Normalize(game.light_dir), 4.f);
	ml_vec3 mc2 = mlVec3Scalef(mlVec3Normalize(game.light_dir), 5.f);
	mc.y += GROUND_LEVEL;
	mc2.y += GROUND_LEVEL;
	uiDebugLine(mc, mc2, 0xffff7f00);
	uiDebugSphere(mc2, 0.15f, 0xffcf9f3f);

	ml_vec3 origo = { 0, GROUND_LEVEL, 0 };
	ml_vec3 xaxis = { 1, GROUND_LEVEL, 0 };
	ml_vec3 yaxis = { 0, GROUND_LEVEL + 1, 0 };
	ml_vec3 zaxis = { 0, GROUND_LEVEL, 1 };
	uiDebugLine(origo, xaxis, 0xff00ff00);
	uiDebugLine(origo, yaxis, 0xffff0000);
	uiDebugLine(origo, zaxis, 0xff0000ff);

	uiDrawDebug(&game.projection, &game.modelview);


	uiText(5, 15, 0xffaaaaaa, "%g, %g, %g",
	       (double)game.camera.cx + game.camera.offset.x, \
	       game.camera.offset.y,
	       (double)game.camera.cz + game.camera.offset.z);
	uiText(5, 5, 0xffaaaaaa, "fps: %d, t: %f", (int)(1.f / frametime), game.time_of_day);
	uiDraw(viewport);

	if (mouse_captured)
		SDL_WarpMouseInWindow(window, viewport->x >> 1, viewport->y >> 1);
}

int
main(int argc, char* argv[]) {
	SDL_Event event;
	SDL_Point sz;
	GLenum rc;

	if (argc == 3 && strcmp(argv[1], "objtest") == 0) {
		char* fdata = osReadWholeFile(argv[2]);
		obj_mesh obj;
		objLoad(&obj, fdata, 0.1f);
		printf("loaded %s: %zu verts, %zu faces\n", argv[2], obj.nverts / 3, obj.nindices / 3);
		free(fdata);
		objFree(&obj);
		exit(0);
	}

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

	if (SDL_Init(SDL_INIT_EVERYTHING) < 0)
		return 1;

	window = SDL_CreateWindow("roam",
	                               SDL_WINDOWPOS_UNDEFINED,
	                               SDL_WINDOWPOS_UNDEFINED,
	                               1100, 550,
	                               SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
	if (window == NULL)
		roamError(SDL_GetError());

	context = SDL_GL_CreateContext(window);
	if (context == NULL)
		roamError(SDL_GetError());

	SDL_GL_MakeCurrent(window, context);
	SDL_GL_SetSwapInterval(1);

	glewExperimental = GL_TRUE;
	if ((rc = glewInit()) != GLEW_OK)
		roamError((const char*)glewGetErrorString(rc));

	int major, minor;
	glGetIntegerv(GL_MAJOR_VERSION, &major);
	glGetIntegerv(GL_MINOR_VERSION, &minor);
	printf("GL %d.%d\n", major, minor);

	printf("Version: %s\n"
	       "Vendor: %s\n"
	       "Renderer: %s\n"
	       "GLSL Version: %s\n",
	       glGetString(GL_VERSION),
	       glGetString(GL_VENDOR),
	       glGetString(GL_RENDERER),
	       glGetString(GL_SHADING_LANGUAGE_VERSION));

	SDL_GetWindowSize(window, &sz.x, &sz.y);
	glViewport(0, 0, sz.x, sz.y);

	mlInitMatrixStack(&game.projection, 3);
	mlInitMatrixStack(&game.modelview, 16);

	mlPerspective(mlGetMatrix(&game.projection), mlDeg2Rad(70.f),
	              (float)sz.x / (float)sz.y,
	              0.1f, VIEW_DISTANCE*CHUNK_SIZE);

	gameInit();
	glCheck(__LINE__);

	int64_t startms, nowms;
	float frametime;
	startms = (int64_t)SDL_GetTicks();

	for (;;) {
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_WINDOWEVENT) {
				if (event.window.event == SDL_WINDOWEVENT_RESIZED) {
					glViewport(0, 0, event.window.data1, event.window.data2);
					sz.x = event.window.data1;
					sz.y = event.window.data2;
				} else if (event.window.event == SDL_WINDOWEVENT_MAXIMIZED) {
					SDL_GetWindowSize(window, &sz.x, &sz.y);
					glViewport(0, 0, sz.x, sz.y);
					mlPerspective(mlGetMatrix(&game.projection), mlDeg2Rad(70.f),
					              (float)sz.x / (float)sz.y,
					              0.1f, 100.f);
				} else if (event.window.event == SDL_WINDOWEVENT_FOCUS_LOST) {
					SDL_SetRelativeMouseMode(SDL_FALSE);
					mouse_captured = false;
				} else if (event.window.event == SDL_WINDOWEVENT_CLOSE) {
					goto exit;
				}
			} else if (!gameHandleEvent(&event)) {
				goto exit;
			}
		}

		nowms = (int64_t)SDL_GetTicks() - startms;
		if (nowms < 1)
			nowms = 1;
		frametime = (float)((double)nowms / 1000.0);
		gameUpdate(frametime);
		startms = (int64_t)SDL_GetTicks();
		gameRender(&sz, frametime);

		SDL_GL_SwapWindow(window);
		glCheck(__LINE__);
	}

exit:
	gameExit();
	SDL_GL_DeleteContext(context);
	SDL_DestroyWindow(window);
	SDL_Quit();
	return 0;
}
