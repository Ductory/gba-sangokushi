#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "buffer.h"

#define BUF_INIT_SIZE 16

/* allocation */
#define alloc(obj,n)  malloc(sizeof(*(obj)) * (n))
#define allocz(obj,n) calloc((n), sizeof(*(obj)))
#define allocr(obj,n) realloc((obj), sizeof(*(obj)) * (n))

buf_t *new_buf(size_t n)
{
	size_t cap = BUF_INIT_SIZE;
	while (cap < n)
		cap <<= 1;
	buf_t *buf = alloc(buf, 1);
	buf->cap  = cap;
	buf->size = 0;
	buf->buf  = malloc(cap);
	return buf;
}

void del_buf(buf_t *buf)
{
	free(buf->buf);
	free(buf);
}

void buf_cls(buf_t *buf)
{
	buf->size = 0;
}

void buf_ccat(buf_t *buf, int ch)
{
	if (buf->size + 1 > buf->cap)
		buf->buf = realloc(buf->buf, buf->cap <<= 1);
	buf->buf[buf->size++] = ch;
}

void buf_wccat(buf_t *buf, int ch)
{
	if (buf->size + 2 > buf->cap)
		buf->buf = realloc(buf->buf, buf->cap <<= 1);
	*(wchar_t*)(buf->buf + buf->size) = ch;
	buf->size += 2;
}

void buf_mcat(buf_t *buf, const void *mem, size_t size)
{
	while (buf->size + size > buf->cap)
		buf->buf = allocr(buf->buf, buf->cap <<= 1);
	memcpy(buf->buf + buf->size, mem, size);
	buf->size += size;
}

/**
 * \0 will not be added
 */
void buf_cat(buf_t *buf, const char *s)
{
	size_t len = strlen(s);
	while (buf->size + len > buf->cap)
		buf->buf = allocr(buf->buf, buf->cap <<= 1);
	memcpy(buf->buf + buf->size, s, len);
	buf->size += len;
}

void buf_wcat(buf_t *buf, const wchar_t *s)
{
	size_t len = wcslen(s) * 2;
	while (buf->size + len > buf->cap)
		buf->buf = allocr(buf->buf, buf->cap <<= 1);
	memcpy(buf->buf + buf->size, s, len);
	buf->size += len;
}

#define BUFFER_MAX 2048

void buf_catf(buf_t *buf, const char *fmt, ...)
{
	char b[BUFFER_MAX];
	va_list ap;
	va_start(ap, fmt);
	vsprintf(b, fmt, ap);
	buf_cat(buf, b);
}

void buf_wcatf(buf_t *buf, const wchar_t *fmt, ...)
{
	wchar_t wb[BUFFER_MAX];
	va_list ap;
	va_start(ap, fmt);
	vswprintf(wb, BUFFER_MAX, fmt, ap);
	buf_wcat(buf, wb);
}
