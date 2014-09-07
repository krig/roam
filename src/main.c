#include "common.h"
#include "math3d.h"

struct Roam {
	SDL_Window* window;
	SDL_GLContext context;
} roam;

const char* basic_vshader = "#version 130\n"
	"uniform mat4 MVP;\n"
	"in vec3 position;\n"
	"in vec4 in_color;\n"
	"out vec4 out_color;\n"
	"void main() {\n"
	"out_color = in_color;\n"
	"gl_Position = MVP * vec4(position, 1);\n"
	"}\n";

const char* basic_fshader = "#version 130\n"
	"precision highp float;\n"
	"in vec4 out_color;\n"
	"out vec4 fragment;\n"
	"void main() {\n"
	"fragment = out_color;\n"
	"}\n";

static GLuint
compile_shader(GLenum type, const char* source) {
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

static GLuint
link_program(GLuint vsh, GLuint fsh) {
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

static void
check_gl_error(int line) {
	GLenum err;
	char* msg;
	do {
		err = glGetError();
		switch (err) {
		case GL_INVALID_ENUM: msg = "GL_INVALID_ENUM"; break;
		case GL_INVALID_VALUE: msg = "GL_INVALID_VALUE"; break;
		case GL_INVALID_OPERATION: msg = "GL_INVALID_OPERATION"; break;
		case GL_INVALID_FRAMEBUFFER_OPERATION: msg = "GL_INVALID_FRAMEBUFFER_OPERATION"; break;
		case GL_OUT_OF_MEMORY: msg = "GL_OUT_OF_MEMORY"; break;
		default: msg = "(other)"; break;
		}
		if (err != GL_NO_ERROR)
			fprintf(stderr, "GL error (%d): (0x%x) %s\n", line, err, msg);
	} while (err != GL_NO_ERROR);
}

typedef struct material_t {
	GLuint program;
	GLuint mvp;
} material_t;

material_t basic_material;

typedef struct mesh_t {
	GLuint vao;
	GLuint vbo;
	GLenum type;
	size_t nelements;
} mesh_t;

mesh_t test_mesh;

typedef struct vertex_t {
	ml_vec3 pos;
	uint32_t clr;
} vertex_t;

static void
create_material(material_t* material, const char* vsource, const char* fsource) {
	GLuint vshader, fshader, program;
	vshader = compile_shader(GL_VERTEX_SHADER, vsource);
	fshader = compile_shader(GL_FRAGMENT_SHADER, fsource);
	if (vshader > 0 && fshader > 0) {
		program = link_program(vshader, fshader);
	}
	if (vshader != 0)
		glDeleteShader(vshader);
	if (fshader != 0)
		glDeleteShader(fshader);
	material->program = program;
	glUseProgram(program);
	material->mvp = glGetUniformLocation(program, "MVP");
	glUseProgram(0);
}

static void
create_mesh(mesh_t* mesh, size_t n, vertex_t* data) {
	glGenVertexArrays(1, &mesh->vao);
	glBindVertexArray(mesh->vao);
	glGenBuffers(1, &mesh->vbo);
	glBindBuffer(GL_ARRAY_BUFFER, mesh->vbo);
	glBufferData(GL_ARRAY_BUFFER, n * sizeof(vertex_t), data, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vertex_t), 0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, GL_BGRA, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(vertex_t), (const void*)(3*sizeof(float)));
	glEnableVertexAttribArray(1);
	glBindVertexArray(0);
	mesh->type = GL_TRIANGLES;
	mesh->nelements = n;
}

static void
game_init() {
	ml_vec3 corners[8] = {
		{-0.5f,-0.5f,-0.5f}, // bottom
		{ 0.5f,-0.5f,-0.5f},
		{-0.5f,-0.5f, 0.5f},
		{ 0.5f,-0.5f, 0.5f},
		{-0.5f, 0.5f,-0.5f}, // top
		{-0.5f, 0.5f, 0.5f},
		{ 0.5f, 0.5f,-0.5f},
		{ 0.5f, 0.5f, 0.5f}
	};

	vertex_t tris[] = {
		{ corners[0], 0xfff0ffff },
		{ corners[1], 0xffff0fff },
		{ corners[2], 0xfffff0ff },
		{ corners[1], 0xffffff0f },
		{ corners[3], 0xfffffff0 },
		{ corners[2], 0xff0fffff },

		{ corners[4], 0xff0fffff },
		{ corners[5], 0xfff0ffff },
		{ corners[6], 0xffff0fff },
		{ corners[6], 0xfffff0ff },
		{ corners[5], 0xffffff0f },
		{ corners[7], 0xfffffff0 },

		{ corners[2], 0xfffffff0 },
		{ corners[3], 0xffffff0f },
		{ corners[5], 0xfffff0ff },
		{ corners[3], 0xffff0fff },
		{ corners[7], 0xfff0ffff },
		{ corners[5], 0xff0fffff },

		{ corners[0], 0xffff0fff },
		{ corners[4], 0xfffff0ff },
		{ corners[1], 0xfff0ffff },
		{ corners[1], 0xff0fffff },
		{ corners[4], 0xfffffff0 },
		{ corners[6], 0xffffff0f },

		{ corners[2], 0xff0fffff },
		{ corners[4], 0xfffffff0 },
		{ corners[0], 0xfff0ffff },
		{ corners[2], 0xffffff0f },
		{ corners[5], 0xffff0fff },
		{ corners[4], 0xfffff0ff },

		{ corners[3], 0xfffffff0 },
		{ corners[1], 0xfff0ffff },
		{ corners[6], 0xffffff0f },
		{ corners[3], 0xffff0fff },
		{ corners[6], 0xfffff0ff },
		{ corners[7], 0xff0fffff }
	};
	create_material(&basic_material, basic_vshader, basic_fshader);
	check_gl_error(__LINE__);
	create_mesh(&test_mesh, 36, tris);
	check_gl_error(__LINE__);
}

static void
game_exit() {
	glDeleteProgram(basic_material.program);
	glDeleteBuffers(1, &test_mesh.vbo);
	glDeleteVertexArrays(1, &test_mesh.vao);
}

static void
game_handle_event(SDL_Event* event) {
}

static void
game_update() {
}

static void
print_matrix(ml_matrix* m) {
	printf("%.1f %.1f %.1f %.1f ", m->m[0], m->m[1], m->m[2], m->m[3]);
	printf("%.1f %.1f %.1f %.1f ", m->m[4], m->m[5], m->m[6], m->m[7]);
	printf("%.1f %.1f %.1f %.1f ", m->m[8], m->m[9], m->m[10], m->m[11]);
	printf("%.1f %.1f %.1f %.1f\n", m->m[12], m->m[13], m->m[14], m->m[15]);
}

static void
game_render(SDL_Point* viewport) {
	ml_matrix proj;
	ml_matrix modelview;
	ml_matrix mvp;

	glClearColor(0.1f, 0.1f, 0.1f, 1.f);
	glClearDepth(1.f);
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

	mlPerspective(&proj, mlDeg2Rad(70.f),
	              (float)viewport->x / (float)viewport->y,
	              0.1f, 100.f);

	static float pitch = 0.f;
	static float yaw = 0.f;
	pitch += 0.01f;
	if (pitch > ML_PI*0.5f)
		pitch -= ML_PI;
	yaw += 0.01f;
	if (yaw > ML_PI*2.f)
		yaw -= ML_PI*2.f;
	mlLoadFPSMatrix(&modelview, 0, 0, 0, pitch, yaw);

	static float f = 0.f;
	f += 0.01f;

	mlLookAt(&modelview,
	         sin(f), 2.5f, cos(f),
	         0, 0.5f, 0.0,
	         0, 1, 0);


	glUseProgram(basic_material.program);
	glBindVertexArray(test_mesh.vao);

	mlLoadMatrix(&mvp, &proj);
	mlTranslate(&modelview, 0.f, 0.5f, 0.f);
	mlMulMatrix(&mvp, &modelview);

	glUniformMatrix4fv(basic_material.mvp, 1, GL_FALSE, mvp.m);

	glDrawArrays(test_mesh.type, 0, test_mesh.nelements);

	glBindVertexArray(0);
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

	roam.window = SDL_CreateWindow("roam",
	                               SDL_WINDOWPOS_UNDEFINED,
	                               SDL_WINDOWPOS_UNDEFINED,
	                               1100, 550,
	                               SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
	if (roam.window == NULL)
		roam_error(SDL_GetError());

	roam.context = SDL_GL_CreateContext(roam.window);
	if (roam.context == NULL)
		roam_error(SDL_GetError());

	SDL_GL_MakeCurrent(roam.window, roam.context);
	SDL_GL_SetSwapInterval(1);

	glewExperimental = GL_TRUE;
	if ((rc = glewInit()) != GLEW_OK)
		roam_error(glewGetErrorString(rc));

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
	SDL_GetWindowSize(roam.window, &sz.x, &sz.y);
	glViewport(0, 0, sz.x, sz.y);

	game_init();
	check_gl_error(__LINE__);

	for (;;) {
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT ||
			    (event.type == SDL_KEYDOWN &&
			     event.key.keysym.sym == SDLK_ESCAPE)) {
				goto exit;
			} else if (event.type == SDL_WINDOWEVENT) {
				if (event.window.event == SDL_WINDOWEVENT_RESIZED) {
					glViewport(0, 0, event.window.data1, event.window.data2);
					sz.x = event.window.data1;
					sz.y = event.window.data2;
				} else if (event.window.event == SDL_WINDOWEVENT_MAXIMIZED) {
					SDL_GetWindowSize(roam.window, &sz.x, &sz.y);
					glViewport(0, 0, sz.x, sz.y);
				}
			} else {
				game_handle_event(&event);
			}
		}

		game_update();
		game_render(&sz);

		SDL_GL_SwapWindow(roam.window);
		check_gl_error(__LINE__);
	}

exit:
	game_exit();
	SDL_GL_DeleteContext(roam.context);
	SDL_DestroyWindow(roam.window);
	SDL_Quit();
	return 0;
}
