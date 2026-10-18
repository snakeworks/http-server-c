#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "server.h"

Http_Method cstr_to_http_method(char *str) {
    if (strcmp(str, "GET") == 0) return GET;
    else if (strcmp(str, "POST") == 0) return POST;
    else UNKNOWN;
}

// NOTE: I know there is the inet_ntop function, but I thought it'd be fun to write my own.
// It will probably only make sense on little endian machines though, so don't try running
// this on your IBM System/360. Also it won't work on IPv6 addresses, but who cares?
i32 addr_to_cstr(
    u16 buffer_size;
    struct sockaddr_in *addr,
    char buffer[restrict buffer_size],
    u16 buffer_size
) {
    if (buffer_size < IPV4_STR_LEN) {
        return -1;
    }

    u32 addr_uint = addr->sin_addr.s_addr; // addr_uint is big endian!
    sprintf(
        buffer,
        "%d.%d.%d.%d",
        (addr_uint) & 0xff,
        (addr_uint >> 8) & 0xff,
        (addr_uint >> 16) & 0xff,
        (addr_uint >> 24) & 0xff
    );

    return 0;
}

i32 server_init(Server *server, u16 port) {
    server->fd = socket(AF_INET, SOCK_STREAM, 0);
    server->port = htons(port);

    if (server->fd < 0) {
        perror("socket");
        return server->fd;
    }

    struct sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_port = server->port,
        .sin_addr = {
            .s_addr = INADDR_ANY,
        }
    };

    i32 sock_opt_result = setsockopt(server->fd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int));
    if (sock_opt_result < 0) {
        perror("setsockopt");
        return sock_opt_result;
    }

    i32 bind_result = bind(server->fd, (struct sockaddr *) &addr, sizeof(addr));
    if (bind_result < 0) {
        perror("bind");
        return bind_result;
    }

    i32 listen_result = listen(server->fd, 10);
    if (listen_result < 0) {
        perror("listen");
        return listen_result;
    }

    printf("Server listening on port: %hu\n", ntohs(server->port));

    return 0;
}

i32 server_recv_request(Server *server, Request *req) {
    struct sockaddr_in addr = {0};
    socklen_t addr_len = sizeof(addr);
    req->sender_fd = accept(server->fd, (struct sockaddr *) &addr, &addr_len);

    char ipbuf[IPV4_STR_LEN];
    addr_to_cstr(&addr, ipbuf, IPV4_STR_LEN);

    if (req->sender_fd < 0) {
        perror("recv_request");
        return req->sender_fd;
    }

    char buf[MAX_REQUEST_LEN];
    recv(req->sender_fd, &buf, MAX_REQUEST_LEN, 0);

    char *s = &buf[0];
    char http_method[MAX_HTTP_METHOD_LEN];
    memset(http_method, 0, MAX_HTTP_METHOD_LEN);
    memset(req->resource, 0, MAX_RESOURCE_LEN);

    // TODO: Very unsafe, but works lol
    u8 part = 0;
    while (*s != '\r') {
        if (*s == ' ') {
            part++;
            s++;
        }

        switch (part) {
        case 0:
            http_method[strlen(http_method)] = *s;
            break;
        case 1:
            req->resource[strlen(req->resource)] = *s;
            break;
        default:
            break;
        }

        s++;
    }

    req->method = cstr_to_http_method(http_method);

    printf("%s %s from %s\n", http_method, req->resource, ipbuf); // Leaking client IP lmao

    return 0;
}

// TODO: Unsafe but works, pls fix later
i32 send_html(
    i32 fd,
    const char *path,
    u16 code,
    const char *reason,
    Html_Template_Args *args
) {
    if (code < 100 || code > 999) {
        printf("send_html: Status code must be between 100-999");
        return -1;
    }

    char *contents = read_file_to_cstr(path);
    if (contents == NULL) {
        return -1;
    }
    usize file_size = strlen(contents);

    struct sockaddr_in addr = {0};
    socklen_t socklen = sizeof(addr);
    i32 peername_result = getpeername(fd, (struct sockaddr *) &addr, &socklen);
    if (peername_result < 0) {
        perror("send_html");
        return -1;
    }

    char ipbuf[IPV4_STR_LEN];
    addr_to_cstr(&addr, ipbuf, IPV4_STR_LEN);

    char *header_format = "HTTP/1.1 %d %s\r\nContent-Type: text/html\r\n\r\n";
    usize header_size = snprintf(NULL, 0, header_format, code, reason) + 1;
    char header[header_size];
    snprintf(header, header_size, header_format, code, reason);

    char *base_dir = get_base_dir(path);
    char *rendered_html = render_html(contents, file_size, base_dir, args);
    free(base_dir);

    usize response_size = strlen(rendered_html) + strlen(header);
    char *response = (char *)malloc(sizeof(char) * response_size + 1);
    memset(response, 0, response_size);

    strcpy(response, header);
    strcat(response, rendered_html);

    i32 send_result = send(fd, response, response_size, 0);
    if (send_result < 0) {
        perror("send_html");
        free(response);
        free(rendered_html);
        free(contents);
        return send_result;
    }

    free(response);
    free(rendered_html);
    free(contents);

    return 0;
}
