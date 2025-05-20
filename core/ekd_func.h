#ifndef _EKD_FUNC_H
#define _EKD_FUNC_H

#include "gba.h"
#include "gba_video.h"


u32 ekd_uncompress(void *dest, const void *src);
void ekd_draw_HEXmap(drawparam_t *dp, u32 id);
void ekd_draw_avatar(drawparam_t *dp, u32 id);

#endif // _EKD_FUNC_H
