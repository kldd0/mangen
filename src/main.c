#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <dirent.h>
#include <stdlib.h>
#include <fnmatch.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <openssl/evp.h>

#define VERSION "1.0"

#define MAX_DEPTH 1000
#define BUFFER_SIZE 4096

#define return_defer(value) do { result = (value); goto defer; } while (0)

void version(FILE *stream)
{
    fprintf(stream, "MANGEN Version %s\n", VERSION);
}

void usage(FILE *stream)
{
    fprintf(stream, "Usage: ./mangen [DIR_PATH] [OPTIONS]\n");
    fprintf(stream, "Generate a manifest of files in the specified directory with hashes.\n\n");
    fprintf(stream, "ARGUMENTS:\n");
    fprintf(stream, "    DIR_PATH      Directory to generate manifest for (default: current directory)\n");
    fprintf(stream, "OPTIONS:\n");
    fprintf(stream, "    -h            Display this help message\n");
    fprintf(stream, "    -v            Display version information\n");
    fprintf(stream, "    -e PATTERN    Exclude entries matching GLOB pattern (e.g., -e \"*.txt\")\n");
}

bool match(const char *string, const char *pattern)
{
    int flags = FNM_PATHNAME | FNM_PERIOD;
    return (fnmatch(pattern, string, flags) == 0);
}

bool hash(const char *path, char *hash_str)
{

    FILE *f = fopen(path, "rb");
    if (f == NULL) goto fail;

    // MD stands for `message digest`
    // mdctx stores hash context
    EVP_MD_CTX *mdctx = EVP_MD_CTX_new();
    if (mdctx == NULL) goto fail;

    if (EVP_DigestInit_ex(mdctx, EVP_sha256(), NULL) == 0) goto fail;

    char hash_buffer[BUFFER_SIZE];
    size_t read = 0;
    while ((read = fread(hash_buffer, 1, sizeof(hash_buffer), f)) > 0) {
        if (EVP_DigestUpdate(mdctx, hash_buffer, read) == 0) goto fail;
    }
    if (ferror(f)) {
        // ferror does not set errno
        goto fail;
    }

    // Finalize hash context
    unsigned int md_len;
    unsigned char md_value[EVP_MAX_MD_SIZE];
    if (EVP_DigestFinal_ex(mdctx, md_value, &md_len) == 0) goto fail;

    for (unsigned int i = 0; i < md_len; i++) {
        sprintf(hash_str + (i * 2), "%02x", md_value[i]);
    }
    hash_str[md_len * 2] = '\0';

    fclose(f);
    EVP_MD_CTX_free(mdctx);
    return true;
fail:
    fprintf(stderr, "ERROR: Could not hash file `%s`: %s\n", path, strerror(errno));
    if (f) fclose(f);
    if (mdctx) EVP_MD_CTX_free(mdctx);
    return false;
}

void traverse_directory(const char *base_path, const char *cur_path, const char *excluding_pattern, int depth)
{
    if (depth >= MAX_DEPTH) return;

    DIR *dir = opendir(cur_path);
    if (dir == NULL) {
        fprintf(stderr, "ERROR: Could not open directory `%s`\n", strerror(errno));
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 ||
                strcmp(entry->d_name, "..") == 0 ||
                match(entry->d_name, excluding_pattern)) {
            continue;
        }

        // +2 for sep '/' and null terminator
        size_t path_len = strlen(cur_path) + strlen(entry->d_name) + 2;
        char entry_path[path_len];

        size_t cx = snprintf(entry_path, path_len, "%s/%s", cur_path, entry->d_name);
        if (cx + 1 < path_len) {
            fprintf(stderr, "ERROR: Failed to create path string `%s` %zu/%zu\n", entry_path, cx, path_len);
            continue;
        }

        struct stat statbuf;
        if (stat(entry_path, &statbuf) == -1) {
            fprintf(stderr, "ERROR: Failed to stat `%s` %s\n", entry_path, strerror(errno));
            continue;
        }

        if (S_ISDIR(statbuf.st_mode)) {
            traverse_directory(base_path, entry_path, excluding_pattern, ++depth);
        } else if (S_ISREG(statbuf.st_mode)) {
            char hash_str[EVP_MAX_MD_SIZE];
            if (hash(entry_path, hash_str)) {
                size_t base_path_len = strlen(base_path);
                // +1 for sep '/'
                char *relative_path = entry_path + base_path_len + 1;
                fprintf(stdout, "%s : %s\n", relative_path, hash_str);
            }
        }
    }
    closedir(dir);
}


int main(int argc, char **argv)
{
    int c;
    const char *path = ".";
    const char *excluding_pattern = "";

    while ((c = getopt(argc, argv, "hve:")) != -1) {
        switch (c) {
            case 'h':
                usage(stdout);
                return 0;
            case 'v':
                version(stdout);
                return 0;
            case 'e':
                excluding_pattern = optarg;
                break;
            default:
                return 1;
        }
    }

    if (optind < argc) {
        path = argv[optind];
    } else {
        fprintf(stdout, "WARN: DIR_PATH was not provided, running with DEFAULT_PATH `%s`\n", path);
    }

    struct stat statbuf;
    if (stat(path, &statbuf) == -1 || !S_ISDIR(statbuf.st_mode)) {
        fprintf(stderr, "ERROR: Could not read directory `%s`\n", path);
        return 1;
    }

    traverse_directory(path, path, excluding_pattern, 0);
    return 0;
}
