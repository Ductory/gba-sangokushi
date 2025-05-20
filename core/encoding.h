#ifndef _ENCODING_H
#define _ENCODING_H

#include "core/gba.h"


/* general */

#define CODE_HI_MAX 0xFEu
#define CODE_HI_MIN 0x81u
#define CODE_LO_MAX 0xFFu
#define CODE_LO_MIN 0x40u
#define CODE_HI_NUM (CODE_HI_MAX - CODE_HI_MIN + 1)
#define CODE_LO_NUM (CODE_LO_MAX - CODE_LO_MIN + 1)

typedef u16 (*codetab_t)[CODE_LO_NUM];
typedef bool (*codemap_t)[CODE_LO_NUM];

codetab_t build_code_table(const char *name);
codemap_t build_code_map(const char *name);


/* Shift_JIS (.932) */

#define SJIS_HI_MAX 0xFCu
#define SJIS_HI_MIN 0x81u
#define SJIS_LO_MAX 0xFFu
#define SJIS_LO_MIN 0x40u
#define SJIS_HI_NUM (SJIS_HI_MAX - SJIS_HI_MIN + 1)
#define SJIS_LO_NUM (SJIS_LO_MAX - SJIS_LO_MIN + 1)

#define SJIS_KATAKANA_MIN 0xA0u
#define SJIS_KATAKANA_MAX 0xDFu

codetab_t build_sjis_table(const char *name);
codemap_t build_sjis_map(const char *name);
int is_sjis_double_byte(u32 c);
void sjis2utf16(codetab_t tab, u16 *buf, u8 *s);


/* GBK (.936) */

#define GBK_HI_MAX 0xFEu
#define GBK_HI_MIN 0x81u
#define GBK_LO_MAX 0xFEu
#define GBK_LO_MIN 0x40u
#define GBK_HI_NUM (GBK_HI_MAX - GBK_HI_MIN + 1)
#define GBK_LO_NUM (GBK_LO_MAX - GBK_LO_MIN + 1)

codetab_t build_gbk_table(const char *name);
codemap_t build_gbk_map(const char *name);
int is_gbk_double_byte(u32 c);
void gbk2utf16(codetab_t tab, u16 *buf, u8 *s);


/* standard */

#define LC_UTF8 ".UTF8"
#define LC_SJIS ".932"
#define LC_GBK ".936"

#define CP_ACP 0
#define CP_UTF8 65001
#define CP_SJIS 932
#define CP_GBK 936

int mbs2wcs(u16 *wcs, const char *mbs, int cb, size_t max, unsigned cp);
int wcs2mbs(char *mbs, const u16 *wcs, int cch, size_t max, unsigned cp);
int mbs2mbs(char *dst, const char *src, int cb, size_t max, unsigned cp_dst, unsigned cp_src);

#define utf16_utf8_c(wcs,mbs,cb) mbs2wcs(wcs, mbs, cb, 1, CP_UTF8)
#define utf16_sjis_c(wcs,mbs,cb) mbs2wcs(wcs, mbs, cb, 1, CP_SJIS)
#define utf16_gbk_c(wcs,mbs,cb) mbs2wcs(wcs, mbs, cb, 1, CP_GBK)
#define utf16_ansi_c(wcs,mbs,cb) mbs2wcs(wcs, mbs, cb, 1, CP_ACP)

#define utf8_utf16_c(mbs,wcs,max) wcs2mbs(mbs, wcs, 1, max, CP_UTF8)
#define sjis_utf16_c(mbs,wcs,max) wcs2mbs(mbs, wcs, 1, max, CP_SJIS)
#define gbk_utf16_c(mbs,wcs,max) wcs2mbs(mbs, wcs, 1, max, CP_GBK)
#define ansi_utf16_c(mbs,wcs,max) wcs2mbs(mbs, wcs, 1, max, CP_ACP)

#define sjis_utf8_c(dst,src,cb,max) mbs2mbs(dst, src, cb, max, CP_SJIS, CP_UTF8)
#define gbk_utf8_c(dst,src,cb,max) mbs2mbs(dst, src, cb, max, CP_GBK, CP_UTF8)
#define ansi_utf8_c(dst,src,cb,max) mbs2mbs(dst, src, cb, max, CP_ACP, CP_UTF8)
#define utf8_sjis_c(dst,src,cb,max) mbs2mbs(dst, src, cb, max, CP_UTF8, CP_SJIS)
#define gbk_sjis_c(dst,src,cb,max) mbs2mbs(dst, src, cb, max, CP_GBK, CP_SJIS)
#define ansi_sjis_c(dst,src,cb,max) mbs2mbs(dst, src, cb, max, CP_ACP, CP_SJIS)
#define utf8_gbk_c(dst,src,cb,max) mbs2mbs(dst, src, cb, max, CP_UTF8, CP_GBK)
#define sjis_gbk_c(dst,src,cb,max) mbs2mbs(dst, src, cb, max, CP_SJIS, CP_GBK)
#define ansi_gbk_c(dst,src,cb,max) mbs2mbs(dst, src, cb, max, CP_ACP, CP_GBK)


#define utf16_utf8_s(wcs,mbs,max) mbs2wcs(wcs, mbs, -1, max, CP_UTF8)
#define utf16_sjis_s(wcs,mbs,max) mbs2wcs(wcs, mbs, -1, max, CP_SJIS)
#define utf16_gbk_s(wcs,mbs,max) mbs2wcs(wcs, mbs, -1, max, CP_GBK)
#define utf16_ansi_s(wcs,mbs,max) mbs2wcs(wcs, mbs, -1, max, CP_ACP)

#define utf8_utf16_s(mbs,wcs,max) wcs2mbs(mbs, wcs, -1, max, CP_UTF8)
#define sjis_utf16_s(mbs,wcs,max) wcs2mbs(mbs, wcs, -1, max, CP_SJIS)
#define gbk_utf16_s(mbs,wcs,max) wcs2mbs(mbs, wcs, -1, max, CP_GBK)
#define ansi_utf16_s(mbs,wcs,max) wcs2mbs(mbs, wcs, -1, max, CP_ACP)

#define sjis_utf8_s(dst,src,max) mbs2mbs(dst, src, -1, max, CP_SJIS, CP_UTF8)
#define gbk_utf8_s(dst,src,max) mbs2mbs(dst, src, -1, max, CP_GBK, CP_UTF8)
#define ansi_utf8_s(dst,src,max) mbs2mbs(dst, src, -1, max, CP_ACP, CP_UTF8)
#define utf8_sjis_s(dst,src,max) mbs2mbs(dst, src, -1, max, CP_UTF8, CP_SJIS)
#define gbk_sjis_s(dst,src,max) mbs2mbs(dst, src, -1, max, CP_GBK, CP_SJIS)
#define ansi_sjis_s(dst,src,max) mbs2mbs(dst, src, -1, max, CP_ACP, CP_SJIS)
#define utf8_gbk_s(dst,src,max) mbs2mbs(dst, src, -1, max, CP_UTF8, CP_GBK)
#define sjis_gbk_s(dst,src,max) mbs2mbs(dst, src, -1, max, CP_SJIS, CP_GBK)
#define ansi_gbk_s(dst,src,max) mbs2mbs(dst, src, -1, max, CP_ACP, CP_GBK)

#endif // _ENCODING_H
