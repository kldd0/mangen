#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <dirent.h>
#include <sys/stat.h>

#define VERSION "1.0"

void version(FILE *stream)
{
    fprintf(stream, "MANGEN Version %s\n", VERSION);
}

void usage(FILE *stream)
{
    fprintf(stream, "Usage: ./mangen [DIR_PATH] [OPTIONS]\n");
    fprintf(stream, "Generate a manifest of files in the specified directory with hashes.\n\n");
    fprintf(stream, "ARGUMENTS:\n");
    fprintf(stream, "    DIR_PATH    Directory to generate manifest for (default: current directory)\n");
    fprintf(stream, "OPTIONS:\n");
    fprintf(stream, "    -h          Display this help message\n");
    fprintf(stream, "    -v          Display version information\n");
}

void traverse_directory(const char *path, const char *cur_path)
{

    DIR *dir = opendir(cur_path);
    if (dir == NULL) {
        fprintf(stderr, "ERROR: Could not open directory `%s`\n", strerror(errno));
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        char entry_path[1024];
        snprintf(entry_path, sizeof(entry_path), "%s/%s", cur_path, entry->d_name);

        struct stat statbuf;
        if (stat(entry_path, &statbuf) == -1) {
            fprintf(stderr, "ERROR: Failed to stat `%s` %s\n", entry_path, strerror(errno));
            continue;
        }

        if (S_ISDIR(statbuf.st_mode)) {
            traverse_directory(path, entry_path);
        } else if (S_ISREG(statbuf.st_mode)) {
            fprintf(stdout, "%s\n", entry_path);
        }
    }
    closedir(dir);
}


int main(int argc, char **argv)
{
    int c;
    const char *path = ".";

    while ((c = getopt(argc, argv, "hv")) != -1) {
        switch (c) {
            case 'h':
                usage(stdout);
                return 0;
            case 'v':
                version(stdout);
                return 0;
            default:
                usage(stdout);
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

    traverse_directory(path, path);
    return 0;
}
