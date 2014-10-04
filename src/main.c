#include <time.h>
#include "common.h"
#include "math3d.h"
#include "shaders.h"
#include "ui.h"
#include "objfile.h"
#include "noise.h"
#include "voxelmap.h"
#include "game.h"

#define MAX_MESHES 100
#define MAX_RENDERABLES 100

static SDL_Window* window;
static SDL_GLContext context;
static ml_mesh meshes[MAX_MESHES];
static ml_renderable renderables[MAX_RENDERABLES];
static bool mouse_captured = false;
static bool wireframe_mode = false;

struct game_t game;

static void
gameInit() {
	game.camera.chunk[0] =
		game.camera.chunk[1] =
		game.camera.chunk[2] = 0;
	ml_vec3 offs = { 2.537648, 1.336000, 0.514751 };
	game.camera.offset = offs;
	game.camera.pitch = -0.544628;
	game.camera.yaw = 1.056371;

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

	uint32_t top, bottom;
	top = 0xff24C6DC;
	bottom = 0xff514A9D;
	ml_vtx_pos_n_clr corners[8] = {
		{{-0.5f,-0.5f,-0.5f }, { 0, 0, 0 }, bottom },
		{{ 0.5f,-0.5f,-0.5f }, { 0, 0, 0 }, bottom },
		{{-0.5f,-0.5f, 0.5f }, { 0, 0, 0 }, bottom },
		{{ 0.5f,-0.5f, 0.5f }, { 0, 0, 0 }, bottom },
		{{-0.5f, 0.5f,-0.5f }, { 0, 0, 0 }, top },
		{{-0.5f, 0.5f, 0.5f }, { 0, 0, 0 }, top },
		{{ 0.5f, 0.5f,-0.5f }, { 0, 0, 0 }, top },
		{{ 0.5f, 0.5f, 0.5f }, { 0, 0, 0 }, top }
	};

	ml_vtx_pos_n_clr tris[] = {
		corners[0], // bottom
		corners[1],
		corners[2],
		corners[1],
		corners[3],
		corners[2],

		corners[4], // top
		corners[5],
		corners[6],
		corners[6],
		corners[5],
		corners[7],

		corners[2], // front
		corners[3],
		corners[5],
		corners[3],
		corners[7],
		corners[5],

		corners[0], // back
		corners[4],
		corners[1],
		corners[1],
		corners[4],
		corners[6],

		corners[2], // left
		corners[4],
		corners[0],
		corners[2],
		corners[5],
		corners[4],

		corners[3], // right
		corners[1],
		corners[6],
		corners[3],
		corners[6],
		corners[7]
	};

	ml_vec3 normals[6] = {
		{ 0,-1, 0 },
		{ 0, 1, 0 },
		{ 0, 0, 1 },
		{ 0, 0,-1 },
		{-1, 0, 0 },
		{ 1, 0, 0 }
	};

	for (int i = 0; i < 6; ++i) {
		tris[i*6 + 0].n = normals[i];
		tris[i*6 + 1].n = normals[i];
		tris[i*6 + 2].n = normals[i];
		tris[i*6 + 3].n = normals[i];
		tris[i*6 + 4].n = normals[i];
		tris[i*6 + 5].n = normals[i];
	}

	printf("materials...\n");
	mlCreateMaterial(&game.materials[MAT_BASIC], basic_vshader, basic_fshader);
	mlCreateMaterial(&game.materials[MAT_UI], ui_vshader, ui_fshader);
	mlCreateMaterial(&game.materials[MAT_DEBUG], debug_vshader, debug_fshader);
	mlCreateMaterial(&game.materials[MAT_CHUNK], chunk_vshader, chunk_fshader);

	mlCreateMesh(&meshes[0], 36, tris, ML_POS_3F | ML_N_3F | ML_CLR_4UB);
	mlCreateRenderable(&renderables[0], game.materials + MAT_BASIC, meshes + 0);

	{
		char* teapot = osReadWholeFile("data/teapot.obj");
		obj_mesh m;
		objLoad(&m, teapot, 0.1f);
		objCreateMesh(&meshes[1], &m, objGenNormalsFn);
		mlCreateRenderable(&renderables[1], game.materials + MAT_BASIC, meshes + 1);
		objFree(&m);
		free(teapot);
	}

	printf("ui...\n");
	uiInit(game.materials + MAT_UI, game.materials + MAT_DEBUG);

	glCheck(__LINE__);

	mouse_captured = true;
	SDL_SetRelativeMouseMode(SDL_TRUE);

	unsigned long lcg = time(NULL);
	osnInit((unsigned long (*)(void*))&lcg_rand, &lcg);
	simplexInit((unsigned long (*)(void*))&lcg_rand, &lcg);

	gameInitMap();
	printf("chunk...\n");
	game_chunk chunk;
	gameGenerateChunk(&chunk, 0, 0, 0);
	gameTesselateChunk(&meshes[2], &chunk);
	mlCreateRenderable(&renderables[2], game.materials + MAT_CHUNK, meshes + 2);
}

static void
gameExit() {
	int i;
	uiExit();
	for (i = 0; i < MAX_MATERIALS; ++i)
		if (game.materials[i].program != 0)
			glDeleteProgram(game.materials[i].program);
	for (i = 0; i < MAX_MESHES; ++i)
		if (meshes[i].vbo != 0)
			glDeleteBuffers(1, &meshes[i].vbo);
	for (i = 0; i < MAX_RENDERABLES; ++i)
		if (renderables[i].vao != 0)
			glDeleteVertexArrays(1, &renderables[i].vao);
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
			printf("%f, %f, %f p:%f, y:%f\n",
			       game.camera.offset.x, game.camera.offset.y, game.camera.offset.z,
			       game.camera.pitch, game.camera.yaw);
		else if (event->key.keysym.sym == game.controls.wireframe)
			wireframe_mode = !wireframe_mode;
		break;
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

	// update player/input
	// update blocks
	// update creatures
	// update effects
}

static void
printMatrix(ml_matrix* m) {
	printf("%.1f %.1f %.1f %.1f ", m->m[0], m->m[1], m->m[2], m->m[3]);
	printf("%.1f %.1f %.1f %.1f ", m->m[4], m->m[5], m->m[6], m->m[7]);
	printf("%.1f %.1f %.1f %.1f ", m->m[8], m->m[9], m->m[10], m->m[11]);
	printf("%.1f %.1f %.1f %.1f\n", m->m[12], m->m[13], m->m[14], m->m[15]);
}

#define RGB2F(r, g, b) (float)(0x##r)/255.f, (float)(0x##g)/255.f, (float)(0x##b)/255.f

static void
gameRender(SDL_Point* viewport) {
	glCheck(__LINE__);

	glEnable(GL_DEPTH_TEST);
	glLogicOp(GL_INVERT);
	glDepthFunc(GL_LESS);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glClearColor(RGB2F(28, 30, 48), 1.f);
	glClearDepth(1.f);
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

	if (wireframe_mode)
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

	// draw blocks (tesselate in parallel?)
	// draw objects
	// draw creatures
	// draw players
	// draw effects
	// draw alphablended (sort in parallel?)
	// fbo effects?
	// draw ui

	mlFPSMatrix(mlGetMatrix(&game.modelview), game.camera.offset, game.camera.pitch, game.camera.yaw);

	static float f = 0.f;
	f += 0.01f;

	ml_vec4 amb_color = { RGB2F(61, 04, 5F), 0.4f };
	ml_vec4 fog_color = { RGB2F(28, 30, 48), 0.025f };
	ml_vec3 light_dir = { 0.5f, 1.f, 0.5f };
	ml_vec3 tlight;
	ml_matrix33 normalmat;

	light_dir.x = sin(fmod(f*0.33f, ML_TWO_PI));
	light_dir.y = cos(fmod(f*0.66f, ML_TWO_PI));

	gameDrawMap();
	mlPushMatrix(&game.modelview);
	mlGetRotationMatrix(&normalmat, mlGetMatrix(&game.modelview));
	tlight = mlMulMat33Vec(&normalmat, &light_dir);
	mlDrawBegin(&renderables[2]);
	mlUniformMatrix(renderables[2].material->projmat, mlGetMatrix(&game.projection));
	mlUniformMatrix(renderables[2].material->modelview, mlGetMatrix(&game.modelview));
	mlGetRotationMatrix(&normalmat, mlGetMatrix(&game.modelview));
	mlUniformMatrix33(renderables[2].material->normalmat, &normalmat);
	ml_vec3 chunk_offset = { -1, -1, -1 };
	mlUniformVec3(renderables[2].material->chunk_offset, &chunk_offset);
	mlUniformVec4(renderables[2].material->amb_color, &amb_color);
	mlUniformVec4(renderables[2].material->fog_color, &fog_color);
	mlUniformVec3(renderables[2].material->light_dir, &tlight);
	mlDrawEnd(&renderables[2]);
	mlPopMatrix(&game.modelview);

	// transform light into eye space
	mlPushMatrix(&game.modelview);
	mlGetRotationMatrix(&normalmat, mlGetMatrix(&game.modelview));
	tlight = mlMulMat33Vec(&normalmat, &light_dir);
	mlRotate(mlGetMatrix(&game.modelview), f, 0.f, 1.f, 0.f);
	mlTranslate(mlGetMatrix(&game.modelview), 0.f, 0.5f, 0.f);
	mlDrawBegin(&renderables[0]);
	mlUniformMatrix(renderables[0].material->projmat, mlGetMatrix(&game.projection));
	mlUniformMatrix(renderables[0].material->modelview, mlGetMatrix(&game.modelview));
	mlGetRotationMatrix(&normalmat, mlGetMatrix(&game.modelview));
	mlUniformMatrix33(renderables[0].material->normalmat, &normalmat);
	mlUniformVec4(renderables[0].material->amb_color, &amb_color);
	mlUniformVec4(renderables[0].material->fog_color, &fog_color);
	mlUniformVec3(renderables[0].material->light_dir, &tlight);
	mlDrawEnd(&renderables[0]);
	mlPopMatrix(&game.modelview);

	mlPushMatrix(&game.modelview);
	mlGetRotationMatrix(&normalmat, mlGetMatrix(&game.modelview));
	tlight = mlMulMat33Vec(&normalmat, &light_dir);
	mlTranslate(mlGetMatrix(&game.modelview), 1.5f, 0.4f, 0.f);
	mlRotate(mlGetMatrix(&game.modelview), -f, 0.f, 1.f, 0.f);
	mlDrawBegin(&renderables[1]);
	mlUniformMatrix(renderables[1].material->projmat, mlGetMatrix(&game.projection));
	mlUniformMatrix(renderables[1].material->modelview, mlGetMatrix(&game.modelview));
	mlGetRotationMatrix(&normalmat, mlGetMatrix(&game.modelview));
	mlUniformMatrix33(renderables[1].material->normalmat, &normalmat);
	mlUniformVec4(renderables[1].material->amb_color, &amb_color);
	mlUniformVec4(renderables[1].material->fog_color, &fog_color);
	mlUniformVec3(renderables[1].material->light_dir, &tlight);
	mlDrawEnd(&renderables[1]);
	mlPopMatrix(&game.modelview);

	if (wireframe_mode)
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	ml_vec3 mc = mlVec3Scalef(mlVec3Normalize(light_dir), 4.f);
	ml_vec3 mc2 = mlVec3Scalef(mlVec3Normalize(light_dir), 5.f);
	uiDebugLine(mc, mc2, 0xffff7f00);
	uiDebugSphere(mc2, 0.15f, 0xffcf9f3f);

	ml_vec3 origo = { 0, 0, 0 };
	ml_vec3 xaxis = { 1, 0, 0 };
	ml_vec3 yaxis = { 0, 1, 0 };
	ml_vec3 zaxis = { 0, 0, 1 };
	uiDebugLine(origo, xaxis, 0xff00ff00);
	uiDebugLine(origo, yaxis, 0xffff0000);
	uiDebugLine(origo, zaxis, 0xff0000ff);

	uiDrawDebug(&game.projection, &game.modelview);


	uiText(5, 5, 0xafaaaaaa, "%g", osnNoise(game.camera.offset.x, game.camera.offset.y, game.camera.offset.z));
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
	              0.1f, 100.f);

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
		gameRender(&sz);

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
