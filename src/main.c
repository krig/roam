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
#include "stb.h"

static SDL_Window* window;
static SDL_GLContext context;
static bool mouse_captured = false;

struct game_t game;

ml_tex2d blocks_texture;

static void reset_inputstate(void);

static
void game_init()
{
	mlLoadTexture2D(&blocks_texture, "data/blocks8-v1.png");

	game.camera.pitch = 0;
	game.camera.yaw = 0;
	mlVec3Assign(game.camera.pos, 0, 0, 0);
	game.fast_day_mode = false;
	game.debug_mode = true;
	game.collisions_on = true;
	game.camera.mode = CAMERA_FLIGHT;
	game.enable_ground = true;
	game.wireframe = false;

	struct controls_t default_controls = {
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

	reset_inputstate();

	printf("* Load materials + UI\n");
	mlCreateMaterial(&game.materials[MAT_BASIC], basic_vshader, basic_fshader);
	mlCreateMaterial(&game.materials[MAT_UI], ui_vshader, ui_fshader);
	mlCreateMaterial(&game.materials[MAT_DEBUG], debug_vshader, debug_fshader);
	mlCreateMaterial(&game.materials[MAT_CHUNK], chunk_vshader, chunk_fshader);
	mlCreateMaterial(&game.materials[MAT_CHUNK_ALPHA], chunk_vshader, chunkalpha_fshader);
	mlCreateMaterial(&game.materials[MAT_SKY], sky_vshader, sky_fshader);
	ui_init(game.materials + MAT_UI, game.materials + MAT_DEBUG);

	game.day = 0;
	game.time_of_day = 0;

	sky_init();
	player_init();
	map_init();
	player_move_to_spawn();
	glCheck(__LINE__);

	mouse_captured = false;
	//SDL_SetRelativeMouseMode(SDL_TRUE);
	//SDL_SetWindowGrab(window, SDL_TRUE);

}


static
void game_exit()
{
	map_exit();
	sky_exit();
	ui_exit();
	for (int i = 0; i < MAX_MATERIALS; ++i)
		mlDestroyMaterial(game.materials + i);
	mlDestroyMatrixStack(&game.projection);
	mlDestroyMatrixStack(&game.modelview);
	mlDestroyTexture2D(&blocks_texture);
}


static
void reset_inputstate(void)
{
	memset(&game.input, 0, sizeof(struct inputstate_t));
}

static
void capture_mouse(bool capture)
{
	SDL_SetRelativeMouseMode(capture ? SDL_TRUE : SDL_FALSE);
	SDL_SetWindowGrab(window, capture ? SDL_TRUE : SDL_FALSE);
	mouse_captured = capture;
}

extern bool alpha_sort_chunks;
extern bool alpha_sort_faces;

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
		else if (sym == SDLK_F7) {
			alpha_sort_faces = !alpha_sort_faces;
			printf("sort faces: %d\n", alpha_sort_faces);
			ui_add_console_line(stb_sprintf("sort faces: %d", alpha_sort_faces));
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
			printf("deleting picked block (%d, %d, %d)\n",
				game.input.picked_block.x,
				game.input.picked_block.y,
				game.input.picked_block.z);
			if (!mouse_captured) {
				printf("focus gained\n");
				capture_mouse(true);
			} else {
				if (blockTypeByCoord(game.input.picked_block) != BLOCK_AIR) {
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

			ivec3_t feet = playerBlock();
			ivec3_t head = feet;
			head.y += 1;
			if (blockTypeByCoord(game.input.prepicked_block) == BLOCK_AIR &&
			    !blockCompare(head, game.input.prepicked_block) &&
			    !blockCompare(feet, game.input.prepicked_block)) {
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

/*
static inline
ivec3_t ivec3_t_offset(ivec3_t v, int x, int y, int z) {
	ivec3_t to = { v.x + x, v.y + y, v.z + z };
	return to;
}

static inline
vec3_t ivec3_t_to_vec3(ivec3_t v) {
	return mlMakeVec3(v.x, v.y, v.z);
}

// sweep box 2 into box 1
static
bool sweep_aabb(vec3_t center, vec3_t extent, vec3_t center2, vec3_t extent2, vec3_t delta,
	vec3_t* sweep_pos, float* time, vec3_t* hitpoint, vec3_t* hitdelta, vec3_t* hitnormal)
{
	bool hit = false;
	if (delta.x == 0 && delta.y == 0 && delta.z == 0) {
		hit = mlTestAABBAABB_2(center, extent, center2, extent2, hitpoint, hitdelta, hitnormal);
		if (hit) {
			*time = 0;
			*sweep_pos = center2;
		}
	} else {
		hit = mlTestSegmentAABB(center2, delta, extent2, center, extent, time, hitpoint, hitdelta, hitnormal);
		if (hit) {
			sweep_pos->x = center2.x + delta.x * *time;
			sweep_pos->y = center2.y + delta.y * *time;
			sweep_pos->z = center2.z + delta.z * *time;
			vec3_t dir = mlVec3Normalize(delta);
			hitpoint->x += dir.x * extent2.x;
			hitpoint->y += dir.y * extent2.y;
			hitpoint->z += dir.z * extent2.z;
		}
	}
	return hit;
}

static
bool sweep_aabb_into_blocks(vec3_t center, vec3_t extent, vec3_t delta,
	ivec3_t* blocks, size_t nblocks,
	vec3_t* outhitpoint, vec3_t* outhitdelta, vec3_t* outhitnormal, vec3_t* outsweeppos)
{

	float nearest_time;
	float time;
	vec3_t hitpoint;
	vec3_t hitnormal;
	vec3_t hitdelta;
	vec3_t sweeppos;

	bool hit;
	vec3_t bcenter;
	vec3_t bextent = { 0.5f, 0.5f, 0.5f };

	hit = false;
	nearest_time = 1.f;
	for (int i = 0; i < nblocks; ++i) {
		bcenter = ivec3_t_to_vec3(blocks[i]);
		if (sweep_aabb(bcenter, bextent, center, extent, delta,
				&sweeppos, &time, &hitpoint, &hitdelta, &hitnormal)) {
			if (time < nearest_time) {
				hit = true;
				*outhitpoint = hitpoint;
				*outhitdelta = hitdelta;
				*outhitnormal = hitnormal;
				*outsweeppos = sweeppos;
			}
		}
	}

	return hit;
}
*/

static
void game_tick(float dt)
{
	player_tick(dt);
	ui_tick(dt);
	map_tick();
	sky_tick(dt);
	// update player/input
	// update blocks
	// update creatures
	// update effects

}

static
void game_draw(SDL_Point* viewport, float frametime)
{
	mat44_t view;
	frustum_t frustum;
	glCheck(__LINE__);

	glEnable(GL_DEPTH_TEST);
	glLogicOp(GL_INVERT);
	glDepthFunc(GL_LESS);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glClearColor(game.sky_light.x, game.sky_light.y, game.sky_light.z, 1.f);
	glClearDepth(1.f);
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

	if (game.wireframe)
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

	chunkpos_t camera = playerChunk();
	vec3_t viewcenter = playerChunkCameraOffset();

	if (game.camera.mode != CAMERA_3RDPERSON) {
		mlFPSMatrix(&view, viewcenter, game.camera.pitch, game.camera.yaw);
	}
	else {
		mlLookAt(&view,
		         viewcenter.x, viewcenter.y, viewcenter.z,
		         game.player.pos.x - camera.x*CHUNK_SIZE,
		         game.player.pos.y,
		         game.player.pos.z - camera.z*CHUNK_SIZE,
		         0, 1.f, 0);
	}

	mlCopyMatrix(mlGetMatrix(&game.modelview), &view);
	mlGetFrustum(&frustum, mlGetMatrix(&game.projection), &view);

	// crosshair
	ui_rect(viewport->x/2 - 1, viewport->y/2 - 5, 2, 10, 0x4fffffff);
	ui_rect(viewport->x/2 - 5, viewport->y/2 - 1, 10, 2, 0x4fffffff);

	if (game.enable_ground)
		map_draw(&frustum);

	if (game.wireframe)
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	sky_draw();

	if (game.wireframe)
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

	if (game.enable_ground)
		map_draw_alphapass();

	if (game.wireframe)
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	mat44_t invview;
	mlInvertOrthoMatrix(&invview, &view);


	{
		dvec3_t pp = game.camera.pos;
		vec3_t v = {frustum.planes[5].x, frustum.planes[5].y, frustum.planes[5].z};
		if (map_raycast(pp, v, 16, &game.input.picked_block, &game.input.prepicked_block))
			if (game.camera.mode == CAMERA_FPS)
				ui_debug_block(game.input.picked_block, 0xcff1c40f);
	}

	if (game.camera.mode == CAMERA_3RDPERSON) {
		float ext = (game.player.crouching ? CROUCHHEIGHT : PLAYERHEIGHT) * 0.5f;
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
		origo = mlMulMatVec3(&invview, &origo);
		xaxis = mlVec3Add(origo, xaxis);
		yaxis = mlVec3Add(origo, yaxis);
		zaxis = mlVec3Add(origo, zaxis);
		ui_debug_line(origo, xaxis, 0xff00ff00);
		ui_debug_line(origo, yaxis, 0xffff0000);
		ui_debug_line(origo, zaxis, 0xff0000ff);

		static float fps = 0;
		fps = (1.f / frametime) * 0.1f + fps * 0.9f;

		ui_rect(2, 2, 450, 60, 0x66000000);
		ui_text(4, 60 - 9, 0xffffffff,
			"pos: (%2.2g, %2.2g, %2.2g)\n"
			"vel: (%2.2f, %2.2f, %2.2f)\n"
			"chunk: (%d, %d)\n"
			"%s%s%s\n"
			"fps: %d, t: %1.3f",
			game.player.pos.x, game.player.pos.y, game.player.pos.z,
			game.player.vel.x, game.player.vel.y, game.player.vel.z,
			camera.x, camera.z,
			game.player.onground ? "onground " : "",
			game.player.crouching ? "crouching " : "",
			game.player.sprinting ? "sprinting " : "",
			(int)roundf(fps), game.time_of_day);
	}
	ui_draw_debug(&game.projection, &game.modelview);
	ui_draw(viewport);

	//if (mouse_captured)
	//	SDL_WarpMouseInWindow(window, viewport->x >> 1, viewport->y >> 1);
}

int main(int argc, char* argv[])
{
	SDL_Event event;
	SDL_Point sz;
	GLenum rc;

	if (argc == 3 && strcmp(argv[1], "objtest") == 0) {
		char* fdata = sys_readfile(argv[2]);
		obj_mesh obj;
		objLoad(&obj, fdata, 0.1f);
		printf("loaded %s: %zu verts, %zu faces\n", argv[2], obj.nverts / 3, obj.nindices / 3);
		free(fdata);
		objFree(&obj);
		exit(0);
	}

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
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
		fatal_error(SDL_GetError());

	context = SDL_GL_CreateContext(window);
	if (context == NULL)
		fatal_error(SDL_GetError());

	SDL_GL_MakeCurrent(window, context);
	//SDL_GL_SetSwapInterval(1);

	glewExperimental = GL_TRUE;
	if ((rc = glewInit()) != GLEW_OK)
		fatal_error((const char*)glewGetErrorString(rc));

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
	//SDL_GL_GetDrawableSize(window, &sz.x, &sz.y);
	glViewport(0, 0, sz.x, sz.y);

	mlInitMatrixStack(&game.projection, 3);
	mlInitMatrixStack(&game.modelview, 16);

	mlPerspective(mlGetMatrix(&game.projection), DEG2RAD(70.f),
	              (float)sz.x / (float)sz.y,
	              0.1f, 1024.f);

	game_init();
	glCheck(__LINE__);

	int64_t startms, nowms;
	float frametime;
	startms = (int64_t)SDL_GetTicks();

	game.game_active = true;
	while (game.game_active) {
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_WINDOWEVENT) {
				if (event.window.event == SDL_WINDOWEVENT_RESIZED) {
					//SDL_GL_GetDrawableSize(window, &sz.x, &sz.y);
					SDL_GetWindowSize(window, &sz.x, &sz.y);
					glViewport(0, 0, sz.x, sz.y);
					//sz.x = event.window.data1;
					//sz.y = event.window.data2;
				} else if (event.window.event == SDL_WINDOWEVENT_MAXIMIZED) {
					//SDL_GL_GetDrawableSize(window, &sz.x, &sz.y);
					SDL_GetWindowSize(window, &sz.x, &sz.y);
					glViewport(0, 0, sz.x, sz.y);
					mlPerspective(mlGetMatrix(&game.projection), DEG2RAD(70.f),
					              (float)sz.x / (float)sz.y,
					              0.1f, 1024.f);
				} else if (event.window.event == SDL_WINDOWEVENT_FOCUS_LOST) {
					printf("focus lost\n");
					capture_mouse(false);
				} else if (event.window.event == SDL_WINDOWEVENT_CLOSE) {
					goto exit;
				}
			} else if (!handle_event(&event)) {
				goto exit;
			}
		}

		nowms = (int64_t)SDL_GetTicks() - startms;
		if (nowms < 1)
			nowms = 1;
		frametime = (float)((double)nowms / 1000.0);
		game_tick(frametime);
		startms = (int64_t)SDL_GetTicks();
		game_draw(&sz, frametime);

		SDL_GL_SwapWindow(window);
		glCheck(__LINE__);
	}

exit:
	game_exit();
	SDL_GL_DeleteContext(context);
	SDL_DestroyWindow(window);
	SDL_Quit();
	return 0;
}
