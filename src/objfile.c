#include "common.h"
#include "math3d.h"

typedef struct obj_mesh {
	size_t nfaces;
	size_t nverts;
	size_t ntexcoords;
	size_t nnormals;
} obj_mesh;

#define LINEBUF_SIZE 1024

static bool
readLine(char* to, const char** data) {
	char* wp = to;
	const char* buf = *data;

retry:
	if (!*buf)
		return false;

	while (*buf && isspace(*buf))
		++buf;

	if (*buf == '#') {
		while (*buf && *buf != '\n')
			++buf;
		if (*buf)
			++buf;
		goto retry;
	}

	while (*buf && *buf != '\n') {
		*wp++ = *buf++;
		if (wp - to >= LINEBUF_SIZE)
			roamError("objLoad: Line too long");
	}

	if (*buf && wp == to)
		goto retry;

	*wp = '\0';
	return true;
}


bool objLoad(obj_mesh* mesh, const char* data) {
	char linebuf[LINEBUF_SIZE];

	while (readLine(linebuf, &data)) {
		printf("line: %s\n", linebuf);
	}

	return false;
}

