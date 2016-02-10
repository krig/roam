#include "common.h"
#include <time.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

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
