#include <string.h>
#include <strings.h>
#include "mime.h"

typedef struct {
    const char *ext;
    const char *type;
} Mime;

static const Mime table[] = {
    {".html", "text/html"},
    {".css",  "text/css"},
    {".js",   "application/javascript"},
    {".png",  "image/png"},
    {".jpg",  "image/jpeg"},
};

const char *mime_type(const char *path)
{
    const char *dot = strrchr(path, '.');

    if (dot != NULL) {
        for (size_t i = 0; i < sizeof(table) / sizeof(table[0]); i++) {
            if (strcasecmp(dot, table[i].ext) == 0)
                return table[i].type;
        }
    }
    return "application/octet-stream";
}
