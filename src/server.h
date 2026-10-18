#ifndef SERVER_H
#define SERVER_H

#include <netinet/in.h>
#include "base.h"
#include "html_renderer.h"

#define MAX_REQUEST_LEN 1024
#define MAX_HTTP_METHOD_LEN 7
#define MAX_RESOURCE_LEN 512
#define IPV4_STR_LEN 16

typedef struct {
    i32 fd;
    unsigned short port;
} Server;

typedef enum {
    GET,
    POST,
    UNKNOWN,
} Http_Method;

typedef struct {
    i32 sender_fd;
    Http_Method method;
    char resource[MAX_RESOURCE_LEN];
} Request;

Http_Method cstr_to_http_method(char *str);
i32 addr_to_cstr(
    u16 buffer_size;
    struct sockaddr_in *addr,
    char buffer[restrict buffer_size],
    u16 buffer_size
);
i32 server_init(Server *server, u16 port);
i32 server_recv_request(Server *server, Request *req);
i32 send_html(
    i32 fd,
    const char *path,
    u16 code,
    const char *reason,
    Html_Template_Args *args
);

#endif // SERVER_H
