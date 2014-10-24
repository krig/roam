#pragma once

enum VarType {
	VAR_FLOAT,
	VAR_INT,
	VAR_BOOL
};

typedef struct var {
	void* val;
	int type;
	const char* name;
	union {
		float f;
		int i;
		bool b;
	} prev;
	union {
		float f;
		int i;
	} vmin;
	union {
		float f;
		int i;
	} vmax;
} var_t;

void init_fvar(float* v, float vmin, float vmax, const char* name);
void init_ivar(int* v, int vmin, int vmax, const char* name);
void init_bvar(bool* v, const char* name);

void add_var_callback(const char* var, void (*cb)(var_t* var, void* userdata), void* userdata);
var_t *get_var(const char* name);

void tweaks_exit(void);
void tweaks_tick(float dt);

// display ui for tweak
void tweaks_show(const char* name);
bool tweaks_handle_event(SDL_Event* event);
