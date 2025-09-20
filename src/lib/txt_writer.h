#ifdef __cplusplus
extern "C" {
#endif

#ifndef FILE_WRITER_H
#define FILE_WRITER_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>

extern char* logFile;
extern char* logDir;

// Write a single line to a given file
// Returns false on fail
bool writeLineToFile(const char* filename, const char* line);

// Creates a file, 'nuff said
bool createFile(const char* filepath);

int emptyFile(const char *filename);

bool fileLog(const char* format, ...);

#endif // FILE_READER_H

#ifdef __cplusplus
}
#endif
