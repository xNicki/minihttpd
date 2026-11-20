#include <stdio.h>
#include <stdlib.h>
#include "server.h"

int main(int argc, char *argv[])
{
    int port = 8080;
    const char *root = "www";

    if (argc > 1)
        port = atoi(argv[1]);
    if (argc > 2)
        root = argv[2];

    if (port <= 0 || port > 65535) {
        fprintf(stderr, "Uso: %s [puerto] [carpeta]\n", argv[0]);
        return 1;
    }
    return server_run(port, root);
}
