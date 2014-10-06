#pragma once

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
	int chunk[3];
	ml_vec3 offset;
	float pitch;
	float yaw;
};

struct controls_t {
	int left;
	int right;
	int forward;
	int backward;
	int jump;
	int crouch;
	int interact;
	int primary_action;
	int secondary_action;
	int wireframe;
	int debuginfo;
	int exitgame;
};

struct blockinfo_t {
	const char* name; // short description
	// texture tile indices
	// top, bottom, left, right, front, back
	int tex[6];
};

struct game_map;

struct game_t {
	struct camera_t camera;
	struct controls_t controls;
	ml_material materials[MAX_MATERIALS];
	ml_matrixstack projection;
	ml_matrixstack modelview;
	struct game_map* map;

	double time_of_day; // (0 - 1 looping: 0 is midday, 0.5 is midnight)
	ml_vec4 ambient_color;
	ml_vec4 fog_color;
	ml_vec3 light_dir;
};

extern struct game_t game;
