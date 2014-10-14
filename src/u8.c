#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

/* returns a unicode character or EOF
   doesn't handle more than 4-byte sequences
   Valid utf-8 sequences look like this :
   0xxxxxxx
   110xxxxx 10xxxxxx
   1110xxxx 10xxxxxx 10xxxxxx
   11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
   111110xx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx
   1111110x 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx
 */
int u8_getc(FILE* fp) {
	int c[4];
	c[0] = getc(fp);
	if (c[0] < 0x80) return c[0];
	if (c[0] < 0xC2) goto error;
	c[1] = getc(fp);
	if (c[1] < 0) return c[1];
	if ((c[1] & 0xc0) != 0x80) goto error1;
	if (c[0] < 0xe0) {
		return (c[0] << 6) + c[1] - 0x3080;
	} else if (c[0] < 0xf0) {
		if (c[0] == 0xe0 && c[1] < 0xa0) goto error1; /* overlong */
		c[2] = getc(fp);
		if (c[2] < 0) return c[2];
		if ((c[2] & 0xc0) != 0x80) goto error2;
		return (c[0] << 12) + (c[1] << 6) + c[2] - 0xe2080;
	} else if (c[0] < 0xf5) {
		if (c[0] == 0xf0 && c[1] < 0x90) goto error1; /* overlong */
		if (c[0] == 0xf4 && c[1] >= 0x90) goto error1; /* > U+10FFFF */
		c[2] = getc(fp);
		if (c[2] < 0) return c[2];
		if ((c[2] & 0xc0) != 0x80) goto error2;
		c[3] = getc(fp);
		if (c[3] < 0) return c[3];
		if ((c[2] & 0xc0) != 0x80) goto error3;
		return (c[0] << 18) + (c[1] << 12) + (c[2] << 6) + c[3] - 0x3c82080;
	}
	/* > U+10FFFF */
	goto error;
error3: ungetc(c[3], fp);
error2: ungetc(c[2], fp);
error1: ungetc(c[1], fp);
error: return '?';
}
