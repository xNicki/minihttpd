#ifndef FILES_H
#define FILES_H

#include <stddef.h>
#include <sys/types.h>

#define FILE_OK        0
#define FILE_NOT_FOUND 1
#define FILE_FORBIDDEN 2
#define FILE_ERROR     3

int files_open(const char *root, const char *uri,
               char *path, size_t path_size, int *fd, off_t *size);

#endif
