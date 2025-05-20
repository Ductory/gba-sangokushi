#ifndef _GBA_H
#define _GBA_H

#include "core.h"
#include <stdbool.h>
#include <stdint.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;
typedef char c8;

typedef u32 vp_t; /* virtual pointer */

#define _(s) ((char*)(s))

extern u8 *ROM;
extern u32 ROM_size;

extern u8 *PRAM, *VRAM, *OAM;

#define WRAM_BASE 0x2000000
#define IRAM_BASE 0x3000000
#define IO_BASE 0x4000000
#define PRAM_BASE 0x5000000
#define VRAM_BASE 0x6000000
#define OAM_BASE 0x7000000
#define ROM_BASE 0x8000000

#define PRAM_SIZE 0x400
#define VRAM_SIZE 0x18000
#define OAM_SIZE 0x400

#define SCREEN_WIDTH 240
#define SCREEN_HEIGHT 160
#define SCREEN_SIZE (SCREEN_WIDTH * SCREEN_HEIGHT)
#define SCREEN_WIDTH_TILE 30
#define SCREEN_HEIGHT_TILE 20

/* Virtual Pointer Conversion */
#define W2p(P) ((u32)(uintptr_t)(P) - WRAM_BASE + (void*)WRAM)
#define P2p(P) ((u32)(uintptr_t)(P) - PRAM_BASE + (void*)PRAM)
#define V2p(P) ((u32)(uintptr_t)(P) - VRAM_BASE + (void*)VRAM)
#define O2p(P) ((u32)(uintptr_t)(P) - OAM_BASE + (void*)OAM)
#define R2p(P) ((u32)(uintptr_t)(P) - ROM_BASE + (void*)ROM)

#define p2P(p) ((u32)(uintptr_t)((u8*)(p) - PRAM) + PRAM_BASE)
#define p2V(p) ((u32)(uintptr_t)((u8*)(p) - VRAM) + VRAM_BASE)
#define p2O(p) ((u32)(uintptr_t)((u8*)(p) - OAM) + OAM_BASE)
#define p2R(p) ((u32)(uintptr_t)((u8*)(p) - ROM) + ROM_BASE)

/* I/O */

bool load_ROM(const char *name);
void free_ROM(void);
void write_ROM(const char *name);
bool check_ROM_pointer(u32 P);

/* BIOS */

u32 BareComp(void *dest, const void *src, u32 src_size);
u32 RLComp(void *dest, const void *src, u32 src_size);
u32 LZ77Comp(void *dest, const void *src, u32 src_size, bool lazy);
u32 HuffComp(void *dest, const void *src, u32 src_size, bool bmode);
#define HuffComp8(dest,src,src_size) HuffComp(dest, src, src_size, true)
#define HuffComp4(dest,src,src_size) HuffComp(dest, src, src_size, false)

void BareUnComp(void *dest, const void *src, u32 *psize);
void RLUnComp(void *dest, const void *src, u32 *psize);
void LZ77UnComp(void *dest, const void *src, u32 *psize);
void HuffUnComp(void *dest, const void *src, u32 *psize);

typedef enum GBACompressMode {
	CompressModeBare,
	CompressModeLZ77,
	CompressModeHuff,
	CompressModeRL
} GBACompressMode;

/* sprite */
bool get_obj_size(u32 *w, u32 *h, u32 shape, u32 size);

#endif // _GBA_H
