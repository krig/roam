#include "common.h"
#include "math3d.h"

static const char* basic_vshader = "#version 130\n"
	"uniform mat4 projmat;\n"
	"uniform mat4 modelview;\n"
	"uniform mat4 normalmat;\n"
	"in vec3 position;\n"
	"in vec4 color;\n"
	"out vec4 out_color;\n"
	"void main() {\n"
	"out_color = color;\n"
	"gl_Position = projmat * modelview * vec4(position, 1);\n"
	"}\n";

static const char* basic_fshader = "#version 130\n"
	"precision highp float;\n"
	"in vec4 out_color;\n"
	"out vec4 fragment;\n"
	"void main() {\n"
	"fragment = out_color;\n"
	"}\n";


#define MAX_MATERIALS 64
#define MAX_MESHES 100
#define MAX_RENDERABLES 100

static SDL_Window* window;
static SDL_GLContext context;
static ml_matrixstack projection;
static ml_matrixstack modelview;
static material_t materials[MAX_MATERIALS];
static mesh_t meshes[MAX_MESHES];
static renderable_t renderables[MAX_RENDERABLES];

static void
gameInit() {
	uint32_t top, bottom;
	top = 0xff24C6DC;
	bottom = 0xff514A9D;
	vtx_pos_clr_t corners[8] = {
		{{-0.5f,-0.5f,-0.5f}, bottom },
		{{ 0.5f,-0.5f,-0.5f}, bottom },
		{{-0.5f,-0.5f, 0.5f}, bottom },
		{{ 0.5f,-0.5f, 0.5f}, bottom },
		{{-0.5f, 0.5f,-0.5f}, top },
		{{-0.5f, 0.5f, 0.5f}, top },
		{{ 0.5f, 0.5f,-0.5f}, top },
		{{ 0.5f, 0.5f, 0.5f}, top }
	};

	vtx_pos_clr_t tris[] = {
		corners[0],
		corners[1],
		corners[2],
		corners[1],
		corners[3],
		corners[2],

		corners[4],
		corners[5],
		corners[6],
		corners[6],
		corners[5],
		corners[7],

		corners[2],
		corners[3],
		corners[5],
		corners[3],
		corners[7],
		corners[5],

		corners[0],
		corners[4],
		corners[1],
		corners[1],
		corners[4],
		corners[6],

		corners[2],
		corners[4],
		corners[0],
		corners[2],
		corners[5],
		corners[4],

		corners[3],
		corners[1],
		corners[6],
		corners[3],
		corners[6],
		corners[7]
	};
	mlCreateMaterial(&materials[0], basic_vshader, basic_fshader);
	mlCreateMesh(&meshes[0], 36, tris);
	mlCreateRenderable(&renderables[0], materials + 0, meshes + 0);
}

static void
gameExit() {
	int i;
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
		break;
	default:
		break;
	}
	return true;
}

static void
gameUpdate() {
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
	glClearColor(RGB2F(28, 30, 48), 1.f);
	glClearDepth(1.f);
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

	static float pitch = 0.f;
	static float yaw = 0.f;
	pitch += 0.01f;
	if (pitch > ML_PI*0.5f)
		pitch -= ML_PI;
	yaw += 0.01f;
	if (yaw > ML_PI*2.f)
		yaw -= ML_PI*2.f;
	//mlFPSMatrix(mlGetMatrix(&modelview), 0, 0, 0, pitch, yaw);

	static float f = 0.f;
	f += 0.01f;

	mlLookAt(mlGetMatrix(&modelview),
	         sin(f), 2.5f, cos(f),
	         0, 0.5f, 0.0,
	         0, 1, 0);


	mlPushMatrix(&modelview);
	mlTranslate(mlGetMatrix(&modelview), 0.f, 0.5f, 0.f);
	mlDrawBegin(renderables + 0);
	mlBindProjection(renderables + 0, mlGetMatrix(&projection));
	mlBindModelView(renderables + 0, mlGetMatrix(&modelview));
	mlDrawEnd(renderables + 0);
	mlPopMatrix(&modelview);
}


int main(int argc, char* argv[]) {
	SDL_Event event;
	SDL_Point sz;
	GLenum rc;

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
		roamError(glewGetErrorString(rc));

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
				}
			} else if (!gameHandleEvent(&event)) {
				goto exit;
			}
		}

		gameUpdate();
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
