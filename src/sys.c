#include "common.h"
#include <time.h>
#ifdef _MSC_VER
#include <windows.h>
/*
 * gettimeofday.c
 *    Win32 gettimeofday() replacement
 *
 * src/port/gettimeofday.c
 *
 * Copyright (c) 2003 SRA, Inc.
 * Copyright (c) 2003 SKC, Inc.
 *
 * Permission to use, copy, modify, and distribute this software and
 * its documentation for any purpose, without fee, and without a
 * written agreement is hereby granted, provided that the above
 * copyright notice and this paragraph and the following two
 * paragraphs appear in all copies.
 *
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE TO ANY PARTY FOR DIRECT,
 * INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES, INCLUDING
 * LOST PROFITS, ARISING OUT OF THE USE OF THIS SOFTWARE AND ITS
 * DOCUMENTATION, EVEN IF THE UNIVERSITY OF CALIFORNIA HAS BEEN ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * THE AUTHOR SPECIFICALLY DISCLAIMS ANY WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE.  THE SOFTWARE PROVIDED HEREUNDER IS ON AN "AS
 * IS" BASIS, AND THE AUTHOR HAS NO OBLIGATIONS TO PROVIDE MAINTENANCE,
 * SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.
 */
/* FILETIME of Jan 1 1970 00:00:00. */
static const unsigned __int64 epoch = ((unsigned __int64) 116444736000000000ULL);
/*
 * timezone information is stored outside the kernel so tzp isn't used anymore.
 *
 * Note: this function is not for Win32 high precision timing purpose. See
 * elapsed_time().
 */
int
gettimeofday(struct timeval * tp, struct timezone * tzp)
{
    FILETIME    file_time;
    SYSTEMTIME  system_time;
    ULARGE_INTEGER ularge;

    GetSystemTime(&system_time);
    SystemTimeToFileTime(&system_time, &file_time);
    ularge.LowPart = file_time.dwLowDateTime;
    ularge.HighPart = file_time.dwHighDateTime;

    tp->tv_sec = (long) ((ularge.QuadPart - epoch) / 10000000L);
    tp->tv_usec = (long) (system_time.wMilliseconds * 1000);

    return 0;
}
#else
#include <sys/time.h>
#endif
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


uint64_t sys_urandom()
{
	FILE* f = fopen("/dev/urandom", "rb");
	if (f == NULL) f = fopen("/dev/random", "rb");
	if (f == NULL) return time(NULL);
	uint64_t seed;
	fread(&seed, sizeof(uint64_t), 1, f);
	fclose(f);
	return seed;
}


char *sys_readfile(const char *filename)
{
	FILE *f = fopen(filename, "rb");
	if (f == NULL)
		return NULL;
	fseek(f, 0, SEEK_END);
	long flen = ftell(f);
	fseek(f, 0, SEEK_SET);
	char *data = (char *)malloc(flen + 1);
	fread(data, 1, flen, f);
	data[flen] = '\0';
	fclose(f);
	return data;
}


// reallocate if buffer size is too small, else reuse buffer
// returns buffer size
char* sys_readfile_realloc(const char* filename, char* buffer, size_t* len)
{
	FILE *f = fopen(filename, "rb");
	if (f == NULL)
		return NULL;
	fseek(f, 0, SEEK_END);
	long flen = ftell(f);
	fseek(f, 0, SEEK_SET);
	if ((long)*len < flen + 1) {
		buffer = (char *)realloc(buffer, flen + 1);
		*len = flen + 1;
	}
	fread(buffer, 1, flen, f);
	buffer[flen] = '\0';
	fclose(f);
	return buffer;
}


int sys_isfile(const char* filename)
{
	int ret;
	struct stat tmp;
	ret = stat(filename, &tmp);
	if (ret == -1)
		return 0;
	return 1;
}

int64_t sys_timems()
{
	struct timeval tv;
	if (gettimeofday(&tv, NULL) == 0)
		return (int64_t)tv.tv_sec * 1000 + (int64_t)tv.tv_usec / 1000;
	fatal_error("failed to get current time");
}
