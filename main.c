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

typedef struct {
    size_t argc;
    char **arg_keys;
    char **arg_values;
} Html_Template_Args;

Http_Method cstr_to_http_method(char *str) {
    if (strcmp(str, "GET") == 0) return GET;
    else if (strcmp(str, "POST") == 0) return POST;
    else UNKNOWN;
}

char *read_file_to_cstr(const char *path) {
    FILE *file = fopen(path, "r");

    if (file == NULL) {
        printf("read_file_to_cstr: Failed to open file '%s'\n", path);
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    size_t file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    char *contents = (char *)malloc(sizeof(char) * file_size + 1);
    fread(contents, file_size, sizeof(char), file);
    fclose(file);
    contents[file_size] = '\0';

    return contents;
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
int addr_to_cstr(
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
    addr_to_cstr(&addr, ipbuf, IPV4_STR_LEN);
    printf("Received request from IP: %s\n", ipbuf); // Leaking client IP lmao

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

// TODO: Unsafe and probably breaks easily, just like every other god damn function in this project
char *render_html(
    const char *contents,
    size_t contents_size,
    Html_Template_Args *args
) {
    char *buf = malloc(sizeof(char) * contents_size);
    memset(buf, 0, contents_size);

    size_t total_size = contents_size;
    size_t buf_index = 0;

    int inside_braces = 0;
    char *arg_buf;
    for (size_t i = 0; i < contents_size; i++) {
        if (contents[i] == '{') {
            inside_braces = 1;
            arg_buf = malloc(sizeof(char) * 1024);
            memset(arg_buf, 0, 1024);
        } else if (contents[i] == '}') {
            inside_braces = 0;

            // TODO: So much hacks and dirty code but it works
            char *arg_value = "";
            int should_free_value = 0;
            if (arg_buf[0] == '@') {
                should_free_value = 1;

                arg_buf++;

                char *cur_dir = getcwd(NULL, 0);
                size_t str_size = strlen(cur_dir) + strlen(arg_buf) + 2;

                char full_path[str_size];

                snprintf(full_path, str_size, "%s/%s", cur_dir, arg_buf);
                arg_value = read_file_to_cstr(full_path);

                arg_buf--;
                free(cur_dir);
            } else {
                for (size_t j = 0; j < args->argc; j++) {
                    if (strcmp(arg_buf, args->arg_keys[j]) == 0) {
                        arg_value = args->arg_values[j];
                        break;
                    }
                }
            }

            size_t arg_value_size = strlen(arg_value);
            total_size += arg_value_size;
            char *new_buf = realloc(buf, total_size);
            if (new_buf == NULL) {
                printf("Ran out of memory\n");
                exit(-1);
            }
            buf = new_buf;

            for (size_t j = 0; j < arg_value_size; j++) {
                buf[buf_index] = arg_value[j];
                buf_index++;
            }

            if (should_free_value == 1) free(arg_value);
            free(arg_buf);
        } else if (inside_braces == 1) {
            arg_buf[strlen(arg_buf)] = contents[i];
        } else {
            buf[buf_index] = contents[i];
            buf_index++;
        }
    }

    buf[buf_index] = '\0';

    return buf;
}

// TODO: Unsafe but works, pls fix later
int send_html(
    int fd,
    const char *path,
    int code,
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
    size_t file_size = strlen(contents);

    struct sockaddr_in addr = {0};
    socklen_t socklen = sizeof(addr);
    int peername_result = getpeername(fd, (struct sockaddr *) &addr, &socklen);
    if (peername_result < 0) {
        perror("send_html");
        return -1;
    }

    char ipbuf[IPV4_STR_LEN];
    addr_to_cstr(&addr, ipbuf, IPV4_STR_LEN);

    char *header_format = "HTTP/1.1 %d %s\r\nContent-Type: text/html\r\n\r\n";
    size_t header_size = strlen(header_format) + strlen(reason) + 3; // +3 because of status code
    char header[header_size];
    sprintf(header, header_format, code, reason);

    char *parsed_html = render_html(contents, file_size, args);

    size_t response_size = strlen(parsed_html) + header_size + 1;
    char *response = (char *)malloc(sizeof(char) * response_size);
    memset(response, 0, response_size);

    strcpy(response, header);
    strcat(response, parsed_html);

    int send_result = send(fd, response, response_size, 0);
    if (send_result < 0) {
        perror("send_html");
        free(response);
        free(parsed_html);
        free(contents);
        return send_result;
    }

    free(response);
    free(parsed_html);
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

    if (strcmp(req->resource, "/") == 0) {
        Html_Template_Args args = {
            .argc = 1,
            .arg_keys = (char *[]){"name"},
            .arg_values = (char *[]){"Bob"},
        };
        return send_html(req->sender_fd, "./index.html", 200, "OK", &args);
    }
    else return send_html(req->sender_fd, "./404.html", 404, "Not Found", NULL);

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
