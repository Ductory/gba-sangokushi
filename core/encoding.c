#include "encoding.h"
#include "utils/logger.h"
#include <stdio.h>
#include <stdlib.h>

#include <windows.h>

/* we have extended characters, so mbstowcs/wcstombs is not enough */


/////////////
// general //
/////////////

static codetab_t new_table()
{
	u32 len = CODE_HI_NUM * CODE_LO_NUM * sizeof(u16);
	void *tab = malloc(len);
	u32 *p = tab;
	for (u32 i = 0; i < len; i += 4)
		*p++ = 0x3f003f; // init to '?'
	return tab;
}

static codemap_t new_map()
{
	u32 len = CODE_HI_NUM * CODE_LO_NUM * sizeof(bool);
	void *map = malloc(len);
	memset(map, 0, len);
	return map;
}

/**
 * CONVENTION:
 * the file MUST encode in utf8
 * one character per line
 * format: code (SP) character
 * utf8 -> utf16
 */
codetab_t build_code_table(const char *name)
{
	codetab_t tab = new_table();
	u32 code;
	u16 ch = 0;
	char buf[8];
	FILE *fp = fopen(name, "r");
	while (fscanf(fp, "%X %s", &code, buf) == 2) {
		utf16_utf8_c(&ch, buf, sizeof(buf));
		tab[(code >> 8) - CODE_HI_MIN][(code & 0xff) - CODE_LO_MIN] = ch;
	}
	fclose(fp);
	return tab;
}

/**
 * just for GBK and SJIS
 * CONVENTION:
 * one character per line
 * format:
 * 1. character
 * 2. ! XXXX     ; special code
 * 3. #          ; placeholder
 * locale -> map (1 if present, 0 if not)
 */
codemap_t build_code_map(const char *name)
{
	codemap_t map = new_map();
	FILE *fp = fopen(name, "rb");
	int c;
	u16 wc;
	for (; ; fgetc(fp) == '\r' ? fgetc(fp) : 0) {
		c = fgetc(fp);
		if (c & 0x80) // very weak, but is enough
			c = c << 8 | fgetc(fp);
		else if (c == '!') // special code
			fscanf(fp, "%X", &c);
		else if (c == '#') // placeholder, ignore
			continue;
		else // otherwise, just stop handling
			break;
		map[(c >> 8) - CODE_HI_MIN][(c & 0xFF) - CODE_LO_MIN] = 1;
	}
	fclose(fp);
	return map;
}


///////////////
// Shift_JIS //
///////////////

/**
 * build an extended Shift_JIS table
 * because in KMD/EKD/SAN, some characters are not in the standard table
 */
codetab_t build_sjis_table(const char *name)
{
	return build_code_table(name);
}

codemap_t build_sjis_map(const char *name)
{
	return build_code_map(name);
}

int is_sjis_double_byte(u32 c)
{
	return c - SJIS_HI_MIN <= (SJIS_KATAKANA_MIN - 1) - SJIS_HI_MIN
		|| c - (SJIS_KATAKANA_MAX + 1) <= SJIS_HI_MAX - (SJIS_KATAKANA_MAX + 1);
}

void sjis2utf16(codetab_t tab, u16 *buf, u8 *s)
{
	u16 *p = buf;
	while (*s) {
		if (is_sjis_double_byte(*s)) {
			*p++ = tab[s[0] - SJIS_HI_MIN][s[1] - SJIS_LO_MIN];
			s += 2;
			continue;
		}
		if (*s >= SJIS_KATAKANA_MIN && *s <= SJIS_KATAKANA_MAX) {
			utf16_sjis_c(p++, _(s++), 1);
			continue;
		}
		*p++ = *s++;
	}
	*p = '\0';
}


/////////
// GBK //
/////////

codetab_t build_gbk_table(const char *name)
{
	return build_code_table(name);
}

codemap_t build_gbk_map(const char *name)
{
	return build_code_map(name);
}

int is_gbk_double_byte(u32 c)
{
	return c - GBK_HI_MIN <= GBK_HI_MAX - GBK_HI_MIN;
}


//////////////
// standard //
//////////////

int mbs2wcs(u16 *wcs, const char *mbs, int cb, size_t max, unsigned cp)
{
	return MultiByteToWideChar(cp, 0, mbs, cb, wcs, max);
}

int wcs2mbs(char *mbs, const u16 *wcs, int cch, size_t max, unsigned cp)
{
	return WideCharToMultiByte(cp, 0, wcs, cch, mbs, max, NULL, NULL);
}

int mbs2mbs(char *dst, const char *src, int cb, size_t max, unsigned cp_dst, unsigned cp_src)
{
	size_t len = cb >= 0 ? cb : strlen(src) + 1;
	u16 *buf = malloc(len * sizeof(u16));
	int cch = MultiByteToWideChar(cp_src, 0, src, cb, buf, len);
	cch = cb >= 0 ? cch : -1;
	return WideCharToMultiByte(cp_dst, 0, buf, cch, dst, max, NULL, NULL);
}
