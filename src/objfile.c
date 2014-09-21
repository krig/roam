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
	while (wp == to) {
		if (!*buf)
			return false;
		// skip whitespace
		while (*buf && isspace(*buf))
			++buf;
		// skip comments
		if (*buf == '#') {
			while (*buf && *buf != '\n')
				++buf;
			continue;
		}
		// copy line data
		while (*buf && *buf != '\n') {
			*wp++ = *buf++;
			if (wp - to >= LINEBUF_SIZE)
				roamError("objLoad: Line too long");
		}
	}
	*data = buf;
	*wp = '\0';
	return true;
}

bool objLoad(const char* data) {
	char linebuf[LINEBUF_SIZE];

	while (readLine(linebuf, &data)) {
		printf("line: %s %d\n", linebuf, strlen(linebuf));
	}

	return false;
}

char* osReadWholeFile(const char* filename) {
	FILE* f = fopen(filename, "r");
	fseek(f, 0, SEEK_END);
	long flen = ftell(f);
	fseek(f, 0, SEEK_SET);
	char* data = (char*)malloc(flen + 1);
	fread(data, flen, 1, f);
	data[flen] = '\0';
	fclose(f);
	return data;
}

