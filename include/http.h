#ifndef HTTP_H
#define HTTP_H

#include <stddef.h>

#define MAX_REQUEST 8192   /* tamano maximo de toda la solicitud */
#define MAX_LINE    2048   /* tamano maximo de una linea */
#define MAX_URI     1024   /* tamano maximo de la URI */

/* Resultados de http_parse */
#define PARSE_INCOMPLETE 0    /* faltan datos, seguir leyendo */
#define PARSE_OK         1    /* solicitud correcta */
#define PARSE_BAD        400  /* Bad Request */
#define PARSE_METHOD     405  /* Method Not Allowed */

typedef struct {
    char uri[MAX_URI];
    char host[256];
    char user_agent[512];
    int  keep_alive;      /* 1 = mantener la conexion abierta */
} HttpRequest;

/* Analiza la solicitud. "buf" debe terminar en '\0'.
 * En *used deja cuantos bytes ocupa la solicitud. */
int http_parse(const char *buf, size_t len, HttpRequest *req, size_t *used);

/* Texto del codigo de estado ("OK", "Not Found", ...). */
const char *http_status_text(int code);

/* Escribe la cabecera de la respuesta en "out" y devuelve su largo. */
int http_build_header(char *out, size_t size, int status,
                      const char *mime, long length, int keep_alive);

#endif
