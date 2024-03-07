#include "ds.h"
#include <stdio.h>
#define ARGPARSE_IMPLEMENTATION
#include "argparse.h"
#include "io.h"
#include "md5.h"
#include <pthread.h>

struct bruteforce_md5_thread_args {
        char *input;
        ds_dynamic_array words_array;
        size_t start;
        size_t end;
        char **match;
        pthread_mutex_t *mutex;
};

static void *bruteforce_md5_thread(void *args) {
    struct bruteforce_md5_thread_args *thread_args =
        (struct bruteforce_md5_thread_args *)args;

    for (size_t i = thread_args->start; i < thread_args->end; i++) {
        char *str = NULL;
        ds_dynamic_array_get(&thread_args->words_array, i, &str);

        char digest[33] = {0};
        if (md5_hash_digest(str, digest) != 0) {
            DS_LOG_ERROR("Failed to hash input");
            continue;
        }

        if (strcmp(digest, thread_args->input) == 0) {
            pthread_mutex_lock(thread_args->mutex);
            if (*thread_args->match == NULL) {
                *thread_args->match = str;
            }
            pthread_mutex_unlock(thread_args->mutex);
            break;
        }
    }

defer:
    return NULL;
}

int bruteforce_md5_threaded(const char *filename, int thread_count,
                            char **match) {
    int result = 0;
    char *input = NULL;
    char *dictionary = NULL;
    ds_string_slice words = {0};
    ds_string_slice word = {0};
    ds_dynamic_array words_array = {0};

    pthread_t threads[thread_count];
    struct bruteforce_md5_thread_args thread_args[thread_count];

    pthread_mutex_t mutex;
    pthread_mutex_init(&mutex, NULL);

    if (filename == NULL) {
        DS_LOG_ERROR("Bruteforce requires a dictionary file");
        return_defer(1);
    }

    if (io_read_file(filename, &dictionary) != 0) {
        DS_LOG_ERROR("Failed to read dictionary");
        return_defer(1);
    }

    if (io_read_file(NULL, &input) != 0) {
        DS_LOG_ERROR("Failed to read input");
        return_defer(1);
    }

    ds_string_slice_init(&words, dictionary, strlen(dictionary));
    ds_dynamic_array_init(&words_array, sizeof(char *));
    while (ds_string_slice_tokenize(&words, '\n', &word) == 0) {
        char *str = NULL;
        if (ds_string_slice_to_owned(&word, &str) != 0) {
            DS_LOG_ERROR("Failed to copy word");
            return_defer(1);
        }

        ds_dynamic_array_append(&words_array, &str);
    }

    for (int i = 0; i < thread_count; i++) {
        thread_args[i].input = input;
        thread_args[i].words_array = words_array;
        thread_args[i].start = i * (words_array.count / thread_count);
        thread_args[i].end = (i + 1) * (words_array.count / thread_count);
        if (i == thread_count - 1) {
            thread_args[i].end = words_array.count;
        }
        thread_args[i].match = match;
        thread_args[i].mutex = &mutex;

        if (pthread_create(&threads[i], NULL, bruteforce_md5_thread,
                           &thread_args[i]) != 0) {
            DS_LOG_ERROR("Failed to create thread");
            return_defer(1);
        }
    }

    for (int i = 0; i < thread_count; i++) {
        if (pthread_join(threads[i], NULL) != 0) {
            DS_LOG_ERROR("Failed to join thread");
            return_defer(1);
        }
    }

    if (*match == NULL) {
        DS_LOG_ERROR("Failed to find a match");
        return_defer(1);
    }

defer:
    pthread_mutex_destroy(&mutex);
    for (size_t i = 0; i < words_array.count; i++) {
        char *str = NULL;
        ds_dynamic_array_get(&words_array, i, &str);
        if (*match != NULL && strcmp(str, *match) == 0) {
            continue;
        }
        free(str);
    }
    ds_dynamic_array_free(&words_array);
    ds_string_slice_free(&words);
    if (input != NULL) {
        free(input);
    }
    if (dictionary != NULL) {
        free(dictionary);
    }
    return result;
}

int bruteforce_md5(const char *filename, char **match) {
    int result = 0;
    char *input = NULL;
    char *dictionary = NULL;
    ds_string_slice words = {0};
    ds_string_slice word = {0};

    if (filename == NULL) {
        DS_LOG_ERROR("Bruteforce requires a dictionary file");
        return_defer(1);
    }

    if (io_read_file(filename, &dictionary) != 0) {
        DS_LOG_ERROR("Failed to read dictionary");
        return_defer(1);
    }

    if (io_read_file(NULL, &input) != 0) {
        DS_LOG_ERROR("Failed to read input");
        return_defer(1);
    }

    ds_string_slice_init(&words, dictionary, strlen(dictionary));
    while (ds_string_slice_tokenize(&words, '\n', &word) == 0) {
        char *str = NULL;
        if (ds_string_slice_to_owned(&word, &str) != 0) {
            DS_LOG_ERROR("Failed to copy word");
            return_defer(1);
        }

        char digest[33] = {0};
        if (md5_hash_digest(str, digest) != 0) {
            DS_LOG_ERROR("Failed to hash input");
            return_defer(1);
        }

        if (strcmp(digest, input) == 0) {
            *match = str;
            return_defer(0);
        }

        free(str);
    }

    DS_LOG_ERROR("Failed to find a match");
    return_defer(1);

defer:
    ds_string_slice_free(&words);
    if (input != NULL) {
        free(input);
    }
    if (dictionary != NULL) {
        free(dictionary);
    }
    return result;
}

int hash_md5(const char *filename) {
    int result = 0;
    char *input = NULL;

    if (io_read_file(filename, &input) != 0) {
        DS_LOG_ERROR("Failed to read input");
        return_defer(1);
    }

    char digest[33] = {0};
    if (md5_hash_digest(input, digest) != 0) {
        DS_LOG_ERROR("Failed to hash input");
        return_defer(1);
    }

    printf("%s\n", digest);

defer:
    if (input != NULL) {
        free(input);
    }
    return result;
}

int main(int argc, char **argv) {
    int result = 0;
    struct argparse_parser *parser =
        argparse_new("md5", "1.0.0", "md5sum clone");
    if (parser == NULL) {
        DS_LOG_ERROR("Failed to create parser");
        return_defer(1);
    }

    argparse_add_argument(
        parser,
        (struct argparse_options){
            .short_name = 'f',
            .long_name = "file",
            .description = "File to hash; if not provided, reads from stdin",
            .type = ARGUMENT_TYPE_VALUE,
            .required = 0,
        });
    argparse_add_argument(
        parser,
        (struct argparse_options){
            .short_name = 'c',
            .long_name = "bruteforce",
            .description =
                "Attempt to bruteforce the hash provided as input; this will "
                "use the file as a dictionary and makes it required",
            .type = ARGUMENT_TYPE_FLAG,
            .required = 0,
        });

    argparse_add_argument(
        parser, (struct argparse_options){
                    .short_name = 'p',
                    .long_name = "threads",
                    .description = "Number of threads to use for bruteforce",
                    .type = ARGUMENT_TYPE_VALUE,
                    .required = 0,
                });

    if (argparse_parse(parser, argc, argv) != 0) {
        DS_LOG_ERROR("Failed to parse arguments");
        return_defer(1);
    }

    char *filename = argparse_get_value(parser, "file");
    unsigned int bruteforce = argparse_get_flag(parser, "bruteforce");
    char *threads_value = argparse_get_value(parser, "threads");

    int thread_count = 1;
    if (threads_value != NULL) {
        thread_count = atoi(threads_value);
    }

    if (bruteforce == 1) {
        char *match = NULL;

        if (thread_count > 1) {
            if (bruteforce_md5_threaded(filename, thread_count, &match) != 0) {
                return_defer(1);
            }
        } else {
            if (bruteforce_md5(filename, &match) != 0) {
                return_defer(1);
            }
        }

        printf("%s\n", match);
        free(match);
    } else {
        if (hash_md5(filename) != 0) {
            return_defer(1);
        }
    }

defer:
    if (parser != NULL) {
        argparse_free(parser);
    }
    return result;
}
