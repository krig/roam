#include <time.h>
#include <pthread.h>
#include "common.h"
#include "math3d.h"
#include "shaders.h"
#include "ui.h"
#include "objfile.h"
#include "noise.h"
#include "voxelmap.h"
#include "game.h"
#include "geometry.h"
#include "u8.h"
#include "sky.h"

static SDL_Window* window;
static SDL_GLContext context;
static bool mouse_captured = false;
static bool wireframe_mode = false;

struct game_t game;

ml_tex2d blocks_texture;

static void gameUpdateTime(float dt);


uint64_t good_seed() {
	FILE* f = fopen("/dev/urandom", "rb");
	if (f == NULL)
		f = fopen("/dev/random", "rb");
	if (f == NULL)
		return time(NULL);
	uint64_t seed;
	fread(&seed, sizeof(uint64_t), 1, f);
	fclose(f);
	return seed;
}


static void
gameInit() {
	mlLoadTexture2D(&blocks_texture, "data/blocks8-v1.png");

	ml_dvec3 offs = { 2.537648, OCEAN_LEVEL + 1.336000, 0.514751 };
	game.camera.pos = offs;
	game.camera.pitch = -0.544628;
	game.camera.yaw = 1.056371;
	game.fast_day_mode = false;
	game.debug_mode = false;

	struct controls_t default_controls = {
		.left = SDL_SCANCODE_A,
		.right = SDL_SCANCODE_D,
		.forward = SDL_SCANCODE_W,
		.backward = SDL_SCANCODE_S,
		.jump = SDL_SCANCODE_SPACE,
		.sprint = SDL_SCANCODE_LCTRL,
		.crouch = SDL_SCANCODE_LSHIFT,
		.interact = SDL_SCANCODE_E,
		.primary_action = SDL_BUTTON_LEFT,
		.secondary_action = SDL_BUTTON_RIGHT,
		.wireframe = SDLK_o,
		.debuginfo = SDLK_p,
		.exitgame = SDLK_ESCAPE
	};
	game.controls = default_controls;


	printf("* Load materials + UI\n");
	mlCreateMaterial(&game.materials[MAT_BASIC], basic_vshader, basic_fshader);
	mlCreateMaterial(&game.materials[MAT_UI], ui_vshader, ui_fshader);
	mlCreateMaterial(&game.materials[MAT_DEBUG], debug_vshader, debug_fshader);
	mlCreateMaterial(&game.materials[MAT_CHUNK], chunk_vshader, chunk_fshader);
	mlCreateMaterial(&game.materials[MAT_SKY], sky_vshader, sky_fshader);
	uiInit(game.materials + MAT_UI, game.materials + MAT_DEBUG);

	game.day = 0;
	game.time_of_day = 0;
	gameUpdateTime(0);
	gameInitMap();

	while (blockType(game.camera.pos.x,
	                 game.camera.pos.y-1,
	                 game.camera.pos.z) != BLOCK_AIR &&
	       blockType(game.camera.pos.x,
	                 game.camera.pos.y,
	                 game.camera.pos.z) != BLOCK_AIR)
		game.camera.pos.y += 1.0;

	skyInit();

	glCheck(__LINE__);

	mouse_captured = true;
	SDL_SetRelativeMouseMode(SDL_TRUE);
}

static ml_vec3
sun_mix(ml_vec3* colors, double day_amt, double dusk_amt, double night_amt, double dawn_amt) {
	ml_vec3 c;
	c.x = colors[0].x*day_amt + colors[1].x*dusk_amt + colors[2].x*night_amt + colors[3].x*dawn_amt;
	c.y = colors[0].y*day_amt + colors[1].y*dusk_amt + colors[2].y*night_amt + colors[3].y*dawn_amt;
	c.z = colors[0].z*day_amt + colors[1].z*dusk_amt + colors[2].z*night_amt + colors[3].z*dawn_amt;
	return c;
}

static inline ml_vec3
mkrgb(uint32_t rgb) {
	ml_vec3 c = {((float)((rgb>>16)&0xff)/255.f),
	             ((float)((rgb>>8)&0xff)/255.f),
	             ((float)((rgb)&0xff)/255.f) };
	return c;
}

static void
gameUpdateTime(float dt) {
	double daylength = DAY_LENGTH;
	if (game.fast_day_mode)
		daylength = 10.0;
	double step = (dt / daylength);
	game.time_of_day += step;
	while (game.time_of_day >= 1.0) {
		game.day += 1;
		game.time_of_day -= 1.0;
	}

	double t = game.time_of_day;
	double day_amt, night_amt, dawn_amt, dusk_amt;

	if (t >= 0 && t < 0.5) {
		day_amt = 1.0;
		night_amt = dawn_amt = dusk_amt = 0;
	} else if (t >= 0.5 && t < 0.6) {
		double f = (t - 0.5) * 10.0; // 0-1
		dusk_amt = sin(f * ML_PI);
		day_amt = mlClampd(1.0 - f*2.0, 0.0, 1.0);
		night_amt = mlClampd((f - 0.5)*2.0, 0.0, 1.0);
		dawn_amt = 0;
	} else if (t >= 0.6 && t < 0.9) {
		night_amt = 1.0;
		dawn_amt = dusk_amt = day_amt = 0;
	} else {
		double f = (t - 0.9) * 10.0; // 0-1
		dawn_amt = sin(f * ML_PI);
		night_amt = mlClampd(1.0 - f*2.0, 0.0, 1.0);
		day_amt = mlClampd((f - 0.5)*2.0, 0.0, 1.0);
		dusk_amt = 0;
	}

	double low_light = 0.1;
	double lightlevel = mlMax(day_amt, low_light);
	game.light_level = lightlevel;

#define MKRGB(rgb) mkrgb(0x##rgb)

	// day, dusk, night, dawn
	ml_vec3 ambient[4] = {
		MKRGB(ffffff),
		MKRGB(544769),
		MKRGB(101010),
		MKRGB(6f2168),
	};
	ml_vec3 sky_dark[4] = {
		MKRGB(3F6CB4),
		MKRGB(40538e),
		MKRGB(030710),
		MKRGB(3d2163),
	};
	ml_vec3 sky_light[4] = {
		MKRGB(A6BEDB),
		MKRGB(6a6ca5),
		MKRGB(272e58),
		MKRGB(e16e7a),
	};
	ml_vec3 sun_color[4] = {
		MKRGB(E8EAE7),
		MKRGB(fdf2c9),
		MKRGB(e2f3fa),
		MKRGB(fefebb),
	};
	ml_vec3 fog[4] = {
		MKRGB(B5D2E2),
		MKRGB(ad6369),
		MKRGB(666666),
		MKRGB(f7847a),
	};

	game.amb_light = sun_mix(ambient, day_amt, dusk_amt, night_amt, dawn_amt);
	game.sky_dark = sun_mix(sky_dark, day_amt, dusk_amt, night_amt, dawn_amt);
	game.sky_light = sun_mix(sky_light, day_amt, dusk_amt, night_amt, dawn_amt);
	game.sun_color = sun_mix(sun_color, day_amt, dusk_amt, night_amt, dawn_amt);
	ml_vec3 fog_color = sun_mix(fog, day_amt, dusk_amt, night_amt, dawn_amt);
	double fog_density = (night_amt * 0.002) + (dawn_amt * 0.005) + (day_amt * 0.004) + (dusk_amt * 0.004);

	mlVec4Assign(game.fog_color, fog_color.x, fog_color.y, fog_color.z, fog_density);
	mlVec3Assign(game.light_dir, cos(t * ML_TWO_PI), -sin(t * ML_TWO_PI), 0);
	game.light_dir = mlVec3Normalize(game.light_dir);
}

static void
gameExit() {
	gameFreeMap();
	skyExit();
	uiExit();
	for (int i = 0; i < MAX_MATERIALS; ++i)
		mlDestroyMaterial(game.materials + i);
	mlDestroyMatrixStack(&game.projection);
	mlDestroyMatrixStack(&game.modelview);
	mlDestroyTexture2D(&blocks_texture);
}

static ml_ivec3 picked_block;
static ml_ivec3 prepicked_block;


static bool
gameHandleEvent(SDL_Event* event) {
	switch (event->type) {
	case SDL_QUIT:
		return false;
	case SDL_KEYDOWN:
		if (event->key.keysym.sym == game.controls.exitgame)
			return false;
		//else if (event->key.keysym.sym == game.controls.debuginfo)
		//	printf("(%d, %d) + (%f, %f, %f) p:%f, y:%f\n",
		//	       game.camera.cx, game.camera.cz,
		//	       game.camera.offset.x, game.camera.offset.y, game.camera.offset.z,
		//	       game.camera.pitch, game.camera.yaw);
		else if (event->key.keysym.sym == game.controls.wireframe)
			wireframe_mode = !wireframe_mode;
		else if (event->key.keysym.sym == SDLK_F1)
			game.debug_mode = !game.debug_mode;
		else if (event->key.keysym.sym == SDLK_F2)
			game.fast_day_mode = true;
		break;
	case SDL_KEYUP:
		if (event->key.keysym.sym == SDLK_F2)
			game.fast_day_mode = false;
		break;
	case SDL_MOUSEBUTTONDOWN:
		switch (event->button.button) {
		case SDL_BUTTON_LEFT: {
			printf("deleting picked block (%d, %d, %d)\n",
			       picked_block.x, picked_block.y, picked_block.z);
			if (blockTypeByCoord(picked_block) != BLOCK_AIR) {
				printf("can delete.\n");
				game.map.blocks[blockByCoord(picked_block)] = BLOCK_AIR;
				gameUpdateBlock(picked_block);
			}
		} break;
		case SDL_BUTTON_RIGHT: {
			printf("creating block (%d, %d, %d)\n",
			       prepicked_block.x, prepicked_block.y, prepicked_block.z);

			ml_ivec3 head = cameraBlock();
			ml_ivec3 feet = head;
			feet.y -= 1;
			if (blockTypeByCoord(prepicked_block) == BLOCK_AIR &&
			    !blockCompare(head, prepicked_block) &&
			    !blockCompare(feet, prepicked_block)) {
				printf("can create.\n");
				game.map.blocks[blockByCoord(prepicked_block)] = BLOCK_PIG;
				gameUpdateBlock(prepicked_block);
			}
		} break;
		}
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
	game.camera.pos.x += xaxis.x * right;
	game.camera.pos.z += xaxis.z * right;
	game.camera.pos.x += zaxis.x * -forward;
	game.camera.pos.z += zaxis.z * -forward;
}

static void
playerJump(float speed) {
	game.camera.pos.y += speed;
}

static void
gameUpdate(float dt) {
	const float xsens = 1.f / ML_TWO_PI;
	const float ysens = 1.f / ML_TWO_PI;
	int mouse_dx, mouse_dy;
	const Uint8* state;
	float speed = 2.f * dt;

	state = SDL_GetKeyboardState(NULL);

	if (state[game.controls.sprint] || state[SDL_SCANCODE_CAPSLOCK])
		speed *= 10.f;

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

	gameUpdateTime(dt);
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
	glClearColor(game.fog_color.x, game.fog_color.y, game.fog_color.z, 1.f);
	glClearDepth(1.f);
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

	if (wireframe_mode)
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

	ml_chunk camera = cameraChunk();
	ml_vec3 viewcenter = cameraChunkOffset();

	ml_matrix view;
	mlFPSMatrix(&view, viewcenter, game.camera.pitch, game.camera.yaw);
	//mlLookAt(&view, game.camera.offset.x, game.camera.offset.y, game.camera.offset.z,
	//         2.f, OCEAN_LEVEL, -2.f,
	//         0.f, 1.f, 0.f);

	mlCopyMatrix(mlGetMatrix(&game.modelview), &view);

	// calculate view frustum
	ml_frustum frustum;
	mlGetFrustum(&frustum, mlGetMatrix(&game.projection), &view);

	uiRect(viewport->x/2 - 1, viewport->y/2, 2, 2, 0x7fffffff);

	// draw blocks (tesselate in parallel?)
	// draw objects
	// draw creatures
	// draw players
	// draw effects
	// draw alphablended (sort in parallel?)
	// fbo effects?
	// draw ui

	gameDrawMap(&frustum);

	skyDraw();

	if (wireframe_mode)
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	/*
	ml_vec3 mc = mlVec3Scalef(mlVec3Normalize(game.light_dir), 4.f);
	ml_vec3 mc2 = mlVec3Scalef(mlVec3Normalize(game.light_dir), 5.f);
	mc.y += OCEAN_LEVEL;
	mc2.y += OCEAN_LEVEL;
	uiDebugLine(mc, mc2, 0xffff7f00);
	uiDebugSphere(mc2, 0.15f, 0xffcf9f3f);
	*/

	ml_matrix invview;
	mlInvertOrthoMatrix(&invview, &view);


	{
		ml_ivec3 pp = {
			round(game.camera.pos.x),
			round(game.camera.pos.y),
			round(game.camera.pos.z)
		};
		ml_vec3 v = {
			frustum.planes[5].x,
			frustum.planes[5].y,
			frustum.planes[5].z
		};
		if (gameRayTest(pp, v, 16, &picked_block, &prepicked_block)) {
			uiDebugBlock(picked_block, 0xff00ff00);
		}
	}

	ml_vec3 origo = { 0.0f, -0.25f, -0.4f };
	ml_vec3 xaxis = { 0.025, 0, 0 };
	ml_vec3 yaxis = { 0, 0.025, 0 };
	ml_vec3 zaxis = { 0, 0, 0.025 };
	origo = mlMulMatVec3(&invview, &origo);
	xaxis = mlVec3Add(origo, xaxis);
	yaxis = mlVec3Add(origo, yaxis);
	zaxis = mlVec3Add(origo, zaxis);
	uiDebugLine(origo, xaxis, 0xff00ff00);
	uiDebugLine(origo, yaxis, 0xffff0000);
	uiDebugLine(origo, zaxis, 0xff0000ff);

	uiDrawDebug(&game.projection, &game.modelview);

	uiRect(2, 2, 320, 5 + 16 + 2, 0x66000000);
	uiText(5, 5, 0xffffffff, "%g, %g, %g (%d, %d)\nfps: %d, t: %f",
	       game.camera.pos.x,
	       game.camera.pos.y,
	       game.camera.pos.z,
	       camera.x,
	       camera.z,
	       (int)(1.f / frametime), game.time_of_day);
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
	              0.1f, 1024.f);

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
					              0.1f, 1024.f);
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
