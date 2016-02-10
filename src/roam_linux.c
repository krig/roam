// main source file for Linux

#include <stdnoreturn.h>

#define ML_SWAP(a, b) do { __typeof__ (a) _swap_##__LINE__ = (a); (a) = (b); (b) = _swap_##__LINE__; } while (0)

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

#include <sys/time.h>

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

int64_t sys_timems()
{
	struct timeval tv;
	if (gettimeofday(&tv, NULL) == 0)
		return (int64_t)tv.tv_sec * 1000 + (int64_t)tv.tv_usec / 1000;
	fatal_error("failed to get current time");
}



int main(int argc, char* argv[]) {
	return roam_main(argc, argv);
}
