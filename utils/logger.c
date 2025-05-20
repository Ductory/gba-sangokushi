#include "logger.h"
#include <stdarg.h>

void log_print(FILE *fp, int type, const char *fmt, ...)
{
	if (!fp)
		return;
	if (type == LOG_DEBUG)
		fputs("debug: ", fp);
	else if (type == LOG_NORMAL)
		fputs("log: ", fp);
	else
		fprintf(stderr, "\e[31m%s\e[0m: ", type == LOG_ERROR ? "error" : "warning");
	va_list ap;
	va_start(ap, fmt);
	vfprintf(fp, fmt, ap);
	va_end(ap);
	fputc('\n', fp);
}
