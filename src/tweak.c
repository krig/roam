#include "common.h"
#include "math3d.h"
#include "ui.h"
#include "game.h"
#include "tweak.h"
#include <luajit-2.0/luajit.h>
#include <luajit-2.0/lua.h>
#include <luajit-2.0/lualib.h>
#include <luajit-2.0/lauxlib.h>

struct var_data {
	var_t var;
	void (*call)(var_t *var, void *userdata);
	void *userdata;
	struct var_data *next;
};

struct cmd {
	const char *name;
	int (*call)(char *arg, void *userdata);
	void *userdata;
	struct cmd *next;
};

static struct var_data *vars = NULL;
static struct cmd *cmds = NULL;
static const char *vars_data = "data/vars.lua";
static int reload_interval = 60; // frames
static int reload_counter = 0;
static lua_State *L;

// format of data file?
// maybe a simple line based format is enough:
// <name> <type> = <value>\n
// can also allow multiple values..
// <name> <type>* = <value> <value> <value>

void tweaks_init()
{
	L = luaL_newstate();
	luaJIT_setmode(L, 0, LUAJIT_MODE_ENGINE|LUAJIT_MODE_ON);
	luaL_openlibs(L);
	luaL_dostring(L, "print(\"LuaJIT loaded\")");
	luaL_loadfile(L, vars_data);
	reload_counter = reload_interval;
	tweaks_tick();
}


void tweaks_exit()
{
	struct var_data *i, *k;
	for (i = vars; i != NULL;) {
		k = i;
		i = i->next;
		free(k);
	}
	vars = NULL;
	lua_close(L);
	L = NULL;
}


// update prev value for tweaks
void tweaks_tick()
{
	if (reload_counter++ < reload_interval)
		return;
	reload_counter = 0;


	struct var_data *i;
	bool changed;
	for (i = vars; i != NULL; i = i->next) {
		switch (i->var.type) {
		case VAR_FLOAT:
			changed = (*(float*)i->var.val - i->var.prev.f) > ML_EPSILON;
			break;
		case VAR_INT:
			changed = *(int*)i->var.val != i->var.prev.i;
			break;
		case VAR_BOOL:
			changed = *(bool*)i->var.val != i->var.prev.b;
			break;
		}
		if (changed && i->call != NULL)
			(i->call)(&i->var, i->userdata);
		switch (i->var.type) {
		case VAR_FLOAT:
			i->var.prev.f = *(float*)i->var.val;
			break;
		case VAR_INT:
			i->var.prev.i = *(int*)i->var.val;
			break;
		case VAR_BOOL:
			i->var.prev.b = *(bool*)i->var.val;
			break;
		}
	}
}


void init_fvar(float *v, float vmin, float vmax, const char *name)
{
	struct var_data *var;
	var = (struct var_data *)malloc(sizeof(struct var_data));
	var->var.val = v;
	var->var.type = VAR_FLOAT;
	var->var.name = name;
	var->var.prev.f = *v;
	var->var.vmin.f = vmin;
	var->var.vmax.f = vmax;
	var->call = NULL;
	var->userdata = NULL;
	var->next = vars;
	vars = var;
}


void init_ivar(int *v, int vmin, int vmax, const char *name)
{
	struct var_data *var;
	var = (struct var_data *)malloc(sizeof(struct var_data));
	var->var.val = v;
	var->var.type = VAR_INT;
	var->var.name = name;
	var->var.prev.i = *v;
	var->var.vmin.i = vmin;
	var->var.vmax.i = vmax;
	var->call = NULL;
	var->userdata = NULL;
	var->next = vars;
	vars = var;
}


void init_bvar(bool *v, const char *name)
{
	struct var_data *var;
	var = (struct var_data *)malloc(sizeof(struct var_data));
	var->var.val = v;
	var->var.type = VAR_BOOL;
	var->var.name = name;
	var->var.prev.b = *v;
	var->var.vmin.i = 0;
	var->var.vmax.i = 1;
	var->call = NULL;
	var->userdata = NULL;
	var->next = vars;
	vars = var;
}


void cmd_init(const char *name, int (*cb)(char *arg, void *userdata), void *userdata)
{
	struct cmd *cmd;
	cmd = (struct cmd *)malloc(sizeof(struct cmd));
	cmd->name = name;
	cmd->call = cb;
	cmd->userdata = userdata;
	cmd->next = cmds;
	cmds = cmd;
}


int cmd_invoke(const char *name, char *arg)
{
	struct cmd *i;
	for (i = cmds; i != NULL; i = i->next)
		if (strcmp(name, i->name) == 0)
			return (*i->call)(arg, i->userdata);
	return -1;
}


void set_var_callback(const char *var, void (*cb)(var_t *var, void *userdata), void *userdata)
{
	struct var_data *i;
	if ((i = (struct var_data *)get_var(var)) != NULL) {
		i->call = cb;
		i->userdata = userdata;
	}
}


var_t *get_var(const char *name)
{
	struct var_data *i;
	for (i = vars; i != NULL; i = i->next)
		if (strcmp(name, i->var.name) == 0)
			return &i->var;
	return NULL;
}

