// main source file for Windows

#define __restrict__
#define __typeof__ decltype
#define noreturn
#define inline __forceinline
#define snprintf _snprintf

#define ML_SWAP(a, b) do { a=(a+b) - (b=a); } while (0)

#include "stb.c"
#include "blocks.c"
#include "gen.c"
#include "geometry.c"
#include "map.c"
#include "math3d.c"
#include "noise.c"
#include "objfile.c"
#include "player.c"
#include "script.c"
#include "sky.c"
#include "stb.c"
#include "sys.c"
#include "u8.c"
#include "ui.c"
#include "main.c"

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

int64_t sys_timems()
{
	struct timeval tv;
	if (gettimeofday(&tv, NULL) == 0)
		return (int64_t)tv.tv_sec * 1000 + (int64_t)tv.tv_usec / 1000;
	fatal_error("failed to get current time");
}


uint64_t sys_urandom()
{
	// TODO
	return 0;
}


int main(int argc, char* argv[]) {
	return roam_main(argc, argv);
}
