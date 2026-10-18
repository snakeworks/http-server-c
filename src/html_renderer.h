#ifndef HTML_RENDERER_H
#define HTML_RENDERER_H

#include "base.h"

typedef struct {
    usize argc;
    char **arg_keys;
    char **arg_values;
} Html_Template_Args;

char *render_html(
    const char *contents,
    usize contents_size,
    const char *base_dir,
    Html_Template_Args *args
);

#endif // HTML_RENDERER_H
