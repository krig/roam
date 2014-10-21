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
#include <assert.h>

static inline
void fatal_error(const char* msg, ...)
{
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
static inline
unsigned strmcpy(char *target, const char *source, unsigned length)
{
	const char *origin = target;
	if(length) { while(*source && --length) *target++ = *source++; *target = 0; }
	return target - origin;
}

// Replacements for str(n)cpy, str(n)cat
// Rationale: http://byuu.org/articles/programming/strcpycat
// length argument includes null-terminator
// returns: strlen(target)
static inline
unsigned strmcat(char *target, const char *source, unsigned length)
{
	const char *origin = target;
	while(*target && length) target++, length--;
	return (target - origin) + strmcpy(target, source, length);
}

char* sys_readfile(const char* filename);

uint64_t sys_urandom(void);
