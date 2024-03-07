#include "io.h"
#include "ds.h"

int io_read_file(const char *filename, char **input) {
    int result = 0;

    ds_string_builder sb = {0};
    ds_string_builder_init(&sb);

    FILE *file = NULL;
    if (filename != NULL) {
        if ((file = fopen(filename, "r")) == NULL) {
            DS_LOG_ERROR("Failed to open file: %s", filename);
            return_defer(1);
        }
    } else {
        file = stdin;
    }

    char line[1024] = {0};
    while (fgets(line, sizeof(line), file) != NULL) {
        if (ds_string_builder_appendn(&sb, line, strlen(line)) != 0) {
            DS_LOG_ERROR("Failed to append line to string builder");
            return_defer(1);
        }
    }

    ds_string_builder_build(&sb, input);

defer:
    if (filename != NULL && file != NULL) {
        fclose(file);
    }
    ds_string_builder_free(&sb);
    return result;
}
