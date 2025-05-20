#include "koei.h"
#include "gba.h"
#include "utils/io.h"
#include "utils/logger.h"
#include <string.h>

u32 koei_compress(void *dest, const void *src, u32 src_size, bool extra)
{
	switch (*(u8*)src >> 4) {
		case CompressModeBare: return BareComp(dest, src, src_size);
		case CompressModeLZ77: return LZ77Comp(dest, src, src_size, extra);
		case CompressModeHuff: return HuffComp(dest, src, src_size, extra);
		case CompressModeRL:   return RLComp(dest, src, src_size);
		default: return -1;
	}
}

void koei_uncompress(void *dest, const void *src, u32 *psize)
{
	typedef void (*uncomp_t)(void*, const void*, u32*);
	static const uncomp_t table[] = {
		BareUnComp, LZ77UnComp, HuffUnComp, RLUnComp
	};
	if (*(u8*)src >= 0x40) { // invalid type
		*psize = 0;
		return;
	}
	table[*(u8*)src >> 4](dest, src, psize);
}

int koei_get_compress_type(const void *src)
{
	return *(u8*)src >> 4;
}

static constexpr u32 BitMaskTable[] = {
	0, 1, 3, 7, 0xF, 0x1F, 0x3F, 0x7F, 0xFF,
	0x1FF, 0x3FF, 0x7FF, 0xFFF, 0x1FFF, 0x3FFF, 0x7FFF, 0xFFFF
};

typedef struct uncompress_t {
	const u8 *input_ptr;
	u16 bit_buffer;
	u16 bit_extension;
	u8 remain_bits;
	u8 control_byte;
} uncompress_t;

static void read_bits(uncompress_t *D, u32 num)
{
	// HI to LO
	D->bit_buffer <<= num; // throw trash
	if (D->remain_bits >= num) {
		D->remain_bits -= num;
		D->bit_buffer |= (D->bit_extension >> D->remain_bits) & BitMaskTable[num];
	} else {
		u32 delta = num - D->remain_bits;
		D->bit_buffer |= (D->bit_extension & BitMaskTable[D->remain_bits]) << delta;
		D->bit_extension = *D->input_ptr++;
		D->bit_extension |= *D->input_ptr++ << 8;
		D->remain_bits = 16 - delta;
		D->bit_buffer |= (D->bit_extension >> (16 - delta)) & BitMaskTable[delta];
	}
}

void koei_lz77_uncompress(void *dest, const void *src, u32 *psize)
{
	u8 *dest_ptr = dest;
	uncompress_t D = { .input_ptr = src, .remain_bits = 0 };

	read_bits(&D, 16);
	while (1) {
		D.control_byte = *D.input_ptr++;
		for (u32 i = 0; i < 8; ++i, D.control_byte <<= 1) {
			if (D.control_byte & 0x80) { // literal
				*dest_ptr++ = *D.input_ptr++;
				continue;
			}
			// match length
			u32 length;
			if (D.bit_buffer & 0x8000) {
				length = 1;
			} else if (D.bit_buffer & 0x4000) {
				length = (D.bit_buffer & 0x6000) >> 13;
				read_bits(&D, 2);
			} else if (D.bit_buffer & 0x2000) {
				length = (D.bit_buffer & 0x3800) >> 11;
				read_bits(&D, 4);
			} else if (D.bit_buffer & 0x1000) {
				length = (D.bit_buffer & 0x1E00) >> 9;
				read_bits(&D, 6);
			} else if (D.bit_buffer & 0x800) {
				length = (D.bit_buffer & 0xF80) >> 7;
				read_bits(&D, 8);
			} else if (D.bit_buffer & 0x400) {
				length = (D.bit_buffer & 0x7E0) >> 5;
				read_bits(&D, 10);
			} else if (D.bit_buffer & 0x200) {
				length = (D.bit_buffer & 0x3F8) >> 3;
				read_bits(&D, 12);
			} else {
				length = ((D.bit_buffer & 0x1FC) >> 2) + 0x80;
				if (length != 0xFF) {
					read_bits(&D, 13);
				} else {
					*psize = dest_ptr - (u8*)dest;
					return;
				}
			}
			// match distance
			u32 distance;
			D.bit_buffer &= ~0x8000;
			if (D.bit_buffer <= 0x7FF) {
				distance = (D.bit_buffer & 0x600) >> 9;
				read_bits(&D, 7);
			} else if (D.bit_buffer <= 0xBFF) {
				distance = ((D.bit_buffer & 0x300) >> 8) + 4;
				read_bits(&D, 8);
			} else if (D.bit_buffer <= 0x17FF) {
				distance = (((D.bit_buffer - 0xC00) & 0xF80) >> 7) + 8;
				read_bits(&D, 9);
			} else if (D.bit_buffer <= 0x2FFF) {
				distance = (((D.bit_buffer - 0x1800) & 0x1FC0) >> 6) + 0x20;
				read_bits(&D, 10);
			} else if (D.bit_buffer <= 0x3FFF) {
				distance = (D.bit_buffer & 0xFFF | 0x1000) >> 5;
				read_bits(&D, 11);
			} else if (D.bit_buffer <= 0x4FFF) {
				distance = (D.bit_buffer & 0xFFF | 0x1000) >> 4;
				read_bits(&D, 12);
			} else if (D.bit_buffer <= 0x5FFF) {
				distance = (D.bit_buffer & 0xFFF | 0x1000) >> 3;
				read_bits(&D, 13);
			} else if (D.bit_buffer <= 0x6FFF) {
				distance = (D.bit_buffer & 0xFFF | 0x1000) >> 2;
				read_bits(&D, 14);
			} else {
				distance = (D.bit_buffer & 0xFFF | 0x1000) >> 1;
				read_bits(&D, 15);
			}
			// copy
			u8 *window_ptr = dest_ptr - distance - 1;
			for (u32 i = 0; i <= length; ++i)
				*dest_ptr++ = *window_ptr++;
		}
	}
}

typedef struct compress_t {
    u8 *output_ptr;
	u8 *curr_ptr;
	u8 *next_ptr;
    u32 bit_buffer;
    u8 bit_count;
	u8 remain_bits;
} compress_t;

static void write_bits(compress_t *C, u32 bits, u32 num_bits)
{
    C->bit_buffer |= (bits & BitMaskTable[num_bits]) << (32 - C->bit_count - num_bits);
    C->bit_count += num_bits;
    if (C->bit_count >= 16) {
		C->curr_ptr[0] = C->bit_buffer >> 16 & 0xFF;
		C->curr_ptr[1] = C->bit_buffer >> 24;
        C->bit_buffer <<= 16;
        C->bit_count -= 16;
    }
	if (C->remain_bits < num_bits) {
		C->curr_ptr = C->next_ptr;
		C->next_ptr = C->output_ptr;
		C->output_ptr += 2;
	}
	C->remain_bits = (C->remain_bits - num_bits + 16) & 0xF;
}

static void flush_bits(compress_t *C)
{
    if (C->bit_count > 0) {
        C->curr_ptr[0] = C->bit_buffer >> 16 & 0xFF;
		C->curr_ptr[1] = C->bit_buffer >> 24;
    }
	C->output_ptr -= 2; // `next_ptr` read 2 bytes
}

static int find_match(const u8 *src, u32 src_pos, u32 max_len, u32 window_size, u32 *length, u32 *distance)
{
    u32 max_match = 0;
    u32 best_dist = 0;
    u32 start = (src_pos > window_size) ? src_pos - window_size : 0;
    for (u32 i = start; i < src_pos; ++i) { // not lazy
        u32 j = 0;
        while (j < max_len && src[i + j] == src[src_pos + j]) ++j;
        if (j >= max_match) {
            max_match = j;
            best_dist = src_pos - i;
        }
    }
	u32 num_bits;
	switch (max_match - 1) {
		case 1: num_bits = 1; break;
		case 2 ... 3: num_bits = 3; break;
		case 4 ... 7: num_bits = 5; break;
		case 8 ... 0xF: num_bits = 7; break;
		case 0x10 ... 0x1F: num_bits = 9; break;
		case 0x20 ... 0x3F: num_bits = 11; break;
		case 0x40 ... 0x7F: num_bits = 13; break;
		case 0x80 ... 0xFE: num_bits = 14; break;
		default: return 0;
	}
	switch (best_dist - 1) {
		case 0 ... 3: num_bits += 6; break;
		case 4 ... 7: num_bits += 7; break;
		case 8 ... 0x1F: num_bits += 8; break;
		case 0x20 ... 0x7F: num_bits += 9; break;
		case 0x80 ... 0xFF: num_bits += 10; break;
		case 0x100 ... 0x1FF: num_bits += 11; break;
		case 0x200 ... 0x3FF: num_bits += 12; break;
		case 0x400 ... 0x7FF: num_bits += 13; break;
		case 0x800 ... 0xFFF: num_bits += 14; break;
		default: return 0;
	}
    if (num_bits < max_match << 3) {
        *length = max_match;
        *distance = best_dist;
        return 1;
    }
    return 0;
}

static void write_length(compress_t *C, u32 length)
{
    if (length == 1) {
        write_bits(C, length, 1);
    } else if (length <= 3) {
        write_bits(C, length, 1 + 2);
    } else if (length <= 7) {
        write_bits(C, length, 2 + 3);
    } else if (length <= 0xF) {
        write_bits(C, length, 3 + 4);
    } else if (length <= 0x1F) {
        write_bits(C, length, 4 + 5);
    } else if (length <= 0x3F) {
        write_bits(C, length, 5 + 6);
    } else if (length <= 0x7F) {
        write_bits(C, length, 6 + 7);
    } else {
        write_bits(C, length & 0x7F, 7 + 7);
    }
}

static void write_distance(compress_t *C, u32 distance)
{
    if (distance <= 3) {
        write_bits(C, distance, 6);
    } else if (distance <= 7) {
        write_bits(C, 0x8 | (distance - 4), 7);
    } else if (distance <= 0x1F) {
        write_bits(C, 0x18 + (distance - 8), 8);
    } else if (distance <= 0x7F) {
        write_bits(C, 0x60 + (distance - 0x20), 9);
    } else if (distance <= 0xFF) {
        write_bits(C, 0x180 | (distance & 0x7F), 10);
    } else if (distance <= 0x1FF) {
        write_bits(C, 0x400 | (distance & 0xFF), 11);
    } else if (distance <= 0x3FF) {
        write_bits(C, 0xA00 | (distance & 0x1FF), 12);
    } else if (distance <= 0x7FF) {
        write_bits(C, 0x1800 | (distance & 0x3FF), 13);
    } else {
        write_bits(C, 0x3800 | (distance & 0x7FF), 14);
    }
}

u32 koei_lz77_compress(void *dest, const void *src, u32 src_size)
{
    compress_t C = { .output_ptr = dest + 2, .next_ptr = dest, .bit_buffer = 0, .bit_count = 0 };
    const u8 *src_ptr = src;

    u32 control_byte = 0;
    u32 control_bit = 0;
    u8 *control_addr = C.output_ptr++;

    while ((void*)src_ptr < src + src_size) {
        u32 length, distance;
		u32 src_pos = (void*)src_ptr - src;
		u32 max_len = src_size - src_pos > 0xFF ? 0xFF : src_size - src_pos;
        if (find_match(src, src_pos, max_len, 0xFFF, &length, &distance)) {
            // control bit 0
            control_byte <<= 1;
            write_length(&C, length - 1);
            write_distance(&C, distance - 1);
            src_ptr += length;
        } else { // literal
            // control bit 1
            control_byte = (control_byte << 1) | 1;
			*C.output_ptr++ = *src_ptr++;
        }
        // write control byte
        if (++control_bit == 8) {
            *control_addr = control_byte;
            control_addr = C.output_ptr++;
            control_bit = 0;
        }
    }
    // terminal
    write_bits(&C, 0x7F, 14);
    // remain control bits
	control_byte <<= 1;
	++control_bit;
	control_byte <<= (8 - control_bit);
	*control_addr = control_byte;

    flush_bits(&C);

    return C.output_ptr - (u8*)dest;
}

///////////////////////
// Indexed Character //
///////////////////////

u16 ichar2char(codeparam_t *cp, ichar_t ch)
{
	u16 code = cp->table[ch];
	return code >> 8 | (code & 0xFF) << 8;
}

ichar_t char2ichar(codeparam_t *cp, u16 ch)
{
	return cp->table[ch >> 8 | (ch & 0xFF) << 8];
}
