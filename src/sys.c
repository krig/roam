#include "common.h"
#include <time.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#ifndef _MSC_VER
#include <sys/time.h>
#else
/**
 * This file has no copyright assigned and is placed in the Public Domain.
 * This file is part of the w64 mingw-runtime package.
 * No warranty is given; refer to the file DISCLAIMER.PD within this package.
 */
#include <windows.h>

struct timezone 
{
  __int32  tz_minuteswest; /* minutes W of Greenwich */
  bool  tz_dsttime;     /* type of dst correction */
};

struct timespec {
__int32    tv_sec;         /* seconds */
__int32    tv_nsec;        /* nanoseconds */
};

#define FILETIME_1970 116444736000000000ull /* seconds between 1/1/1601 and 1/1/1970 */
#define HECTONANOSEC_PER_SEC 10000000ull

int getntptimeofday (struct timespec *, struct timezone *);

int getntptimeofday (struct timespec *tp, struct timezone *z)
{
  int res = 0;
  union {
    unsigned long long ns100; /*time since 1 Jan 1601 in 100ns units */
    FILETIME ft;
  }  _now;
  TIME_ZONE_INFORMATION  TimeZoneInformation;
  DWORD tzi;

  if (z != NULL)
    {
      if ((tzi = GetTimeZoneInformation(&TimeZoneInformation)) != TIME_ZONE_ID_INVALID) {
	z->tz_minuteswest = TimeZoneInformation.Bias;
	if (tzi == TIME_ZONE_ID_DAYLIGHT)
	  z->tz_dsttime = 1;
	else
	  z->tz_dsttime = 0;
      }
    else
      {
	z->tz_minuteswest = 0;
	z->tz_dsttime = 0;
      }
    }

  if (tp != NULL) {
    GetSystemTimeAsFileTime (&_now.ft);	 /* 100-nanoseconds since 1-1-1601 */
    /* The actual accuracy on XP seems to be 125,000 nanoseconds = 125 microseconds = 0.125 milliseconds */
    _now.ns100 -= FILETIME_1970;	/* 100 nano-seconds since 1-1-1970 */
    tp->tv_sec = _now.ns100 / HECTONANOSEC_PER_SEC;	/* seconds since 1-1-1970 */
    tp->tv_nsec = (long) (_now.ns100 % HECTONANOSEC_PER_SEC) * 100; /* nanoseconds */
  }
  return res;
}

int gettimeofday (struct timeval *p, void *z)
{
 struct timespec tp;

 if (getntptimeofday (&tp, (struct timezone *) z))
   return -1;
 p->tv_sec=tp.tv_sec;
 p->tv_usec=(tp.tv_nsec/1000);
 return 0;
}

#endif


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
