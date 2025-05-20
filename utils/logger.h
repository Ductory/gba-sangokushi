#ifndef _LOGGER_H
#define _LOGGER_H

#include <stdio.h>

enum LOG_ENUM {
	LOG_WARNING,
	LOG_ERROR,
	LOG_DEBUG,
	LOG_NORMAL
};

void log_print(FILE *fp, int type, const char *fmt, ...);

#define LOG_W(fmt,...) log_print(stderr, LOG_WARNING, "<%s>: " fmt, __func__, ## __VA_ARGS__)
#define LOG_E(fmt,...) log_print(stderr, LOG_ERROR, "<%s>: " fmt, __func__, ## __VA_ARGS__)
#define LOG_D(fmt,...) log_print(stderr, LOG_DEBUG, "<%s>: " fmt, __func__, ## __VA_ARGS__)
#define LOG_N(fmt,...) log_print(stderr, LOG_NORMAL, fmt, ## __VA_ARGS__)

#ifdef NDEBUG
#define log_print(...) ((void)0)
#endif // NDEBUG

#endif // _LOGGER_H
