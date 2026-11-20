#define _GNU_SOURCE
#include <errno.h>
#include <limits.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/sendfile.h>
#include <sys/socket.h>
#include <unistd.h>

#include "server.h"
#include "http.h"
#include "files.h"
#include "mime.h"

#define MAX_EVENTS 64
#define OUT_SIZE   1024

typedef struct {
    int    fd;
    char   in[MAX_REQUEST + 1];
    size_t in_len;
    char   out[OUT_SIZE];        
    size_t out_len;
    size_t out_sent;
    int    file_fd;         
    off_t  file_offset;
    off_t  file_size;
    int    keep_alive;
} Conn;

static const char *root_dir;    

static void watch(int epfd, Conn *c, uint32_t events)
{
    struct epoll_event ev = { .events = events, .data.ptr = c };
    epoll_ctl(epfd, EPOLL_CTL_MOD, c->fd, &ev);
}

static void close_conn(int epfd, Conn *c)
{
    epoll_ctl(epfd, EPOLL_CTL_DEL, c->fd, NULL);
    if (c->file_fd >= 0)
        close(c->file_fd);
    close(c->fd);
    free(c);
}

static void send_error(Conn *c, int status)
{
    char body[128];
    int body_len = snprintf(body, sizeof(body), "<h1>%d %s</h1>\n",
                            status, http_status_text(status));
    int head_len = http_build_header(c->out, OUT_SIZE, status, "text/html",
                                     body_len, c->keep_alive);
    memcpy(c->out + head_len, body, (size_t)body_len);
    c->out_len = (size_t)(head_len + body_len);
}

static void serve_file(Conn *c, const HttpRequest *req)
{
    char path[PATH_MAX];
    int fd;
    off_t size;

    int r = files_open(root_dir, req->uri, path, sizeof(path), &fd, &size);
    if (r == FILE_NOT_FOUND) {
        send_error(c, 404);
    } else if (r == FILE_FORBIDDEN) {
        send_error(c, 403);
    } else if (r != FILE_OK) {
        send_error(c, 500);
    } else {
        c->out_len = (size_t)http_build_header(c->out, OUT_SIZE, 200,
                                               mime_type(path), (long)size,
                                               c->keep_alive);
        c->file_fd = fd;
        c->file_size = size;
        c->file_offset = 0;
    }
}

static void process_request(int epfd, Conn *c)
{
    HttpRequest req;
    size_t used = 0;

    c->in[c->in_len] = '\0';
    int r = http_parse(c->in, c->in_len, &req, &used);
    if (r == PARSE_INCOMPLETE)
        return;

    if (r == PARSE_OK) {
        c->keep_alive = req.keep_alive;
        serve_file(c, &req);
        memmove(c->in, c->in + used, c->in_len - used);
        c->in_len -= used;
    } else {
        c->keep_alive = 0;          
        send_error(c, r);
    }

    c->out_sent = 0;
    watch(epfd, c, EPOLLOUT);       
}

static void on_readable(int epfd, Conn *c)
{
    ssize_t n = recv(c->fd, c->in + c->in_len, MAX_REQUEST - c->in_len, 0);

    if (n == 0) {                
        close_conn(epfd, c);
        return;
    }
    if (n < 0) {
        if (errno != EAGAIN && errno != EWOULDBLOCK)
            close_conn(epfd, c);
        return;
    }
    c->in_len += (size_t)n;
    process_request(epfd, c);
}

static void on_writable(int epfd, Conn *c)
{
    while (c->out_sent < c->out_len) {
        ssize_t n = send(c->fd, c->out + c->out_sent,
                         c->out_len - c->out_sent, MSG_NOSIGNAL);
        if (n < 0) {
            if (errno != EAGAIN && errno != EWOULDBLOCK)
                close_conn(epfd, c);
            return;
        }
        c->out_sent += (size_t)n;
    }

    while (c->file_fd >= 0 && c->file_offset < c->file_size) {
        ssize_t n = sendfile(c->fd, c->file_fd, &c->file_offset,
                             (size_t)(c->file_size - c->file_offset));
        if (n <= 0) {
            if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK))
                return;
            close_conn(epfd, c);
            return;
        }
    }

    if (c->file_fd >= 0) {
        close(c->file_fd);
        c->file_fd = -1;
    }
    if (!c->keep_alive) {
        close_conn(epfd, c);
        return;
    }

    c->out_len = 0;
    c->out_sent = 0;
    watch(epfd, c, EPOLLIN);
    if (c->in_len > 0)
        process_request(epfd, c);
}

static void accept_clients(int epfd, int listen_fd)
{
    for (;;) {
        int fd = accept4(listen_fd, NULL, NULL, SOCK_NONBLOCK);
        if (fd < 0)
            return;               

        Conn *c = calloc(1, sizeof(*c));
        if (c == NULL) {
            close(fd);
            continue;
        }
        c->fd = fd;
        c->file_fd = -1;

        struct epoll_event ev = { .events = EPOLLIN, .data.ptr = c };
        if (epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev) < 0) {
            close(fd);
            free(c);
        }
    }
}

static int create_listener(int port)
{
    int fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
    if (fd < 0)
        return -1;

    int on = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons((unsigned short)port);

    if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0 ||
        listen(fd, 128) < 0) {
        close(fd);
        return -1;
    }
    return fd;
}

//Bucle Principal
int server_run(int port, const char *root)
{
    root_dir = root;
    signal(SIGPIPE, SIG_IGN);

    int listen_fd = create_listener(port);
    if (listen_fd < 0) {
        perror("no se pudo abrir el puerto");
        return 1;
    }

    int epfd = epoll_create1(0);
    if (epfd < 0) {
        perror("epoll_create1");
        return 1;
    }

    struct epoll_event ev = { .events = EPOLLIN, .data.ptr = NULL };
    epoll_ctl(epfd, EPOLL_CTL_ADD, listen_fd, &ev);

    printf("MiniHTTPd escuchando en http://localhost:%d (carpeta: %s)\n",
           port, root);

    struct epoll_event events[MAX_EVENTS];
    for (;;) {
        int n = epoll_wait(epfd, events, MAX_EVENTS, -1);
        for (int i = 0; i < n; i++) {
            Conn *c = events[i].data.ptr;

            if (c == NULL)
                accept_clients(epfd, listen_fd);
            else if (events[i].events & (EPOLLERR | EPOLLHUP))
                close_conn(epfd, c);
            else if (events[i].events & EPOLLIN)
                on_readable(epfd, c);
            else if (events[i].events & EPOLLOUT)
                on_writable(epfd, c);
        }
    }
}
