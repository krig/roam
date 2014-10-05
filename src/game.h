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
	int top;
	int bottom;
	int left;
	int right;
	int front;
	int back;
	int hitpoints; // 0 = indestructable
	int physics; // physics mode: 0 = no collision, 1 = solid (future: repel/bounce, slippery, water)
	int blend; // if true, block is transparent
	int light; // amount of light emitted by block
	int anim; // texture animation mode
	void (*mesher)(/* ... */); // if custom mesh, this callback generates verts for the block
};

struct game_map;

struct game_t {
	struct camera_t camera;
	struct controls_t controls;
	ml_material materials[MAX_MATERIALS];
	ml_matrixstack projection;
	ml_matrixstack modelview;
	struct game_map* map;
};

extern struct game_t game;
