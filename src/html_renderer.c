#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "html_renderer.h"

#define MAX_ARG_NAME_LEN 1024

const char *peek(const char *contents, usize index) {
    return &contents[index];
}

const char *advance(const char *contents, usize *index) {
    *index = *index + 1;
    return &contents[*index];
}

// TODO: Unsafe and probably breaks easily, just like every other god damn function in this project
char *render_html(
    const char *contents,
    usize contents_size,
    const char *base_dir,
    Html_Template_Args *args
) {
    char *buf = malloc(sizeof(char) * contents_size);
    memset(buf, 0, contents_size);

    usize total_size = contents_size;

    usize buf_index = 0;
    usize contents_index = 0;
    while (contents_index < contents_size) {
        switch (*peek(contents, contents_index)) {
        case '{':
            char arg_buf[MAX_ARG_NAME_LEN];
            memset(arg_buf, 0, MAX_ARG_NAME_LEN);
            usize arg_index = 0;

            while (*advance(contents, &contents_index) != '}') {
                if (arg_index >= MAX_ARG_NAME_LEN) continue;
                arg_buf[arg_index] = *peek(contents, contents_index);
                arg_index++;
            }

            // TODO: So much hacks and dirty code but it works
            char *arg_value = "";
            bool should_free_value = false;
            if (arg_buf[0] == '@') {
                should_free_value = true;

                usize str_size = strlen(base_dir) + strlen(arg_buf + 1) + 2; // +2 for slash in filepath and null terminator

                char full_path[str_size];

                snprintf(full_path, str_size, "%s/%s", base_dir, arg_buf + 1);
                arg_value = read_file_to_cstr(full_path);
            } else {
                for (usize i = 0; i < args->argc; i++) {
                    if (strcmp(arg_buf, args->arg_keys[i]) == 0) {
                        arg_value = args->arg_values[i];
                        break;
                    }
                }
            }

            usize arg_value_size = strlen(arg_value);
            total_size += arg_value_size;
            char *new_buf = realloc(buf, total_size);
            if (new_buf == NULL) {
                printf("Ran out of memory\n");
                exit(-1);
            }
            buf = new_buf;

            for (usize i = 0; i < arg_value_size; i++) {
                buf[buf_index] = arg_value[i];
                buf_index++;
            }

            if (should_free_value == true) free(arg_value);

            contents_index++;
            break;
        default:
            buf[buf_index] = *peek(contents, contents_index);
            contents_index++;
            buf_index++;
            break;
        }
    }

    buf[buf_index] = '\0';

    return buf;
}
