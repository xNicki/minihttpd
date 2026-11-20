#include <stdio.h>
#include <string.h>
#include <strings.h>
#include "http.h"

const char *http_status_text(int code)
{
    switch (code) {
    case 200: return "OK";
    case 400: return "Bad Request";
    case 403: return "Forbidden";
    case 404: return "Not Found";
    case 405: return "Method Not Allowed";
    default:  return "Internal Server Error";
    }
}

/* Copia len bytes en dst sin desbordarlo. Devuelve -1 si no caben. */
static int copy_bounded(char *dst, size_t size, const char *src, size_t len)
{
    if (len >= size)
        return -1;
    memcpy(dst, src, len);
    dst[len] = '\0';
    return 0;
}

/* Analiza "GET /ruta HTTP/1.1" */
static int parse_request_line(const char *line, size_t len, HttpRequest *req)
{
    const char *sp1 = memchr(line, ' ', len);
    if (sp1 == NULL)
        return PARSE_BAD;

    const char *uri = sp1 + 1;
    const char *sp2 = memchr(uri, ' ', len - (size_t)(uri - line));
    if (sp2 == NULL)
        return PARSE_BAD;

    const char *version = sp2 + 1;
    size_t version_len = len - (size_t)(version - line);
    size_t uri_len = (size_t)(sp2 - uri);

    /* Version: HTTP/1.0 o HTTP/1.1 */
    if (version_len != 8 || memcmp(version, "HTTP/1.", 7) != 0 ||
        (version[7] != '0' && version[7] != '1'))
        return PARSE_BAD;

    /* URI: no vacia, empieza con '/' y no es demasiado larga */
    if (uri_len == 0 || uri_len >= MAX_URI || uri[0] != '/')
        return PARSE_BAD;

    /* Se ignora lo que venga despues de '?' o '#' */
    size_t path_len = 0;
    while (path_len < uri_len && uri[path_len] != '?' && uri[path_len] != '#')
        path_len++;
    if (copy_bounded(req->uri, sizeof(req->uri), uri, path_len) < 0)
        return PARSE_BAD;

    /* Solo se permite GET */
    if (sp1 - line != 3 || memcmp(line, "GET", 3) != 0)
        return PARSE_METHOD;

    req->keep_alive = (version[7] == '1');   /* 1.1 = persistente */
    return PARSE_OK;
}

/* Analiza un encabezado "Nombre: valor" */
static int parse_header(const char *line, size_t len, HttpRequest *req)
{
    const char *colon = memchr(line, ':', len);
    if (colon == NULL || colon == line)
        return PARSE_BAD;

    size_t name_len = (size_t)(colon - line);
    const char *value = colon + 1;
    size_t value_len = len - name_len - 1;

    /* Quitar espacios al inicio y al final del valor */
    while (value_len > 0 && (*value == ' ' || *value == '\t')) {
        value++;
        value_len--;
    }
    while (value_len > 0 && (value[value_len - 1] == ' ' ||
                             value[value_len - 1] == '\t'))
        value_len--;

    if (name_len == 4 && strncasecmp(line, "Host", 4) == 0) {
        if (copy_bounded(req->host, sizeof(req->host), value, value_len) < 0)
            return PARSE_BAD;
    } else if (name_len == 10 && strncasecmp(line, "User-Agent", 10) == 0) {
        if (copy_bounded(req->user_agent, sizeof(req->user_agent),
                         value, value_len) < 0)
            return PARSE_BAD;
    } else if (name_len == 10 && strncasecmp(line, "Connection", 10) == 0) {
        if (value_len == 5 && strncasecmp(value, "close", 5) == 0)
            req->keep_alive = 0;
        else if (value_len == 10 && strncasecmp(value, "keep-alive", 10) == 0)
            req->keep_alive = 1;
    }
    return PARSE_OK;
}

int http_parse(const char *buf, size_t len, HttpRequest *req, size_t *used)
{
    if (memchr(buf, '\0', len) != NULL)
        return PARSE_BAD;

    /* La solicitud termina con una linea en blanco */
    const char *end = strstr(buf, "\r\n\r\n");
    if (end == NULL)
        return (len >= MAX_REQUEST) ? PARSE_BAD : PARSE_INCOMPLETE;

    *used = (size_t)(end - buf) + 4;
    memset(req, 0, sizeof(*req));

    /* Se recorre linea por linea: primero la solicitud, luego los encabezados */
    const char *block_end = end + 2;
    const char *line = buf;
    int first = 1;

    while (line < block_end) {
        const char *eol = strstr(line, "\r\n");
        size_t line_len = (size_t)(eol - line);
        if (line_len > MAX_LINE)
            return PARSE_BAD;

        int r = first ? parse_request_line(line, line_len, req)
                      : parse_header(line, line_len, req);
        if (r != PARSE_OK)
            return r;

        first = 0;
        line = eol + 2;
    }

    if (req->host[0] == '\0')      /* Host es obligatorio */
        return PARSE_BAD;
    return PARSE_OK;
}

int http_build_header(char *out, size_t size, int status,
                      const char *mime, long length, int keep_alive)
{
    return snprintf(out, size,
                    "HTTP/1.1 %d %s\r\n"
                    "Server: MiniHTTPd\r\n"
                    "Content-Type: %s\r\n"
                    "Content-Length: %ld\r\n"
                    "Connection: %s\r\n"
                    "%s"
                    "\r\n",
                    status, http_status_text(status), mime, length,
                    keep_alive ? "keep-alive" : "close",
                    status == 405 ? "Allow: GET\r\n" : "");
}
