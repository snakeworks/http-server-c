#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "base.h"

char *read_file_to_cstr(const char *path) {
    FILE *file = fopen(path, "r");

    if (file == NULL) {
        printf("read_file_to_cstr: Failed to open file '%s'\n", path);
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    usize file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    char *contents = (char *)malloc(sizeof(char) * file_size + 1);
    fread(contents, file_size, sizeof(char), file);
    fclose(file);
    contents[file_size] = '\0';

    return contents;
}

char *get_base_dir(const char *file_path) {
    usize len = strlen(file_path);

    char *base = malloc(sizeof(char) * len + 1);
    strcpy(base, file_path);

    for (usize i = len; i > 0; i--) {
        if (base[i] == '/') {
            base[i] = '\0';
            break;
        }
        base[i] = '\0';
    }

    if (base[0] == '\0') base[0] = '.';

    return base;
}
