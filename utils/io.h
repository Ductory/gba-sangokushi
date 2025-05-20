#ifndef _IO_H
#define _IO_H

#include "core/gba.h"

bool readfile(const char *name, u8 **buf, u32 *size);
void writefile(const char *name, u8 *buf, u32 size);

#endif // _IO_H
