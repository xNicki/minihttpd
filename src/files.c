#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include "files.h"

int files_open(const char *root, const char *uri,
               char *path, size_t path_size, int *fd, off_t *size)
{
    char root_real[PATH_MAX];
    char wanted[PATH_MAX];
    char real[PATH_MAX];
    struct stat st;
    int n;

    if (realpath(root, root_real) == NULL)
        return FILE_ERROR;

    if (uri[strlen(uri) - 1] == '/')
        n = snprintf(wanted, sizeof(wanted), "%s%sindex.html", root_real, uri);
    else
        n = snprintf(wanted, sizeof(wanted), "%s%s", root_real, uri);
    if (n < 0 || (size_t)n >= sizeof(wanted))
        return FILE_NOT_FOUND;

    if (realpath(wanted, real) == NULL) {
        if (errno == EACCES)
            return FILE_FORBIDDEN;
        if (errno == ENOENT || errno == ENOTDIR || errno == ENAMETOOLONG)
            return FILE_NOT_FOUND;
        return FILE_ERROR;
    }

    size_t len = strlen(root_real);
    if (strncmp(real, root_real, len) != 0 ||
        (real[len] != '/' && real[len] != '\0'))
        return FILE_FORBIDDEN;

    if (stat(real, &st) < 0)
        return FILE_ERROR;
    if (!S_ISREG(st.st_mode))
        return FILE_FORBIDDEN;

    *fd = open(real, O_RDONLY);
    if (*fd < 0)
        return (errno == EACCES) ? FILE_FORBIDDEN : FILE_ERROR;

    *size = st.st_size;
    snprintf(path, path_size, "%s", real);
    return FILE_OK;
}
