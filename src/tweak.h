#pragma once

enum VarType { VAR_FLOAT, VAR_INT, VAR_BOOL };

typedef struct var {
	void *val;
	int type;
	const char *name;
	union { float f; int i; bool b; } prev;
	union { float f; int i; } vmin;
	union { float f; int i; } vmax;
} var_t;

// Vars are tweakable values, values are loaded from data
// file and reloaded live
// can also be edited via the console
void init_fvar(float *v, float vmin, float vmax, const char *name);
void init_ivar(int *v, int vmin, int vmax, const char *name);
void init_bvar(bool *v, const char *name);

// commands are triggered via the console
// input is a string, output is an int
// <0 returned from cmd_invoke is error
void cmd_init(const char *name, int (*cb)(char *arg, void *userdata), void *userdata);
int cmd_invoke(const char *name, char *arg);

void set_var_callback(const char *var, void (*cb)(var_t *var, void *userdata), void *userdata);
var_t *get_var(const char *name);

void tweaks_init(void);
void tweaks_exit(void);
void tweaks_tick(void);


