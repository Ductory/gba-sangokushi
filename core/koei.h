#ifndef _KOEI_H
#define _KOEI_H

#include "core.h"
#include "gba.h"

/* Compress */
u32 koei_compress(void *dest, const void *src, u32 src_size, bool extra);
void koei_uncompress(void *dest, const void *src, u32 *psize);
int koei_get_compress_type(const void *src);
u32 koei_lz77_compress(void *dest, const void *src, u32 src_size);
void koei_lz77_uncompress(void *dest, const void *src, u32 *psize);


/* Indexed Character */
typedef u16 ichar_t;

typedef struct codeparam_t {
    u16 *table;
	u32 n;
} codeparam_t;

u16 ichar2char(codeparam_t *cp, ichar_t ch);
ichar_t char2ichar(codeparam_t *cp, u16 ch);

#endif // _KOEI_H
