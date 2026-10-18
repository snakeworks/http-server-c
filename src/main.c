#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include "base.h"
#include "server.h"

#define PORT 8080

i32 process_request(Request *req) {
    if (strcmp(req->resource, "/") == 0) {
        Html_Template_Args args = {
            .argc = 1,
            .arg_keys = (char *[]){"name"},
            .arg_values = (char *[]){"Bob"},
        };
        return send_html(req->sender_fd, "./templates/index.html", 200, "OK", &args);
    }
    else return send_html(req->sender_fd, "./templates/404.html", 404, "Not Found", NULL);

    return 0;
}

i32 main() {
    Server server = {0};
    if (server_init(&server, PORT) < 0) {
        return -1;
    }

    while (1) {
        Request req = {0};
        if (server_recv_request(&server, &req) < 0) {
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
