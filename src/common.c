#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

#include "common.h"

void* loadfile(char* name, unsigned int* buflen) {
    char* buf;
	FILE* fin = fopen(name, "rb");
    
	if (!fin) {
        printf("loadfile(): Could not open the file.\n");
		return NULL;
	}

    fseek(fin, 0, SEEK_END);
    *buflen = ftell(fin);
    fseek(fin, 0, SEEK_SET);

    buf = malloc(*buflen);
    if(!buf) {
        fclose(fin);
        printf("loadfile(): Out of memory!\n");
        return NULL;
    }

    fread(buf, 1, *buflen, fin);
	fclose(fin);

    return buf;
}

int writefile(char* name, void* buf, unsigned int buflen) {
	FILE* fout = fopen(name, "wb");
    
	if (!fout) {
        printf("writefile(): Could not open the file.\n");
		return 0;
	}
    fwrite(buf, 1, buflen, fout);
    fclose(fout);
    return 1;
}

int isFileExist(char* name) {
    FILE* f = fopen(name, "r");
    
    if (f) {
        fclose(f);
        return 1;
    } else {
        return 0;
    }
}


//expected function behaviours:
//* int clearPathIfOccupied(const char* path)
//path parameter is an absolute path
//no file/folder exists: return 1
//file and folder is present: attempt delete either
//if folder: delete recursively
//failure to delete: return 0
//
//* int isPathRelative(const char* path)
//self-explanatory
//
//* int retrieveWorkingPath(unsigned int nBufferLength, char* lpBuffer)
//lpBuffer is destination buffer
//nBufferLength is max buffer length(including null)
//if fail: return 0
//otherwise: return number of characters written to buff, without null

// Fully qualified name of the directory being deleted, without trailing backslash
#ifdef _WIN32
#include <windows.h>

/*
int clearPathIfOccupied(const char* path) {
    DWORD attr;
    
    attr = GetFileAttributesA(path);
    if (attr != INVALID_FILE_ATTRIBUTES) {
        SHFILEOPSTRUCTA file_op = {
            NULL,
            FO_DELETE,
            path,
            "",
            FOF_NOCONFIRMATION |
            FOF_NOERRORUI |
            FOF_SILENT,
            FALSE,
            0,
            ""
        };
        return !SHFileOperationA(&file_op);
    }
    return 1;
}

int isPathRelative(const char* path) {
    return PathIsRelativeA(path);
}

int retrieveWorkingPath(unsigned int nBufferLength, char* lpBuffer) {
    return GetCurrentDirectoryA(nBufferLength, lpBuffer);
}
*/

int makeDir(const char *path) {
    return mkdir(path);
}
#else
/*
int clearPathIfOccupied(const char* path) {
    //TODO
    return 0;
}

int isPathRelative(const char* path) {
    return *path != '/';
}

int retrieveWorkingPath() {
    //TODO
    return 0;
}
*/

int makeDir(const char *path) {
    return mkdir(path, 0777);
}
#endif
