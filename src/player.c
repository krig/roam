#include "common.h"
#include "math3d.h"
#include "game.h"
#include "player.h"
#include "script.h"


static struct playervars pv;


static
void player_look(float dt)
{
	struct inputstate *in = &game.input;
	struct camera *cam = &game.camera;
	const float xsens = 1.f / ML_TWO_PI;
	const float ysens = 1.f / ML_TWO_PI;
	float dyaw = in->mouse_xrel * dt * xsens;
	float dpitch = in->mouse_yrel * dt * ysens;
	cam->yaw = m_wrap(cam->yaw - dyaw, 0.f, ML_TWO_PI);
	cam->pitch = m_clamp(cam->pitch - dpitch, -ML_PI_2, ML_PI_2);
	in->mouse_xrel = 0;
	in->mouse_yrel = 0;
}


static
void player_dumb_collide()
{
	struct player *p = &game.player;
	ivec3_t footblock = { round(p->pos.x), round(p->pos.y), round(p->pos.z) };

	int groundblock = footblock.y;
	while (is_collider(footblock.x, groundblock, footblock.z))
                ++groundblock;
	while (!is_collider(footblock.x, groundblock, footblock.z) && groundblock >= 0)
                --groundblock;
	if (groundblock < 0)
		return;

	float groundlevel = (float)groundblock + 0.5f;
        if (p->pos.y - FEETDISTANCE < groundlevel) {
	        p->pos.y = groundlevel + FEETDISTANCE;
	        p->walking = true;
        }
}

void player_init()
{
	struct player *p = &game.player;
	dvec3_t offs = { 0, OCEAN_LEVEL + 2, 0 };
	p->pos = offs;
	m_setvec3(p->vel, 0, 0, 0);
	p->crouch_fade = 0;
	p->walking = false;
	p->sprinting = false;
	p->crouching = false;
}

void player_move_to_spawn()
{
	dvec3_t p = game.player.pos;
	while (blocktype(p.x, p.y-2, p.z) != BLOCK_AIR ||
		blocktype(p.x, p.y-1, p.z) != BLOCK_AIR ||
		blocktype(p.x, p.y, p.z) != BLOCK_AIR ||
		blocktype(p.x + 1, p.y, p.z) != BLOCK_AIR ||
		blocktype(p.x - 1, p.y, p.z) != BLOCK_AIR ||
		blocktype(p.x, p.y, p.z + 1) != BLOCK_AIR ||
		blocktype(p.x, p.y, p.z - 1) != BLOCK_AIR)
		p.y += 1.0;
	game.player.pos = p;
}


void flight_move(struct player *p, float dt)
{
	struct inputstate *in = &game.input;
	player_look(dt);

	float speed = pv.accel;//pv.flyspeed;
	vec3_t movedir = {0, 0, 0};
	if (in->move_left) movedir.x -= 1.f;
	if (in->move_right) movedir.x += 1.f;
	if (in->move_forward) movedir.z -= 1.f;
	if (in->move_backward) movedir.z += 1.f;
	if (in->move_jump) movedir.y += 1.f;
	if (in->move_crouch) movedir.y -= 1.f;
	//if (in->move_sprint) speed += pv.sprintspeed;

	if (movedir.x != 0 || movedir.y != 0 || movedir.z != 0) {
		// normalize move dir? but strafing feels good!
		mat44_t m;
		m_setidentity(&m);
		m_rotate(&m, game.camera.yaw, 0, 1.f, 0);
		vec3_t movevec = m_vec3scale(m_matmulvec3(&m, &movedir), speed*dt);

		p->vel = m_vec3add(p->vel, movevec);
	}

	player_dumb_collide();

	// drag
	p->vel = m_vec3scale(p->vel, 1.f - pv.friction);

	p->pos.x += p->vel.x * dt;
	p->pos.y += p->vel.y * dt;
	p->pos.z += p->vel.z * dt;
}


void player_tick(float dt)
{
	// update script vars
	pv.accel = script_get("player.accel");
	pv.friction = script_get("player.friction");
	pv.gravity = script_get("player.gravity");

	struct player *p = &game.player;
	// update animations
	p->crouch_fade = m_clamp(p->crouch_fade + (p->crouching?dt:-dt)*5.f, 0.f, 1.f);
	++game.player.bobcount;
	if (game.player.bobcount > 200)
		game.player.bobcount = 0;


	if (game.camera.mode == CAMERA_FLIGHT) {
		flight_move(p, dt);
		return;
	}

	p->prev_chunk = p->chunk;
	p->prev_block = p->block;
	p->prev_pos = p->pos;
	p->prev_vel = p->vel;
}
