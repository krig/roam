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
	MAT_SKY,
	MAX_MATERIALS
};

typedef enum cameramode_t {
	CAMERA_FPS,
	CAMERA_3RDPERSON,
	CAMERA_FLIGHT,
	NUM_CAMERA_MODES
} cameramode_t;

struct camera_t {
	ml_dvec3 pos;
	float pitch;
	float yaw;
	cameramode_t mode;
};

struct player_t {
	ml_dvec3 pos;
	ml_vec3 velocity;
	float jumpcount;
	bool onground;
	bool crouching;
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
	struct player_t player;
	struct controls_t controls;
	ml_material materials[MAX_MATERIALS];
	ml_matrixstack projection;
	ml_matrixstack modelview;
	game_map map;

	int day; // increases after every day/night cycle
	double time_of_day;
	double light_level;
	ml_vec3 amb_light;
	ml_vec4 fog_color;
	ml_vec3 light_dir;
	ml_vec3 sun_color;
	ml_vec3 sky_dark;
	ml_vec3 sky_light;

	bool fast_day_mode;
	bool debug_mode;
	bool game_active;
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

static inline ml_chunk playerChunk() {
	ml_chunk c = { floor(round(game.player.pos.x) / CHUNK_SIZE),
	               floor(round(game.player.pos.z) / CHUNK_SIZE) };
	return c;
}

static inline ml_ivec3 playerBlock() {
	ml_ivec3 b = { round(game.player.pos.x),
	               round(game.player.pos.y),
	               round(game.player.pos.z) };
	return b;
}

static inline bool blockCompare(ml_ivec3 a, ml_ivec3 b) {
	return (a.x == b.x && a.y == b.y && a.z == b.z);
}

static inline ml_vec3 playerChunkCameraOffset() {
	ml_chunk c = playerChunk();
	ml_vec3 ret = {
		game.camera.pos.x - (double)(c.x * CHUNK_SIZE),
		game.camera.pos.y,
		game.camera.pos.z - (double)(c.z * CHUNK_SIZE)
	};
	return ret;
}
