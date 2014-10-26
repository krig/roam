#include "common.h"
#include "math3d.h"
#include "ui.h"
#include "game.h"
#include "script.h"
#include <luajit-2.0/luajit.h>
#include <luajit-2.0/lua.h>
#include <luajit-2.0/lualib.h>
#include <luajit-2.0/lauxlib.h>


static lua_State *L;


/*
** Based on luaB_print from lbaselib.c
** If your system does not support `stdout', you can just remove this function.
** If you need, you can define your own `print' function, following this
** model but changing `fputs' to put the strings at a proper place
** (a console window or a log file, for instance).
*/
static int script_print(lua_State *L)
{
	char tmp[2048] = {0};
	int n = lua_gettop(L);  /* number of arguments */
	int i;
	lua_getglobal(L, "tostring");
	for (i=1; i<=n; i++) {
		const char *s;
		lua_pushvalue(L, -1);  /* function to be called */
		lua_pushvalue(L, i);   /* value to print */
		lua_call(L, 1, 1);
		s = lua_tostring(L, -1);  /* get result */
		if (s == NULL)
			return luaL_error(L, LUA_QL("tostring") " must return a string to "
			                  LUA_QL("print"));
		if (i>1) strmcat(tmp, "\t", sizeof(tmp));
		strmcat(tmp, s, sizeof(tmp));
		lua_pop(L, 1);  /* pop result */
	}
	ui_add_console_line(tmp);
	return 0;
}

static int script_quit(lua_State *L)
{
	lua_pop(L, lua_gettop(L));
	game.game_active = false;
	return 0;
}

static int script_debug_mode(lua_State *L)
{
	bool onoff = true;
	if (lua_gettop(L) > 0) {
		int n = lua_tointeger(L, 1);
		onoff = !!n;
	}
	game.debug_mode = onoff;
	return 0;
}

static int script_wireframe(lua_State *L)
{
	bool onoff = true;
	if (lua_gettop(L) > 0) {
		int n = lua_tointeger(L, 1);
		onoff = !!n;
	}
	game.wireframe = onoff;
	return 0;
}

static int script_teleport(lua_State *L)
{
	// TODO: how to error handle in lua
	if (lua_gettop(L) != 3)
		return 0;
	game.player.pos.x = lua_tonumber(L, 1);
	game.player.pos.y = lua_tonumber(L, 2);
	game.player.pos.z = lua_tonumber(L, 3);
	lua_pop(L, 3);
	return 0;
}

void script_init()
{
	L = luaL_newstate();
	luaJIT_setmode(L, 0, LUAJIT_MODE_ENGINE|LUAJIT_MODE_ON);
	luaL_openlibs(L);
	lua_register(L, "print", script_print);
	lua_register(L, "quit", script_quit);
	lua_register(L, "debug", script_debug_mode);
	lua_register(L, "wireframe", script_wireframe);
	lua_register(L, "teleport", script_teleport);
}


void script_exit()
{
	lua_close(L);
	L = NULL;
}


void script_tick()
{
}


int script_exec(const char *cmd)
{
	int ret = luaL_dostring(L, cmd);
	int top = lua_gettop(L);
	const char* s;
	while (top > 0) {
		s = lua_tostring(L, -1);
		if (s != NULL)
			ui_add_console_line(s);
		lua_pop(L, 1);
		--top;
	}
	lua_settop(L, 0);
	return ret < 0 ? ret : 0;
}

int script_dofile(const char* filename)
{
	if (!sys_isfile(filename)) {
		printf("Script not found: %s\n", filename);
		return 0;
	}
	int ret = luaL_dofile(L, filename);
	return ret;
}


void script_def_f(float *v, float vmin, float vmax, const char *name)
{
}

void script_def_i(int *v, int vmin, int vmax, const char *name)
{
}

void script_def_b(bool *v, const char *name)
{
}


// commands are triggered via the console or scripts
// input is a lua_State *
// output is number of results left on the stack
void script_defun(const char *name, int (*cb)(void *L))
{
	lua_register(L, name, (lua_CFunction)cb);
}


float script_get_f(const char *name)
{
	return 0.f;
}


int script_get_i(const char *name)
{
	return 0;
}


bool script_get_b(const char *name)
{
	return false;
}



