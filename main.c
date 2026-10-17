#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>

#define PORT 8080
#define MAX_REQUEST_LEN 1024
#define MAX_RESOURCE_LEN 512
#define MAX_HTTP_METHOD_LEN 7
#define IPV4_STR_LEN 16

typedef enum {
    GET,
    POST,
    UNKNOWN,
} Http_Method;

typedef struct {
    int fd;
    unsigned short port;
} Server;

typedef struct {
    int sender_fd;
    Http_Method method;
    char resource[MAX_RESOURCE_LEN];
} Request;

Http_Method cstr_to_http_method(char *str) {
    if (strcmp(str, "GET") == 0) return GET;
    else if (strcmp(str, "POST") == 0) return POST;
    else UNKNOWN;
}

int server_init(Server *server, unsigned short port) {
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

    int sock_opt_result = setsockopt(server->fd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int));
    if (sock_opt_result < 0) {
        perror("setsockopt");
        return sock_opt_result;
    }

    int bind_result = bind(server->fd, (struct sockaddr *) &addr, sizeof(addr));
    if (bind_result < 0) {
        perror("bind");
        return bind_result;
    }

    int listen_result = listen(server->fd, 10);
    if (listen_result < 0) {
        perror("listen");
        return listen_result;
    }

    return 0;
}

// NOTE: I know there is the inet_ntop function, but I thought it'd be fun to write my own.
// It will probably only make sense on little endian machines though, so don't try running
// this on your IBM System/360. Also it won't work on IPv6 addresses, but who cares?
int addr_to_str(
    short buffer_size;
    struct sockaddr_in *addr,
    char buffer[restrict buffer_size],
    short buffer_size
) {
    if (buffer_size < IPV4_STR_LEN) {
        return -1;
    }

    unsigned int addr_uint = addr->sin_addr.s_addr; // addr_uint is big endian!
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

int recv_request(Server *server, Request *req) {
    struct sockaddr_in addr = {0};
    socklen_t addr_len = sizeof(addr);
    req->sender_fd = accept(server->fd, (struct sockaddr *) &addr, &addr_len);

    char ipbuf[IPV4_STR_LEN];
    addr_to_str(&addr, ipbuf, IPV4_STR_LEN);
    printf("Recevied request from IP: %s\n", ipbuf); // Leaking client IP lmao

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
    int part = 0;
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

    return 0;
}

// TODO: Unsafe but works, pls fix later
int send_html(int fd, const char *path, int code, const char *reason) {
    if (code < 100 || code > 999) {
        printf("send_html: Status code must be between 100-999");
        return -1;
    }

    FILE *file = fopen(path, "r");

    if (file == NULL) {
        printf("send_html: Failed to open file '%s'\n", path);
        return -1;
    }

    char *header_format = "HTTP/1.1 %d %s\r\nContent-Type: text/html\r\n\r\n";
    size_t header_size = strlen(header_format) + strlen(reason) + 3; // +3 because of status code
    char header[header_size];
    sprintf(header, header_format, code, reason);

    fseek(file, 0, SEEK_END);
    size_t file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    char *contents = (char *)malloc(sizeof(char) * file_size);
    fread(contents, file_size, sizeof(char), file);
    fclose(file);

    size_t response_size = file_size + header_size;
    char *response = (char *)malloc(sizeof(char) * response_size);
    memset(response, 0, response_size);

    strcpy(response, header);
    strcat(response, contents);

    int send_result = send(fd, response, response_size, 0);
    if (send_result < 0) {
        perror("send_html");
        free(response);
        free(contents);
        return send_result;
    }

    free(response);
    free(contents);

    return 0;
}

int process_request(Request *req) {
    if (req->method != GET) {
        char *response = "HTTP/1.1 400 Bad Request\r\nContent-Type: text/html\r\n\r\n<p>Only send GET requests</p>\r\n";
        int send_result = send(req->sender_fd, response, strlen(response), 0);
        if (send_result < 0) {
            perror("process_request");
            return send_result;
        }
        return 0;
    }

    if (strcmp(req->resource, "/") == 0) return send_html(req->sender_fd, "./index.html", 200, "OK");
    else return send_html(req->sender_fd, "./404.html", 404, "Not Found");

    return 0;
}

int main() {
    Server server = {0};
    if (server_init(&server, PORT) < 0) {
        return -1;
    }

    printf("Server listening on port: %hu\n", ntohs(server.port));

    while (1) {
        Request req = {0};
        if (recv_request(&server, &req) < 0) {
            close(req.sender_fd);
            continue;
        }

        if (process_request(&req) < 0) {
            close(req.sender_fd);
            continue;
        }

        close(req.sender_fd);
    }

    close(server.fd);

    return 0;
}
