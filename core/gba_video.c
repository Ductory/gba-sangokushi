#include "gba_video.h"
#include "utils/logger.h"

/////////////////////
// translate color //
/////////////////////

ARGB32 RGB16toARGB32(RGB16 src)
{
	return (ARGB32){.a = 255, .r = src.r << 3, .g = src.g << 3, .b = src.b << 3};
}

RGB16 ARGB32toRGB16(ARGB32 src)
{
	return (RGB16){.r = src.r >> 3, .g = src.g >> 3, .b = src.b >> 3};
}

///////////
// video //
///////////

typedef struct drawcxt_t {
	PixelFormat pf;
	u32 scan_w;
	void (*draw_fn)(struct drawcxt_t*, u32, u32, scrdata_t);
	u8 (*tile_bank)[32];
	ARGB pal_bank[16][16];
	u32 img_w, img_h;
	u8 hf, vf;
	bool transparent;
	union {
		void *arr;
		void *arr_argb;
		void *arr_4bpp;
		void *arr_8bpp;
	};
} drawcxt_t;

static void draw_indexed_tile_4bpp(drawcxt_t *cxt, u32 x, u32 y, scrdata_t sd)
{
	u8 (*arr)[cxt->scan_w] = cxt->arr_4bpp;
	u8 *tile = cxt->tile_bank[sd.tid];
	bool transparent = cxt->transparent;
	int i1, i2, j1, j2, stpi, stpj;
	if (cxt->hf ^ sd.hf) // horizontal flip
		i1 = 3, i2 = -1, stpi = -1;
	else
		i1 = 0, i2 = 4, stpi = 1;
	if (cxt->vf ^ sd.vf) // vertical flip
		j1 = 7, j2 = -1, stpj = -1;
	else
		j1 = 0, j2 = 8, stpj = 1;
	for (int j = j1; j != j2; j += stpj) {
		if (y + j < 0)
			continue;
		for (int i = i1; i != i2; i += stpi) {
			if ((x >> 1) + i < 0)
				continue;
			u8 c = *tile++;
			if (!sd.hf) // convention is different
				c = (c & 0xF) << 4 | (c >> 4);
			u8 a = c & 0xF0, b = c & 0xF;
			if (!transparent || a)
				arr[y + j][(x >> 1) + i] = arr[y + j][(x >> 1) + i] & 0xF | a;
			if (!transparent || b)
				arr[y + j][(x >> 1) + i] = arr[y + j][(x >> 1) + i] & 0xF0 | b;
		}
	}
}

static void draw_indexed_tile_8bpp(drawcxt_t *cxt, u32 x, u32 y, scrdata_t sd)
{
	u8 (*arr)[cxt->scan_w] = cxt->arr_8bpp;
	u8 *tile = cxt->tile_bank[sd.tid];
	bool transparent = cxt->transparent;
	int i1, i2, j1, j2, stpi, stpj;
	if (cxt->hf ^ sd.hf) // horizontal flip
		i1 = 7, i2 = -1, stpi = -1;
	else
		i1 = 0, i2 = 8, stpi = 1;
	if (cxt->vf ^ sd.vf) // vertical flip
		j1 = 7, j2 = -1, stpj = -1;
	else
		j1 = 0, j2 = 8, stpj = 1;
	for (int j = j1; j != j2; j += stpj) {
		if (y + j < 0)
			continue;
		for (int i = i1; i != i2; i += stpi) {
			if (x + i < 0)
				continue;
			u8 c = *tile++;
			u8 a = c & 0xF, b = c >> 4;
			if (!transparent || a)
				arr[y + j][x + i] = sd.pb << 4 | a;
			i += stpi;
			if (!transparent || b)
				arr[y + j][x + i] = sd.pb << 4 | b;
		}
	}
}

static void draw_tile_argb(drawcxt_t *cxt, u32 x, u32 y, scrdata_t sd)
{
	ARGB (*arr)[cxt->scan_w] = cxt->arr_argb;
	u8 *tile = cxt->tile_bank[sd.tid];
	bool transparent = cxt->transparent;
	ARGB (*pal_bank)[16] = cxt->pal_bank;
	int i1, i2, j1, j2, stpi, stpj;
	if (cxt->hf ^ sd.hf) // horizontal flip
		i1 = 7, i2 = -1, stpi = -1;
	else
		i1 = 0, i2 = 8, stpi = 1;
	if (cxt->vf ^ sd.vf) // vertical flip
		j1 = 7, j2 = -1, stpj = -1;
	else
		j1 = 0, j2 = 8, stpj = 1;
	for (int j = j1; j != j2; j += stpj) {
		if (y + j < 0)
			continue;
		for (int i = i1; i != i2; i += stpi) {
			if (x + i < 0)
				continue;
			u8 c = *tile++;
			u8 a = c & 0xF, b = c >> 4;
			if (!transparent || a)
				arr[y + j][x + i] = pal_bank[sd.pb][a];
			i += stpi;
			if (!transparent || b)
				arr[y + j][x + i] = pal_bank[sd.pb][b];
		}
	}
}

static void draw_map(drawcxt_t *cxt, void *map_data, u32 x, u32 y, u32 w, u32 h)
{
	scrdata_t (*map)[w] = map_data;
	int i1, i2, stpi, j1, j2, stpj;
	if (cxt->hf) // horizontal flip
		i1 = w - 1, i2 = -1, stpi = -1;
	else
		i1 = 0, i2 = w, stpi = 1;
	if (cxt->vf) // vertical flip
		j1 = h - 1, j2 = -1, stpj = -1;
	else
		j1 = 0, j2 = h, stpj = 1;
	for (int j = j1, j0 = 0; j != j2; j += stpj, ++j0)
		for (int i = i1, i0 = 0; i != i2; i += stpi, ++i0)
			cxt->draw_fn(cxt, (i0 << 3) + x, (j0 << 3) + y, map[j][i]);
}

static void translate_palbank(ARGB32 (*dst_bank)[16], RGB16 (*src_bank)[16], u32 pal_num)
{
	for (u32 i = 0; i < pal_num; ++i)
		for (u32 j = 0; j < 16; ++j)
			dst_bank[i][j] = RGB16toARGB32(src_bank[i][j]);
}

void draw_image(drawparam_t *dp)
{
	drawcxt_t cxt = {
		.pf = dp->pf,
		.tile_bank = dp->tile_bank,
		.arr = dp->arr,
		.img_w = dp->img_w,
		.img_h = dp->img_h,
		.hf = dp->hf,
		.vf = dp->vf,
		.transparent = dp->transparent
	};
	if (!cxt.pf)
		cxt.pf = PixelFormat32bppARGB;
	int stride = ((GetPixelFormatSize(cxt.pf) * (cxt.img_w << 3) + 31) & ~31) >> 3;
	switch (cxt.pf) {
		case PixelFormat4bppIndexed: 
			cxt.scan_w = stride;      cxt.draw_fn = draw_indexed_tile_4bpp; break; // u8*
		case PixelFormat8bppIndexed:
			cxt.scan_w = stride;      cxt.draw_fn = draw_indexed_tile_8bpp; break; // u8*
		case PixelFormat32bppARGB:
			translate_palbank((void*)&cxt.pal_bank, dp->pal_bank, dp->pal_num);
			cxt.scan_w = stride >> 2; cxt.draw_fn = draw_tile_argb;         break; // argb_t*
		default: LOG_E("Unsupported pixel format", __func__);
	}
	draw_map(&cxt, dp->map_data, dp->x, dp->y, dp->w ? : dp->img_w, dp->h ? : dp->img_h);
}

GpImage *create_image(INT width, INT height, PixelFormat pf)
{
	GpImage *img;
	int stride = ((GetPixelFormatSize(pf) * width + 31) & ~31) >> 3;
	BYTE *scan0 = calloc(stride, height);
	GdipCreateBitmapFromScan0(width, height, stride, pf, scan0, &img);
	return img;
}

ColorPalette *image_get_palette(GpImage *img)
{
	int size;
	if (GdipGetImagePaletteSize(img, &size) != Ok || !size)
		return NULL;
	ColorPalette *cp = malloc(size);
	if (GdipGetImagePalette(img, cp, size) != Ok)
		free(cp), cp = NULL;
	return cp;
}

bool image_set_palette(GpImage *img, ColorPalette *cp)
{
	return GdipSetImagePalette(img, cp) == Ok;
}

///////////////////////
// video (low-level) //
///////////////////////

void init_video(void)
{
	if (!PRAM)
		PRAM = calloc(PRAM_SIZE, 1);
	if (!VRAM)
		VRAM = calloc(VRAM_SIZE, 1);
	if (!OAM)
		OAM = calloc(OAM_SIZE, 1);
}

void uninit_video(void)
{
	free(PRAM);
	free(VRAM);
	free(OAM);
}

void video_write(vp_t dest, void *src, u32 len)
{
	if (dest - PRAM_BASE < PRAM_SIZE)
		memmove(P2p(dest), src, len);
	else if (dest - VRAM_BASE < VRAM_SIZE)
		memmove(V2p(dest), src, len);
	else if (dest - OAM_BASE < OAM_SIZE)
		memmove(O2p(dest), src, len);
}

void video_draw(drawparam_t *dp, u32 t, u32 p, u32 m)
{
	dp->tile_bank = V2p(VRAM_BASE + t * TILE_BLOCK_SIZE);
	dp->pal_bank = P2p(PRAM_BASE + p * PAL_BLOCK_SIZE);
	dp->pal_num = 16;
	dp->map_data = V2p(VRAM_BASE + m * MAP_BLOCK_SIZE);
	dp->w = dp->img_w = 32;
	dp->h = dp->img_h = 32;
	dp->transparent = true;
	draw_image(dp);
}

////////////
// import //
////////////

bool import_sprite(void *dest, GpImage *img, u32 bg_index)
{
	PixelFormat pf;
	GdipGetImagePixelFormat(img, &pf);
	if (pf != PixelFormat4bppIndexed)
		return false;

	UINT w, h;
	GdipGetImageWidth(img, &w);
	GdipGetImageHeight(img, &h);

	BitmapData data;
	GdipBitmapLockBits(img, &(GpRect){0, 0, w, h}, ImageLockModeRead, PixelFormat4bppIndexed, &data);
	u8 *p = data.Scan0, *q = dest;

	u32 stride = w >> 1;
	w >>= 3;
	h >>= 3;
	UINT n = w * h;
	for (u32 i = 0; i < n; ++i) {
		u32 row = i / w * 8;
		u32 col = i % w * 8 / 2;
		u8 *qt = q + i * 32;
		for (u32 j = 0; j < 32; ++j) {
			u32 c = p[(row + (j >> 2)) * stride + (col + (j & 3))];
			u32 a = c >> 4, b = c & 0xF;
			if (a == 0 || a == bg_index) a ^= bg_index;
			if (b == 0 || b == bg_index) b ^= bg_index;
			qt[j] = a | b << 4;
		}
	}

	GdipBitmapUnlockBits(img, &data);

	return true;
}
