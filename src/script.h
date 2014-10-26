#pragma once

void script_init(void);
void script_exit(void);
void script_tick(void);

// execute script code
int script_exec(const char *cmd);

// execute script file
int script_dofile(const char* filename);

// commands are triggered via the console or scripts
// input is a lua_State *
// output is number of results left on the stack
void script_defun(const char *name, int (*cb)(void *L));

double script_get(const char *name);
int script_get_i(const char *name);
bool script_get_b(const char *name);
