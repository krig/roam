#pragma once

#include "voxelmap.h"

/*

  shared data for the game

 */

#define DAY_LENGTH 1200.0 /* seconds */

enum E_Materials {
	MAT_BASIC,
	MAT_UI,
	MAT_DEBUG,
	MAT_CHUNK,
	MAX_MATERIALS
};

struct camera_t {
	ml_dvec3 pos;
	float pitch;
	float yaw;
};

struct controls_t {
	int left;
	int right;
	int forward;
	int backward;
	int jump;
	int sprint;
	int crouch;
	int interact;
	int primary_action;
	int secondary_action;
	int wireframe;
	int debuginfo;
	int exitgame;
};

struct game_t {
	struct camera_t camera;
	struct controls_t controls;
	ml_material materials[MAX_MATERIALS];
	ml_matrixstack projection;
	ml_matrixstack modelview;
	game_map map;

	double time_of_day; // (0 - 1 looping: 0 is midday, 0.5 is midnight)
	ml_vec3 amb_light;
	ml_vec4 fog_color;
	ml_vec3 light_dir;

	bool fast_day_mode;
	bool debug_mode;
};

extern struct game_t game;

static inline ml_chunk cameraChunk() {
	ml_chunk c = { floor(round(game.camera.pos.x) / CHUNK_SIZE),
	               floor(round(game.camera.pos.z) / CHUNK_SIZE) };
	return c;
}

static inline ml_ivec3 cameraBlock() {
	ml_ivec3 b = { round(game.camera.pos.x),
	               round(game.camera.pos.y),
	               round(game.camera.pos.z) };
	return b;
}

static inline bool blockCompare(ml_ivec3 a, ml_ivec3 b) {
	return (a.x == b.x && a.y == b.y && a.z == b.z);
}

static inline ml_vec3 cameraChunkOffset() {
	ml_chunk c = cameraChunk();
	ml_vec3 ret = {
		game.camera.pos.x - (double)(c.x * CHUNK_SIZE),
		game.camera.pos.y,
		game.camera.pos.z - (double)(c.z * CHUNK_SIZE)
	};
	return ret;
}
