#include "common.h"
#include "math3d.h"
#include "shaders.h"
#include "ui.h"
#include "objfile.h"

#define MAX_MATERIALS 64
#define MAX_MESHES 100
#define MAX_RENDERABLES 100

static SDL_Window* window;
static SDL_GLContext context;
static ml_matrixstack projection;
static ml_matrixstack modelview;
static ml_material materials[MAX_MATERIALS];
static ml_mesh meshes[MAX_MESHES];
static ml_renderable renderables[MAX_RENDERABLES];
static ml_vec3 camera_pos = { .v = { 0.283782f, -0.302f, 0.966538f } };
static float camera_pitch = 0.755668f;
static float camera_yaw = 0.245576f;
static bool mouse_captured = false;

enum E_Materials {
	MAT_BASIC,
	MAT_UI,
	MAT_DEBUG
};

static void
gameInit() {
	uint32_t top, bottom;
	top = 0xff24C6DC;
	bottom = 0xff514A9D;
	ml_vtx_pos_n_clr corners[8] = {
		{{ .v = {-0.5f,-0.5f,-0.5f } }, { .v = { 0, 0, 0 } }, bottom },
		{{ .v = { 0.5f,-0.5f,-0.5f } }, { .v = { 0, 0, 0 } }, bottom },
		{{ .v = {-0.5f,-0.5f, 0.5f } }, { .v = { 0, 0, 0 } }, bottom },
		{{ .v = { 0.5f,-0.5f, 0.5f } }, { .v = { 0, 0, 0 } }, bottom },
		{{ .v = {-0.5f, 0.5f,-0.5f } }, { .v = { 0, 0, 0 } }, top },
		{{ .v = {-0.5f, 0.5f, 0.5f } }, { .v = { 0, 0, 0 } }, top },
		{{ .v = { 0.5f, 0.5f,-0.5f } }, { .v = { 0, 0, 0 } }, top },
		{{ .v = { 0.5f, 0.5f, 0.5f } }, { .v = { 0, 0, 0 } }, top }
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
		{ .v = { 0, -1, 0 } },
		{ .v = { 0, 1, 0 } },
		{ .v = { 0, 0, 1 } },
		{ .v = { 0, 0, -1 } },
		{ .v = {-1, 0, 0 } },
		{ .v = { 1, 0, 0 } }
	};

	for (int i = 0; i < 6; ++i) {
		tris[i*6 + 0].n = normals[i];
		tris[i*6 + 1].n = normals[i];
		tris[i*6 + 2].n = normals[i];
		tris[i*6 + 3].n = normals[i];
		tris[i*6 + 4].n = normals[i];
		tris[i*6 + 5].n = normals[i];
	}

	mlCreateMaterial(&materials[MAT_BASIC], basic_vshader, basic_fshader);
	mlCreateMaterial(&materials[MAT_UI], ui_vshader, ui_fshader);
	mlCreateMaterial(&materials[MAT_DEBUG], debug_vshader, debug_fshader);

	mlCreateMesh(&meshes[0], 36, tris, ML_POS_3F | ML_N_3F | ML_CLR_4UB);
	mlCreateRenderable(&renderables[0], materials + MAT_BASIC, meshes + 0);

	{
		char* teapot = osReadWholeFile("data/teapot.obj");
		obj_mesh m;
		objLoad(&m, teapot, 0.1f);
		objCreateMesh(&meshes[1], &m, objGenNormalsFn);
		mlCreateRenderable(&renderables[1], materials + MAT_BASIC, meshes + 1);
		objFree(&m);
		free(teapot);
	}

	uiInit(materials + MAT_UI, materials + MAT_DEBUG);

	glCheck(__LINE__);

	mouse_captured = true;
	SDL_SetRelativeMouseMode(SDL_TRUE);
}

static void
gameExit() {
	int i;
	uiExit();
	for (i = 0; i < MAX_MATERIALS; ++i)
		if (materials[i].program != 0)
			glDeleteProgram(materials[i].program);
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
		if (event->key.keysym.sym == SDLK_ESCAPE)
			return false;
		else if (event->key.keysym.sym == SDLK_p)
			printf("%f, %f, %f p:%f, y:%f\n",
			       camera_pos.x, camera_pos.y, camera_pos.z,
			       camera_pitch, camera_yaw);
		break;
	default:
		break;
	}
	return true;
}

static void
playerLook(float dyaw, float dpitch) {
	camera_yaw = mlWrap(camera_yaw - dyaw, 0.f, ML_TWO_PI);
	camera_pitch = mlClamp(camera_pitch - dpitch, -ML_PI_2, ML_PI_2);
}

static void
playerMove(float right, float forward) {
	ml_vec3 xaxis, yaxis, zaxis;
	mlFPSRotation(camera_pitch, camera_yaw, &xaxis, &yaxis, &zaxis);
	camera_pos.x += xaxis.x * right;
	camera_pos.z += xaxis.z * right;
	camera_pos.x += zaxis.x * -forward;
	camera_pos.z += zaxis.z * -forward;
}

static void
playerJump(float speed) {
	camera_pos.y += speed;
}

static void
gameUpdate(float dt) {
	const float xsens = 1.f / ML_TWO_PI;
	const float ysens = 1.f / ML_TWO_PI;
	int mouse_dx, mouse_dy;
	const Uint8* state;
	float speed = 2.f * dt;

	state = SDL_GetKeyboardState(NULL);
	if (state[SDL_SCANCODE_A])
		playerMove(-speed, 0.f);
	if (state[SDL_SCANCODE_D])
		playerMove(speed, 0.f);
	if (state[SDL_SCANCODE_W])
		playerMove(0.f, speed);
	if (state[SDL_SCANCODE_S])
		playerMove(0.f, -speed);
	if (state[SDL_SCANCODE_SPACE])
		playerJump(speed);
	if (state[SDL_SCANCODE_LSHIFT])
		playerJump(-speed);

	if (mouse_captured) {
		SDL_GetRelativeMouseState(&mouse_dx, &mouse_dy);
		playerLook(mouse_dx * dt * xsens, mouse_dy * dt * ysens);
	} else if (SDL_GetMouseState(0, 0) & SDL_BUTTON(SDL_BUTTON_LEFT)) {
			mouse_captured = true;
			SDL_SetRelativeMouseMode(SDL_TRUE);
	}
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

	glClearColor(RGB2F(28, 30, 48), 1.f);
	glClearDepth(1.f);
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

	mlFPSMatrix(mlGetMatrix(&modelview), camera_pos, camera_pitch, camera_yaw);

	static float f = 0.f;
	f += 0.01f;

	ml_vec4 amb_color = {  .v = { RGB2F(61, 04, 5F), 0.3f } };
	ml_vec4 fog_color = {  .v = { RGB2F(28, 30, 48), 0.15f } };
	ml_vec3 light_dir = {  .v = { 0.5f, 1.f, 0.5f } };
	ml_vec3 tlight;
	ml_matrix33 normalmat;

	light_dir.x = sin(fmod(f*0.33f, ML_TWO_PI));
	light_dir.y = cos(fmod(f*0.66f, ML_TWO_PI));

	mlPushMatrix(&modelview);

	// transform light into eye space
	mlGetRotationMatrix(&normalmat, mlGetMatrix(&modelview));
	tlight = mlMulMat33Vec(&normalmat, &light_dir);

	mlRotate(mlGetMatrix(&modelview), f, 0.f, 1.f, 0.f);
	mlTranslate(mlGetMatrix(&modelview), 0.f, 0.5f, 0.f);
	mlDrawBegin(renderables + 0);
	mlUniformMatrix(renderables[0].material->projmat, mlGetMatrix(&projection));
	mlUniformMatrix(renderables[0].material->modelview, mlGetMatrix(&modelview));
	mlGetRotationMatrix(&normalmat, mlGetMatrix(&modelview));
	mlUniformMatrix33(renderables[0].material->normalmat, &normalmat);
	mlUniformVec4(renderables[0].material->amb_color, &amb_color);
	mlUniformVec4(renderables[0].material->fog_color, &fog_color);
	mlUniformVec3(renderables[0].material->light_dir, &tlight);
	mlDrawEnd(renderables + 0);
	mlPopMatrix(&modelview);

	mlPushMatrix(&modelview);

	mlGetRotationMatrix(&normalmat, mlGetMatrix(&modelview));
	tlight = mlMulMat33Vec(&normalmat, &light_dir);

	mlRotate(mlGetMatrix(&modelview), -f, 0.f, 1.f, 0.f);
	mlTranslate(mlGetMatrix(&modelview), 0.f, -0.4f, 0.f);
	mlDrawBegin(renderables + 1);
	mlUniformMatrix(renderables[0].material->projmat, mlGetMatrix(&projection));
	mlUniformMatrix(renderables[0].material->modelview, mlGetMatrix(&modelview));
	mlGetRotationMatrix(&normalmat, mlGetMatrix(&modelview));
	mlUniformMatrix33(renderables[1].material->normalmat, &normalmat);
	mlUniformVec4(renderables[1].material->amb_color, &amb_color);
	mlUniformVec4(renderables[1].material->fog_color, &fog_color);
	mlUniformVec3(renderables[1].material->light_dir, &tlight);
	mlDrawEnd(renderables + 1);
	mlPopMatrix(&modelview);

	ml_vec3 mc = mlVec3Scalef(mlVec3Normalize(light_dir), 4.f);
	ml_vec3 mc2 = mlVec3Scalef(mlVec3Normalize(light_dir), 5.f);
	uiDebugLine(mc, mc2, 0xffff7f00);
	uiDebugSphere(mc2, 0.15f, 0xffcf9f3f);

	uiDrawDebug(&projection, &modelview);

	uiText(5, 5, 0xafaaaaaa, "hello world!");
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

	glEnable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);
	glLogicOp(GL_INVERT);
	glDepthFunc(GL_LESS);
	glCullFace(GL_BACK);
	SDL_GetWindowSize(window, &sz.x, &sz.y);
	glViewport(0, 0, sz.x, sz.y);

	mlInitMatrixStack(&projection, 3);
	mlInitMatrixStack(&modelview, 16);

	mlPerspective(mlGetMatrix(&projection), mlDeg2Rad(70.f),
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
					mlPerspective(mlGetMatrix(&projection), mlDeg2Rad(70.f),
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
