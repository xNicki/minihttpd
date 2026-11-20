#ifndef HTTP_H
#define HTTP_H

#include <stddef.h>

#define MAX_REQUEST 8192   
#define MAX_LINE    2048  
#define MAX_URI     1024   

#define PARSE_INCOMPLETE 0    
#define PARSE_OK         1   
#define PARSE_BAD        400 
#define PARSE_METHOD     405 

typedef struct {
    char uri[MAX_URI];
    char host[256];
    char user_agent[512];
    int  keep_alive;      
} HttpRequest;

int http_parse(const char *buf, size_t len, HttpRequest *req, size_t *used);
const char *http_status_text(int code);
int http_build_header(char *out, size_t size, int status,
                      const char *mime, long length, int keep_alive);
#endif
