#include "core.h"
#include "utils/io.h"
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

/////////
// I/O //
/////////

u8 *PRAM, *VRAM, *OAM, *ROM; // Palette RAM, Video RAM, Object Attribute Memory, ROM
u32 ROM_size;

bool load_ROM(const char *name)
{
	free_ROM();
	return readfile(name, &ROM, &ROM_size);
}

COREAPI void free_ROM(void)
{
	free(ROM);
	ROM = NULL;
	ROM_size = 0;
}

COREAPI void write_ROM(const char *name)
{
	writefile(name, ROM, ROM_size);
}

bool check_ROM_pointer(u32 P)
{
	return P >= ROM_BASE && P < ROM_BASE + ROM_size;
}

//////////
// BIOS //
//////////

/* compress */

#define IS_IN_RANGE(p) (p < (u8*)src + src_size)
#define CHECK_IN_RANGE(p) if (!IS_IN_RANGE(p)) break

u32 BareComp(void *dest, const void *src, u32 src_size)
{
	*(u32*)dest = src_size << 8;
	memcpy(dest + 4, src, src_size);
	return src_size + 4;
}

u32 RLComp(void *dest, const void *src, u32 src_size)
{
	const u8 *p = src;
	u8 *dest_org = dest;
	*(u32*)dest = src_size << 8 | 0x30;
	dest += 4;
	while (p < (u8*)src + src_size) {
		u8 b = *p++; // curr byte
		u8 f = 1; // 1 if duplicate mode
		int cnt = 1; // length
		int dup = 1; // duplicate length
		while (IS_IN_RANGE(p)) {
			++cnt;
			if (*p++ == b) { // duplicate
				++dup;
				if (!f && dup == 3) { // terminate non-dup mode, begin dup mode
					p -= 3, cnt -= 3;
					break;
				}
			} else { // neq
				dup = 1;
				if (f) {
					if (dup >= 3) // dup len is enough, encode it
						break;
					f = 0; // dup len is too short, use non-dup mode
				}
			}
			if (dup == 130) // cannot longer
				break;
			if (!f && cnt == 128) { // try twice at most (check dup len is enough)
				CHECK_IN_RANGE(p);
				if (*p == b) {
					if (dup == 2) {
						p -= 2; cnt -= 2;
						break;
					}
					CHECK_IN_RANGE(p);
					if (p[1] == b)
						--p, --cnt;
				}
				break;
			}
		}
		if (f) {
			if (dup >= 3) { // dup mode
				*(u8*)dest++ = 0x80 | (dup - 3);
				*(u8*)dest++ = b;
			} else { // only if reach the end
				f = 0;
			}
		}
		if (!f) { // non-dup mode
			*(u8*)dest++ = cnt - 1;
			p -= cnt;
			for (int i = 0; i < cnt; ++i)
				*(u8*)dest++ = *p++;
		}
	}
	return (u8*)dest - dest_org;
}

u32 LZ77Comp(void *dest, const void *src, u32 src_size, bool lazy)
{
	const u8 *p = src;
	u8 *dest_org = dest;
	*(u32*)dest = src_size << 8 | 0x10;
	dest += 4;
	while (IS_IN_RANGE(p)) {
		u8 f = 0; // flag
		u8 *pf = dest++; // pointer of flag
		for (int i = 7; IS_IN_RANGE(p) && i >= 0; --i) {
			if (p == src) { // the first byte cannot appear before
				*(u8*)dest++ = *p++;
				continue;
			}
			// match
			int len = 0, of;
			for (const u8 *t = p - (u8*)src <= 0x1000 ? src : p - 0x1000; t < p; ++t) {
				while (*t != *p)
					++t;
				if (t >= p) // cannot find
					break;
				const u8 *p_org = p, *t_org = t;
				u8 j;
				for (j = 0; j < 18 && IS_IN_RANGE(p) && *t == *p; ++j, ++t, ++p);
				if (j > len) {
					len = j;
					of = p - t;
				}
				p = p_org; t = lazy ? t : t_org;
			}
			// encode
			if (len < 3) { // raw
				*(u8*)dest++ = *p++;
			} else { // offset + length
				f |= 1 << i;
				--of;
				*(u16*)dest = (len - 3) << 4 | (of & 0xF00) >> 8 | (of & 0xFF) << 8;
				dest += 2;
				p += len;
			}
		}
		*pf = f; // save flag
	}
	return (u8*)dest - dest_org;
}

typedef struct {u32 f; short l, r;} node;

static void heapify(node *a, short *x, int l, int r)
{
	int p = l, c = p * 2 + 1;
	while (c <= r) {
		if (c + 1 <= r && a[x[c]].f > a[x[c + 1]].f)
			++c;
		if (a[x[p]].f <= a[x[c]].f)
			return;
		short t = x[p];
		x[p] = x[c];
		x[c] = t;
		p = c, c = p * 2 + 1;
	}
}

static void map_tree(node *a, int i, u32 *map, u8 *bmap)
{
	static u32 v;
	static u8 b;
	if (a[i].l == -1) {
		int t = i;
		map[t] = v;
		bmap[t] = b;
		return;
	}
	v <<= 1; ++b;
	map_tree(a, a[i].l, map, bmap);
	v |= 1;
	map_tree(a, a[i].r, map, bmap);
	v >>= 1; --b;
}

u32 HuffComp(void *dest, const void *src, u32 src_size, bool bmode)
{
	const u8 *p = src;
	u8 *dest_org = dest;
	u8 bits = bmode ? 8 : 4;
	*(u32*)dest = src_size << 8 | 0x20 | bits;
	dest += 4;
	int n = 1 << bits;
	node a[511];
	short x[256];
	short cnt = 256;
	// initialize
	for (int i = 0; i < n; ++i)
		a[i].f = 0, a[i].l = -1, x[i] = i;
	// read frequency
	for (; IS_IN_RANGE(p); ++p)
		++a[*p].f;
	if (!bmode) { // 4-bit
		u32 b[16] = {};
		for (int i = 0; i < 256; ++i) {
			b[i >> 4] += a[i].f;
			b[i & 0xF] += a[i].f;
		}
		for (int i = 0; i < 16; ++i)
			a[i].f = b[i];
	}
	// build tree
	for (int i = n / 2 - 1; i >= 0; --i)
		heapify(a, x, i, n - 1);
	while (a[x[0]].f == 0) { // discard unused char
		x[0] = x[--n];
		heapify(a, x, 0, n - 1);
	}
	int i, j;
	while (n && (i = x[0], x[0] = x[--n], heapify(a, x, 0, n - 1), n)) {
		j = x[0];
		a[cnt].f = a[i].f + a[j].f;
		a[cnt].l = j;
		a[cnt].r = i;
		x[0] = cnt++;
		heapify(a, x, 0, n - 1);
	}
	// save tree
	u8 *tree = dest;
	*(u8*)dest++ = cnt - 256; // size
	short q[511], *qh = q, *qt = q;
	*qt++ = cnt - 1; // root
	for (int c = 0; qh < qt;) { // layer by layer
		i = *qh++;
		if (a[i].l == -1) {
			*(u8*)dest++ = i;
			--c;
			continue;
		}
		if (c >= 0x80) // table is too large
			return -1;
		*(u8*)dest++ = c++ >> 1 | (a[i].l < 256) << 7 | (a[i].r < 256) << 6;
		*qt++ = a[i].l;
		*qt++ = a[i].r;
	}
	// preprocess, create encode array
	u32 map[256]; // easy to prove length <= 32
	u8 bmap[256]; // code length
	map_tree(a, cnt - 1, map, bmap);
	// encode
	p = src;
	u32 *dest32 = (u32*)(tree + ((cnt - 255) << 1));
	u32 v = 0;
	i = 0x20;
	while (IS_IN_RANGE(p)) {
		u8 t = *p++;
		for (int j = 0; j < 8; j += bits, t >>= bits) {
			u8 b = t & ((1 << bits) - 1);
			if (i < bmap[b]) { // need next word
				*dest32++ = v | map[b] >> (bmap[b] - i);
				i += 0x20 - bmap[b];
				v = map[b] << i;
			} else {
				i -= bmap[b];
				v |= map[b] << i;
			}
		}
	}
	*dest32++ = v;
	return (u8*)dest32 - dest_org;
}

/* uncompress */

/**
 * if dest is NULL, we return required size
 */

#define CHECK_BUF() do { \
	u32 head = *(u32*)src; \
	size = head >> 8; /* most significant 3 bytes */ \
	if (!dest || size > *psize) { \
		*psize = size; \
		return; \
	} \
} while (0)


void RLUnComp(void *dest, const void *src, u32 *psize)
{
	int size;
	CHECK_BUF();
	const u8 *psrc = src + 4;
	u8 *pdest = dest;
	*psize = size;
	while (size > 0) {
		u8 b = *psrc++;
		int len = b & 0x7F;
		if (b & 0x80) { // duplicate bytes
			size -= len += 3;
			u8 b = *psrc++;
			while (len--)
				*pdest++ = b;
		} else {
			size -= len += 1;
			while (len--)
				*pdest++ = *psrc++;
		}
	}
}

void HuffUnComp(void *dest, const void *src, u32 *psize)
{
	int size;
	CHECK_BUF();
	const u8 *psrc = src;
	u32 *pdest = dest;
	*psize = size;
	const u8 *tree = psrc + 4;
	const u8 *root = tree + 1;
	u8 bits = *psrc & 0xF; // bits num once
	u8 n = (bits & 7) + 4;
	u32 *p = (u32*)(tree + ((*tree + 1) << 1)); // data
	const u8 *q = root;
	u8 cnt = 0;
	while (size > 0) {
		u32 v = *p++, t;
		for (int i = 0x20; --i >= 0; v <<= 1) {
			u8 f = v >> 0x1F;
			u8 b = *q << f;
			q = (u8*)((uintptr_t)q & ~1) + (((*q & 0x3F) + 1) << 1) + f; // query tree
			if (b & 0x80) { // is leaf
				t = t >> bits | *q << (0x20 - bits);
				q = root;
				if (++cnt == n) {
					*pdest++ = t;
					size -= 4;
					cnt = 0;
					if (size <= 0)
						return;
				}
			}
		}
	}
}

void LZ77UnComp(void *dest, const void *src, u32 *psize)
{
	int size;
	CHECK_BUF();
	const u8 *psrc = src + 4;
	u8 *pdest = dest;
	*psize = size;
	while (1) {
		u8 f = *psrc++; // flag
		for (int i = 7; i >= 0; --i) {
			u8 b = *psrc++;
			if (f >> i & 1) { // offset + length
				int len = (b >> 4) + 2;
				size -= len;
				int of = (*psrc++ | (b & 0xF) << 8) + 1;
				while (len-- >= 0) {
					*pdest = pdest[-of];
					++pdest;
				}
			} else { // raw
				*pdest++ = b;
			}
			if (--size <= 0)
				return;
		}
	}
}

void BareUnComp(void *dest, const void *src, u32 *psize)
{
	int size;
	CHECK_BUF();
	*psize = size;
	memcpy(dest, src + 4, size);
}

////////////
// sprite //
////////////

bool get_obj_size(u32 *w, u32 *h, u32 shape, u32 size)
{
	static const u32 rect_table0[] = {2, 4, 4, 8};
	static const u32 rect_table1[] = {1, 1, 2, 4};
	if (shape >= 3 || size >= 4)
		return false;
	switch (shape) {
		case 0: *w = 1 << size; *h = 1 << size; break;
		case 1: *w = rect_table0[size]; *h = rect_table1[size]; break;
		case 2: *w = rect_table1[size]; *h = rect_table0[size]; break;
	}
	return true;
}
