#include <stdlib.h>
#include <stdio.h>
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
