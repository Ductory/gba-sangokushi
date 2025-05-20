#ifndef _GBA_VIDEO_H
#define _GBA_VIDEO_H

#include "core.h"
#include "gba.h"
#include <windows.h>
#include <gdiplus.h>

/* translate color */

typedef struct {
	u16 r : 5;
	u16 g : 5;
	u16 b : 5;
	u16 reserved : 1;
} RGB16;

typedef struct {
	BYTE b, g, r, a;
} ARGB32;

#define u16toRGB16(c) (*(RGB16*)&(c))
#define RGB16tou16(c) (*(u16*)&(c))

#define u32toARGB32(c) (*(ARGB32*)&(c))
#define ARGB32tou32(c) (*(u32*)&(c))

ARGB32 RGB16toARGB32(RGB16 src);
RGB16 ARGB32toRGB16(ARGB32 src);

/* video */

typedef u8 (*tile_t)[32];

/**
 * @brief Screen data
 */
typedef struct scrdata_t {
	u16 tid : 10;
	u16 hf : 1;
	u16 vf : 1;
	u16 pb : 4;
} scrdata_t;

typedef RGB16 (*pal_t)[16];

typedef struct drawparam_t {
	PixelFormat pf;
	u8 (*tile_bank)[32];
	scrdata_t *map_data;
	pal_t pal_bank;
	u8 pal_num;
	u8 hf, vf;
	u32 img_w, img_h; // in tile
	u32 x, y, w, h;
	bool transparent;
	union {
		void *arr;
		void *arr_argb;
		void *arr_4bpp;
		void *arr_8bpp;
	};
} drawparam_t;

void draw_image(drawparam_t *param);
GpImage *create_image(INT width, INT height, PixelFormat pf);
ColorPalette *image_get_palette(GpImage *img);
bool image_set_palette(GpImage *img, ColorPalette *cp);

/* video (low-level) */
#define TILE_SIZE 0x20
#define TILE_BLOCK_SIZE 0x4000
#define PAL_BLOCK_SIZE 0x200
#define MAP_BLOCK_SIZE 0x800

#define OBJ_TILE_ADDR 0x6010000 // B4-5

#define GET_TILE_BASE(P) ((P) & 0x601C000)
#define GET_PAL_BASE(P) ((P) & 0x5000200)
#define GET_MAP_BASE(P) ((P) & 0x601F800)

#define GET_TILE_ADDR(b,i) (VRAM_BASE + (b) * TILE_BLOCK_SIZE + (i) * TILE_SIZE)
#define GET_MAP_ADDR(b,i) (VRAM_BASE + (b) * MAP_BLOCK_SIZE + (i) * 2)

void init_video(void);
void uninit_video(void);
void video_write(vp_t dest, void *src, u32 len);
void video_draw(drawparam_t *ii, u32 t, u32 p, u32 m);

#endif // _GBA_VIDEO_H
