#ifndef __Z_DEF_H__
#define __Z_DEF_H__

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <zlib.h>

int def(FILE *source, FILE *dest, int level);
int inf(FILE *source, FILE *dest);
void zerr(int ret);

#endif //__Z_DEF_H__
