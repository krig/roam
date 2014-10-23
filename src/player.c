#include "common.h"
#include "math3d.h"
#include "game.h"
#include "player.h"
#include "easing.h"

// TODO: make these tweakable from console
#define FLYSPEED 10.f
#define RUNSPEED 5.f
#define SPRINTSPEED 2.f
#define CROUCHSPEED 0.5f
#define JUMPCOUNT 0.2f
#define CROUCHJUMP 0.8f
#define JUMPFORCE 10.f
#define FLYUPDOWN 10.f
#define MAX_VELOCITY 54.f
#define GRAVITY -10.f

static
void player_look(float dt)
{
	const float xsens = 1.f / ML_TWO_PI;
	const float ysens = 1.f / ML_TWO_PI;
	float dyaw = game.input.mouse_xrel * dt * xsens;
	float dpitch = game.input.mouse_yrel * dt * ysens;
	game.camera.yaw = mlWrap(game.camera.yaw - dyaw, 0.f, ML_TWO_PI);
	game.camera.pitch = mlClamp(game.camera.pitch - dpitch, -ML_PI_2, ML_PI_2);
	game.input.mouse_xrel = 0;
	game.input.mouse_yrel = 0;
}

static
void player_move(void)
{
	float right;
	float forward;
	float speed;

	if (!game.input.move_left &&
		!game.input.move_right &&
		!game.input.move_forward &&
		!game.input.move_backward)
		return;


	right = (game.input.move_left && !game.input.move_right) ? -1.f : 0.f;
	right += (game.input.move_right && !game.input.move_left) ? 1.f : 0.f;
	forward = (game.input.move_forward && !game.input.move_backward) ? -1.f : 0.f;
	forward += (game.input.move_backward && !game.input.move_forward) ? 1.f : 0.f;

	if (game.camera.mode == CAMERA_FLIGHT)
		speed = FLYSPEED;
	else
		speed = RUNSPEED;

	if (game.input.move_sprint)
		speed *= SPRINTSPEED;
	if (game.input.move_crouch)
		speed *= CROUCHSPEED;

	ml_matrix m;
	ml_vec3 move = { right, 0, forward };
	ml_vec3 dmove;
	move = mlVec3Normalize(move);
	mlSetIdentity(&m);
	mlRotate(&m, game.camera.yaw, 0, 1.f, 0);
	dmove = mlMulMatVec3(&m, &move);
	dmove = mlVec3Scalef(dmove, speed);

	if ((game.camera.mode == CAMERA_FLIGHT) || game.player.onground) {
		game.player.vel.x += dmove.x;
		game.player.vel.z += dmove.z;
	}
}

static
void player_jump(float dt)
{
	game.player.jumpcount = mlMax(0.f, game.player.jumpcount - dt);
	if (!game.input.move_jump)
		return;

	switch (game.camera.mode) {
	case CAMERA_FLIGHT:
		game.player.jumpcount = JUMPCOUNT;
		game.player.vel.y += FLYUPDOWN * dt;
		break;
	default:
		if (game.player.onground && game.player.jumpcount <= 0.f) {
			game.player.jumpcount = JUMPCOUNT;
			game.player.onground = false;
			float jf = JUMPFORCE;
			if (game.input.move_crouch)
				jf *= CROUCHJUMP;
			game.player.vel.y += jf * dt;
		}
		break;
	}
}

static
void player_crouch(float dt)
{
	if (game.input.move_crouch && game.camera.mode == CAMERA_FLIGHT)
		game.player.vel.y -= -FLYUPDOWN * dt;
	if (!game.input.move_crouch)
		dt = -dt;
	game.player.crouch_fade = mlClamp(game.player.crouch_fade + dt*5.f, 0.f, 1.f);
}

static
void player_dumb_collide(ml_dvec3* pos, ml_vec3* move, ml_vec3* vel)
{
	ml_ivec3 preblock = { round(pos->x), round(pos->y), round(pos->z) };

	int groundblock = preblock.y;
        while (blockType(preblock.x, groundblock, preblock.z) != BLOCK_AIR)
                ++groundblock;
        while (blockType(preblock.x, groundblock, preblock.z) == BLOCK_AIR && groundblock > 0)
                --groundblock;
        float groundlevel = (float)groundblock + 0.5f;

        if (pos->y - FEETDISTANCE + move->y < groundlevel) {
	        pos->y = groundlevel + FEETDISTANCE;
	        move->y = 0.f;
	        game.player.onground = true;
        }
}

static
void player_collide(ml_dvec3* pos, ml_vec3* move, ml_vec3* vel)
{
	/*
	  	ml_ivec3 preblock = { round(pos.x), round(pos.y), round(pos.z) };

	{
		float offs = game.player.crouching ? CROUCHOFFSET : CAMOFFSET;
		float ext = game.player.crouching ? CROUCHEXTENT : PLAYEREXTENT;
		ml_vec3 pcenter = { pos.x, pos.y + offs, pos.z };
		ml_vec3 pextent = { 0.4f, ext, 0.4f };

		size_t n = 0;
		ml_ivec3 cand[8] = {
			preblock,
			ml_ivec3_offset(preblock, mlSign(vel.x), 0, 0),
			ml_ivec3_offset(preblock, 0, 0, mlSign(vel.z)),
			ml_ivec3_offset(preblock, mlSign(vel.x), 0, mlSign(vel.z)),
			ml_ivec3_offset(preblock, mlSign(vel.x), 1, 0),
			ml_ivec3_offset(preblock, 1, 0, mlSign(vel.z)),
			ml_ivec3_offset(preblock, mlSign(vel.x), 1, mlSign(vel.z)),
		};
		cand[7] = ml_ivec3_offset(preblock, 0, mlSign(vel.y) < 0 ? -1 : 2, 0);

		ml_ivec3 other[8];
		for (int i = 0; i < 8; ++i)
			if (blockinfo[blockTypeByCoord(cand[i])].density > WATER_DENSITY)
				other[n++] = cand[i];

		ml_vec3 hitpoint;
		ml_vec3 hitdelta;
		ml_vec3 normal;
		ml_vec3 sweeppos;
		if (sweep_aabb_into_blocks(pcenter, pextent, vel, other, n, &hitpoint, &hitdelta, &normal, &sweeppos)) {
			if (game.debug_mode) {
				ml_vec3 nh = hitpoint;
				nh.x -= playerChunk().x * CHUNK_SIZE;
				nh.z -= playerChunk().z * CHUNK_SIZE;
				ml_vec3 nh2 = mlVec3Add(nh, normal);
				ui_debug_point(nh, 0xffff0000);
				ui_debug_line(nh, nh2, 0xffff7f00);
				nh2 = mlVec3Add(nh, vel);
				ui_debug_line(nh, nh2, 0x6f444444);
			}
		}
	}

	pos = newpos;
	*/
}


void player_init()
{
	ml_dvec3 offs = { 0, OCEAN_LEVEL + 2, 0 };
	game.player.pos = offs;
	mlVec3Assign(game.player.vel, 0, 0, 0);
	game.player.jumpcount = 0;
	game.player.crouch_fade = 0;
	game.player.onground = false;
	game.player.sprinting = false;
	game.player.crouching = false;
}

void player_move_to_spawn()
{
	ml_dvec3 p = game.player.pos;
	while (blockType(p.x, p.y-2, p.z) != BLOCK_AIR ||
		blockType(p.x, p.y-1, p.z) != BLOCK_AIR ||
		blockType(p.x, p.y, p.z) != BLOCK_AIR ||
		blockType(p.x + 1, p.y, p.z) != BLOCK_AIR ||
		blockType(p.x - 1, p.y, p.z) != BLOCK_AIR ||
		blockType(p.x, p.y, p.z + 1) != BLOCK_AIR ||
		blockType(p.x, p.y, p.z - 1) != BLOCK_AIR)
		p.y += 1.0;
	game.player.pos = p;
}


void player_tick(float dt)
{
	// update pos/vel/acc depending on input and control mode
	// collide player against world

	game.player.sprinting = game.input.move_sprint;
	game.player.crouching = game.input.move_crouch;
	// accumulate frame impulses from input, gravity
	// and other physics events
	player_move();
	player_jump(dt);
	player_crouch(dt);
	player_look(dt);
	if (game.camera.mode != CAMERA_FLIGHT)
		game.player.vel.y += GRAVITY * dt;

	ml_dvec3 pos = game.player.pos;
	ml_vec3 vel = game.player.vel;

	ml_vec3 move;

	vel = mlClampVec3(vel, -MAX_VELOCITY, MAX_VELOCITY);
	move = mlVec3Scalef(vel, dt);

	// collide
	if (game.collisions_on)
		player_dumb_collide(&pos, &move, &vel);

	pos.x += move.x;
	pos.y += move.y;
	pos.z += move.z;


	// offset camera from position
	ml_vec3 offset = {0, 0, 0};
	switch (game.camera.mode) {
	case CAMERA_FLIGHT:
		offset.y = CAMOFFSET;
		break;
	case CAMERA_3RDPERSON: {
		ml_vec3 x, y, z;
		mlFPSRotation(game.camera.pitch, game.camera.yaw, &x, &y, &z);
		offset = mlVec3Scalef(z, 6.f);
	} break;
	default: {
		float d = enCubicInOut(game.player.crouch_fade);
		offset.y = CAMOFFSET * (1.f - d) + CROUCHCAMOFFSET * d;
	} break;
	}

	// update player data
	float friction = 0.014f;
	if (game.player.onground)
		friction += 0.03f;
	game.player.vel = mlVec3Scalef(vel, 1.f - friction);
	game.player.pos = pos;
	game.camera.pos.x = pos.x + offset.x;
	game.camera.pos.y = pos.y + offset.y;
	game.camera.pos.z = pos.z + offset.z;
}
