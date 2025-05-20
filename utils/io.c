#include <stdio.h>
#include <stdlib.h>
#include "io.h"

/**
 * read a binary file to buffer
 * @param  name filename
 * @param  buf  pointer to buffer
 * @param  size size of file
 * @return      true if succeeded, false if failed
 */
bool readfile(const char *name, u8 **buf, u32 *size)
{
	FILE *fp = fopen(name, "rb");
	if (!fp)
		return false;
	bool r = true;
	fseek(fp, 0, SEEK_END);
	u32 n = ftell(fp);
	*size = n;
	fseek(fp, 0, SEEK_SET);
	*buf = malloc(n);
	if (!*buf) {
		r = false;
		goto clean;
	}
	fread(*buf, 1, n, fp);
clean:
	fclose(fp);
	return r;
}

/**
 * write a buffer to a binary file
 * @param  name filename
 * @param  buf  buffer
 * @param  size size of buffer
 */
void writefile(const char *name, u8 *buf, u32 size)
{
	FILE *fp = fopen(name, "wb");
	fwrite(buf, 1, size, fp);
	fclose(fp);
}
