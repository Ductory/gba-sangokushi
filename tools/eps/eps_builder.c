#include "eps.h"
#include "core/encoding.h"
#include "utils/io.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>


int main(int argc, const char *argv[])
{
	if (argc < 4) {
		printf("Usage: %s <src> <dst> <patch> <description>\n", argv[0]);
		puts(""
			 "  <src>         - Source file\n"
		     "  <dst>         - Destination file\n"
		     "  <patch>       - Patch file\n"
		     "  <description> - Description of the patch");
		return 1;
	}
	setlocale(LC_CTYPE, LC_UTF8);
	
	const char *src_name = argv[1];
	const char *dst_name = argv[2];
	const char *patch_name = argv[3];
	const char *desc = argv[4];

	u8 *src, *dst, *patch;
	u32 src_size, dst_size, patch_size;
	readfile(src_name, &src, &src_size);
	readfile(dst_name, &dst, &dst_size);
	// GBK -> UTF-8
	u32 n = (strlen(desc) + 1) * 2;
	char *desc_utf8 = malloc(n);
	utf8_gbk_s(desc_utf8, desc, n);
	// build
	buf_t *buf = new_buf(0);
	eps_build(buf, src, dst_size, dst, desc_utf8);
	writefile(patch_name, buf->buf, buf->size);

	free(src);
	free(dst);
	free(desc_utf8);
	del_buf(buf);
	return 0;
}
