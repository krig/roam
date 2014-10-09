#pragma once

#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <float.h>

static inline void
roamError(const char* msg, ...) {
	char buf[512];
	va_list va_args;
	va_start(va_args, msg);
	vsnprintf(buf, 512, msg, va_args);
	va_end(va_args);
	fprintf(stderr, "Error: %s\n", buf);
	exit(1);
}

// Replacements for str(n)cpy, str(n)cat
// Rationale: http://byuu.org/articles/programming/strcpycat
// length argument includes null-terminator
// returns: strlen(target)
static inline unsigned
strmcpy(char *target, const char *source, unsigned length) {
	const char *origin = target;
	if(length) { while(*source && --length) *target++ = *source++; *target = 0; }
	return target - origin;
}

// Replacements for str(n)cpy, str(n)cat
// Rationale: http://byuu.org/articles/programming/strcpycat
// length argument includes null-terminator
// returns: strlen(target)
static inline unsigned
strmcat(char *target, const char *source, unsigned length) {
	const char *origin = target;
	while(*target && length) target++, length--;
	return (target - origin) + strmcpy(target, source, length);
}

static inline char*
osReadWholeFile(const char* filename) {
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

