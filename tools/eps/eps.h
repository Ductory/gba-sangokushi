#ifndef _EPS_H
#define _EPS_H

#include "core/gba.h"
#include "utils/buffer.h"

int eps_apply(u8 **r, u32 *s, const u8 *patch, u32 patch_size);
int eps_check(u8 **r, u32 *s, const u8 *patch, u32 patch_size);
void eps_build(buf_t *buf, const u8 *src, u32 size, const u8 *dest, const char *desc);
char *eps_get_desc(const u8 *patch, u32 size);

#endif // _EPS_H
