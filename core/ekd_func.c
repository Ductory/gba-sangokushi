#include "gba_video.h"
#include "koei.h"
#include "ekd.h"
#include "ekd_func.h"
#include <stdlib.h>
#include <string.h>

u32 ekd_uncompress(void *dest, const void *src)
{
	u8 type = *(u8*)src;
	if (type && type != 0x10 && type != 0x24 && type != 0x28) // only bare, LZ77 and Huff supported
		return 0;
	u32 size = *(u16*)(src + 1);
	u8 *dst = malloc(size);
	koei_uncompress(dst, src, &size);
	// compared to KMD, the compress header is encoded into the raw data
	if ((type & 0x20) && *(u8*)dst) { // has been compressed by LZ77 again
		u32 size = *(u16*)(dst + 1);
		*(void**)dest = malloc(size);
		if (*(void**)dest) {
			LZ77UnComp(*(void**)dest, dst, &size);
			memmove(*(void**)dest, *(void**)dest + 4, size - 4);
		} else {
			size = 0;
		}
		free(dst);
	} else {
		if (type & 0x10)
			memmove(dst, dst + 4, size - 4);
		*(void**)dest = dst;
	}
	return size;
}


void ekd_draw_HEXmap(drawparam_t *dp, u32 id)
{
	u8 *HEXmapTilesBank, *HEXmapPalBank;
	if (HEXmapTypeTable[id]) {
		HEXmapTilesBank = HEXmapTilesBank0;
		HEXmapPalBank = HEXmapPalBank0;
	} else {
		HEXmapTilesBank = HEXmapTilesBank1;
		HEXmapPalBank = HEXmapPalBank1;
	}
	ekd_uncompress(&dp->tile_bank, HEXmapTilesBank);
	ekd_uncompress(&dp->pal_bank, HEXmapPalBank);
	ekd_uncompress(&dp->map_data, R2p(HEXmapMapTable[id]));
	dp->pal_num = 16;
	dp->w = HEXmapSizeTable[id][0];
	dp->h = HEXmapSizeTable[id][1];
	draw_image(dp);
	free(dp->tile_bank);
	free(dp->pal_bank);
	free(dp->map_data);
}

void ekd_draw_avatar(drawparam_t *dp, u32 id)
{
	ekd_uncompress(&dp->tile_bank, R2p(AvatarTilebankTable[id]));
	ekd_uncompress(&dp->pal_bank, R2p(AvatarPalbankTable[id]));
	dp->pal_num = 1;
	dp->w = 8; dp->h = 8;
	u32 n = 64;
	u16 map_data[n];
	for (u32 i = 0; i < n; ++i)
		map_data[i] = i;
	dp->map_data = (scrdata_t*)map_data;
	draw_image(dp);
	free(dp->tile_bank);
	free(dp->pal_bank);
}
