#pragma once

/*

  shared data for the game

 */

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

struct game_t {
	struct camera_t camera;
	ml_material materials[MAX_MATERIALS];
	ml_matrixstack projection;
	ml_matrixstack modelview;
};

extern struct game_t game;
