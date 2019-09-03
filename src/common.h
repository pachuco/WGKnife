#ifndef COMMON_H
#define COMMON_H

#include "common.h"

void* loadfile(char* name, unsigned int* buflen);
int writefile(char* name, void* buf, unsigned int buflen);
int isFileExist(char* name);
int makeDir(const char *path);

#endif