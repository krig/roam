#pragma once
// shared data for the game

#include "voxelmap.h"
#include "player.h"


#define DAY_LENGTH 1200.0 /* seconds */


enum E_Materials {
	MAT_BASIC,
	MAT_UI,
	MAT_DEBUG,
	MAT_CHUNK,
	MAT_CHUNK_ALPHA,
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
	dvec3_t pos;
	float pitch;
	float yaw;
	cameramode_t mode;
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
};


struct game_t {
	struct camera_t camera;
	struct player_t player;
	struct controls_t controls;
	struct inputstate_t input;
	material_t materials[MAX_MATERIALS];
	mtxstack_t projection;
	mtxstack_t modelview;
	struct game_map_t map;

	int day; // increases after every day/night cycle
	double time_of_day;
	double light_level;
	vec3_t amb_light;
	vec4_t fog_color;
	vec3_t light_dir;
	vec3_t sun_color;
	vec3_t sky_dark;
	vec3_t sky_light;

	bool fast_day_mode;
	bool debug_mode;
	bool enable_ground;
	bool game_active;
	bool collisions_on;
	bool wireframe;
};


extern struct game_t game;



static inline
chunkpos_t cameraChunk()
{
	chunkpos_t c = { floor(round(game.camera.pos.x) / CHUNK_SIZE),
	               floor(round(game.camera.pos.z) / CHUNK_SIZE) };
	return c;
}

static inline
ivec3_t cameraBlock()
{
	ivec3_t b = { round(game.camera.pos.x),
	               round(game.camera.pos.y),
	               round(game.camera.pos.z) };
	return b;
}

static inline
chunkpos_t playerChunk()
{
	chunkpos_t c = { floor(round(game.player.pos.x) / CHUNK_SIZE),
	               floor(round(game.player.pos.z) / CHUNK_SIZE) };
	return c;
}

static inline
ivec3_t playerBlock()
{
	ivec3_t b = { round(game.player.pos.x),
	               round(game.player.pos.y),
	               round(game.player.pos.z) };
	return b;
}

static inline
bool blockCompare(ivec3_t a, ivec3_t b)
{
	return (a.x == b.x && a.y == b.y && a.z == b.z);
}

static inline
vec3_t playerChunkCameraOffset()
{
	chunkpos_t c = playerChunk();
	vec3_t ret = {
		game.camera.pos.x - (double)(c.x * CHUNK_SIZE),
		game.camera.pos.y,
		game.camera.pos.z - (double)(c.z * CHUNK_SIZE)
	};
	return ret;
}
