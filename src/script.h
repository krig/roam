#pragma once

typedef void* script_state;

void script_init(void);
void script_exit(void);
void script_tick(void);

// execute script code
int script_exec(char *cmd);

// execute script file
int script_dofile(const char* filename);

void script_defun(const char *name, void (*cb)(int, char**));

double script_get(const char *name);
