#include "common.h"
#include "math3d.h"
#include "game.h"
#include "player.h"
#include "script.h"
#include <lua.h>
#include <lauxlib.h>

//#define FLYSPEED 10.f
//#define RUNSPEED 4.f
//#define SPRINTSPEED 1.5f
//#define CROUCHSPEED 0.5f
//#define JUMPFORCE 10.f
//#define FLYUPDOWN 45.f
//#define MAX_VELOCITY 54.f
//#define GRAVITY -10.f

// Updated from lua once per frame
struct playervars {
	float flyspeed;
	float runspeed;
	float sprintspeed;
	float crouchspeed;
	float jumpforce;
	float flyupdown;
	float maxvel;
	float gravity;
};

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
        if (p->pos.y - FEETDISTANCE + p->vel.y < groundlevel) {
	        p->pos.y = groundlevel + FEETDISTANCE;
	        p->vel.y = 0.f;
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


void flight_move(struct player *p, float dt)
{
	struct inputstate *in = &game.input;
	player_look(dt);

	float speed = pv.flyspeed;
	vec3_t movedir = {0, 0, 0};
	if (in->move_left) movedir.x -= 1.f;
	if (in->move_right) movedir.x += 1.f;
	if (in->move_forward) movedir.z -= 1.f;
	if (in->move_backward) movedir.z += 1.f;
	if (in->move_jump) movedir.y += 1.f;
	if (in->move_crouch) movedir.y -= 1.f;
	if (in->move_sprint) speed += pv.sprintspeed;

	// normalize move dir? but strafing feels good!
	mat44_t m;
	m_setidentity(&m);
	m_rotate(&m, game.camera.yaw, 0, 1.f, 0);
	vec3_t movevec = m_vec3scale(m_matmulvec3(&m, &movedir), speed*dt);

	p->vel = m_vec3add(p->vel, movevec);

	player_dumb_collide();

	p->pos.x += p->vel.x * dt;
	p->pos.y += p->vel.y * dt;
	p->pos.z += p->vel.z * dt;
}

static void updatevar(lua_State *L, const char* name, float* v, float def)
{
	lua_pushstring(L, name);
	lua_gettable(L, -2);
	if (lua_isnumber(L, -1)) {
		*v = lua_tonumber(L, -1);
	} else {
		*v = def;
	}
	lua_pop(L, 1);
}

void player_tick(float dt)
{
	// update script vars
	lua_State *L = script_get_state();
	lua_getglobal(L, "player");
	if (lua_istable(L, -1)) {
		updatevar(L, "flyspeed", &pv.flyspeed, 10.f);
		updatevar(L, "runspeed", &pv.runspeed, 4.f);
		updatevar(L, "sprintspeed", &pv.sprintspeed, 1.5f);
		updatevar(L, "crouchspeed", &pv.crouchspeed, 0.5f);
		updatevar(L, "jumpforce", &pv.jumpforce, 10.f);
		updatevar(L, "flyupdown", &pv.flyupdown, 45.f);
		updatevar(L, "maxvel", &pv.maxvel, 54.f);
		updatevar(L, "gravity", &pv.gravity, -10.f);
	}

	struct player *p = &game.player;
	// update animations
	p->crouch_fade = m_clamp(p->crouch_fade + (p->crouching?dt:-dt)*5.f, 0.f, 1.f);
	//++game.player.bobcount; foot step sound


	if (game.camera.mode == CAMERA_FLIGHT) {
		flight_move(p, dt);
		return;
	}
}
