#pragma once

struct camera_t;

struct player_t {
	ml_dvec3 pos;
	ml_vec3 vel;
	float jumpcount;
	float crouch_fade;
	bool onground;
	bool sprinting;
	bool crouching;
};

struct inputstate_t {
	ml_ivec3 picked_block;
	ml_ivec3 prepicked_block;
	bool enable_ground;
	bool move_sprint;
	bool move_left;
	bool move_right;
	bool move_forward;
	bool move_backward;
	bool move_jump;
	bool move_crouch;
	int mouse_xrel;
	int mouse_yrel;
};


void player_init(void);
void player_tick(float dt);
void player_move_to_spawn(void);


// TODO: Make tweakable
#define FEETDISTANCE 0.5f
#define CAMOFFSET 0.9f
#define CROUCHCAMOFFSET 0.6f
#define CROUCHCENTEROFFSET 0.1f
#define CENTEROFFSET 0.4f // pos + centeroffset = center of player
#define PLAYERHEIGHT 1.8f // half-height of player
#define CROUCHHEIGHT 1.2f
