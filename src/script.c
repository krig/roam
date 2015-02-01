#include "common.h"
#include "math3d.h"
#include "ui.h"
#include "game.h"
#include "script.h"
#include "stb.h"


#define MAX_DEFUNS 32

struct defun_data {
	const char* name;
	void (*cb)(int, char**);
};

static struct defun_data defuns[MAX_DEFUNS];

static stb_sdict* vars;


static int is_boolean_true(const char* str)
{
	return
		stb_stricmp(str, "true") == 0 ||
		stb_stricmp(str, "1") == 0 ||
		stb_stricmp(str, "yes") == 0;
}


static void script_quit(int argc, char** argv)
{
	game.game_active = false;
}

static void script_debug_mode(int argc, char** argv)
{
	game.debug_mode = (argc > 0) ? is_boolean_true(argv[1]) : true;
}

static void script_wireframe(int argc, char** argv)
{
	game.wireframe = (argc > 0) ? is_boolean_true(argv[1]) : true;
}

static void script_teleport(int argc, char** argv)
{
	printf("teleport: %d\n", argc);
	if (argc == 2) {
		game.player.pos.x = atof(argv[1]);
		game.player.pos.z = atof(argv[2]);
	} else if (argc == 3) {
		game.player.pos.x = atof(argv[1]);
		game.player.pos.y = atof(argv[2]);
		game.player.pos.z = atof(argv[3]);
	}
}

void script_init()
{
	printf("script init\n");
	vars = stb_sdict_new(0);
	memset(defuns, 0, sizeof(defuns));
	script_defun("quit", script_quit);
	script_defun("debug", script_debug_mode);
	script_defun("wireframe", script_wireframe);
	script_defun("teleport", script_teleport);
}


void script_exit()
{
}


void script_tick()
{
}


static void script_call(int argc, char** argv)
{
	printf("call: %s\n", argv[0]);
	for (unsigned int i = 0; i < stb_arrcount(defuns); ++i) {
		if (strcmp(defuns[i].name, argv[0]) == 0) {
			(*defuns[i].cb)(argc, argv);
			break;
		}
	}
}


int script_exec(char *cmd)
{
	int count;
	char **tokens;
	tokens = stb_tokens_quoted(cmd, " \t", &count);
	if (count == 3 && strcmp(tokens[1], "=") == 0) {
		printf("defining var %s as %s\n", tokens[0], tokens[2]);
		void* prev = stb_sdict_change(vars, tokens[0], strdup(tokens[2]));
		if (prev != NULL)
			free(prev);
	} else if (count > 0) {
		script_call(count-1, tokens);
	}
	free(tokens);
	return 0;
}

int script_dofile(const char* filename)
{
	if (!sys_isfile(filename)) {
		printf("Script not found: %s\n", filename);
		return 0;
	}
	int len;
	char** lines = stb_stringfile_trimmed((char*)filename, &len, '#');
	for (int i = 0; i < len; ++i) {
		script_exec(lines[i]);
	}
	free(lines);
	return 0;
}


void script_defun(const char *name, void (*cb)(int argc, char** argv))
{
	for (unsigned int i = 0; i < stb_arrcount(defuns); ++i) {
		if (defuns[i].name == 0) {
			defuns[i].name = name;
			defuns[i].cb = cb;
			return;
		}
	}
	printf("Max defuns reached\n");
}

double script_get(const char *name)
{
	char* val = (char*)stb_sdict_get(vars, (char*)name);
	if (val)
		return atof(val);
	return 0.0;
}



