#include "common.h"

struct Roam {
	SDL_Window* window;
	SDL_GLContext context;
} roam;

int main(int argc, char* argv[]) {
	SDL_Event event;
	SDL_Point sz;
	GLenum rc;

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);

	if (SDL_Init(SDL_INIT_EVERYTHING) < 0)
		return 1;

	roam.window = SDL_CreateWindow("roam",
	                               SDL_WINDOWPOS_UNDEFINED,
	                               SDL_WINDOWPOS_UNDEFINED,
	                               640, 320,
	                               SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
	if (roam.window == NULL)
		roam_error(SDL_GetError());

	roam.context = SDL_GL_CreateContext(roam.window);
	if (roam.context == NULL)
		roam_error(SDL_GetError());

	SDL_GL_MakeCurrent(roam.window, roam.context);

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

	SDL_GetWindowSize(roam.window, &sz.x, &sz.y);
	glViewport(0, 0, sz.x, sz.y);

	for (;;) {
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT)
				goto exit;
			if (event.type == SDL_KEYDOWN &&
			    event.key.keysym.sym == SDLK_ESCAPE)
				goto exit;
			if (event.type == SDL_WINDOWEVENT) {
				if (event.window.event == SDL_WINDOWEVENT_RESIZED) {
					SDL_GetWindowSize(roam.window, &sz.x, &sz.y);
					glViewport(0, 0, sz.x, sz.y);
				}
			}
		}

		glClearColor(0.5f, 0.2f, 0.1f, 1.f);
		glClearDepth(1.f);

		SDL_GL_SwapWindow(roam.window);
	}

exit:
	SDL_Quit();
	return 0;
}
