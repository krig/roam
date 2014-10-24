#include "common.h"
#include "math3d.h"
#include "ui.h"
#include "game.h"
#include "tweak.h"

struct event {
	void (*cb)(var_t *var, void *userdata);
	void *userdata;
	struct event *next;
};

struct var_data {
	var_t var;
	struct event *cb;
	struct var_data *next;
};

static struct var_data *vars = NULL;


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
	var->cb = NULL;
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
	var->cb = NULL;
	var->next = vars;
	vars = var;
}


void init_bvar(bool* v, const char *name)
{
	struct var_data *var;
	var = (struct var_data *)malloc(sizeof(struct var_data));
	var->var.val = v;
	var->var.type = VAR_BOOL;
	var->var.name = name;
	var->var.prev.b = *v;
	var->var.vmin.i = 0;
	var->var.vmax.i = 1;
	var->cb = NULL;
	var->next = vars;
	vars = var;
}


void add_var_callback(const char* var, void (*cb)(var_t* var, void* userdata), void* userdata)
{
	struct var_data *i;
	struct event *e;
	i = (struct var_data *)get_var(var);
	if (i == NULL)
		return;
	for (e = i->cb; e != NULL && e->next != NULL; e = e->next)
		;
	if (e == NULL) {
		e = (struct event *)malloc(sizeof(struct event));
		i->cb = e;
	} else {
		e->next = (struct event *)malloc(sizeof(struct event));
		e = e->next;
	}
	e->cb = cb;
	e->userdata = userdata;
	e->next = NULL;
}


var_t *get_var(const char* name)
{
	struct var_data *i;
	for (i = vars; i != NULL; i = i->next)
		if (strcmp(name, i->var.name) == 0)
			return &i->var;
	return NULL;
}


void tweaks_exit()
{
	struct var_data *i, *k;
	struct event *e, *f;
	for (i = vars; i != NULL;) {
		k = i;
		i = i->next;
		for (e = i->cb; e != NULL;) {
			f = e;
			e = e->next;
			free(f);
		}
		free(k);
	}
	vars = NULL;
}


// update prev value for tweaks
void tweaks_tick(float dt)
{
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
		if (changed) {
			struct event *e;
			for (e = i->cb; e != NULL; e = e->next)
				(e->cb)(&i->var, e->userdata);
		}
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


// display ui for tweak
void tweaks_show(const char* name)
{
}


bool tweaks_handle_event(SDL_Event* event)
{
	return false;
}
