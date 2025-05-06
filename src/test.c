#include "unit.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <stdbool.h>

#define MANGEN_PATH "./build/mangen"
#define MAX_PATH 4096
#define BUFFER_SIZE (1024 * 1024)
#define HASH_STR_SIZE 64
#define TEST_DIR_TEMPLATE "/tmp/mangen_test_XXXXXX"

void create_temp_dir(char *template);
void generate_random_structure(const char *path, int depth, int max_depth);
void fill_file_with_random_data(const char *path);
void run(const char *path, const char *excluding_pattern, char **output);
void get_expected_output(const char *path, const char *excluding_pattern, char **output);
bool compare_outputs(const char *app_output, const char *expected_output);
void cleanup(const char *path);

static void test_mangen_basic(void)
{
    unit_test_start();

    srand(time(NULL));
    char template[] = TEST_DIR_TEMPLATE;
    create_temp_dir(template);

    generate_random_structure(template, 0, 3);

    char *app_output = NULL;
    run(template, "", &app_output);
    unit_check(app_output != NULL, "Application ran successfully");

    char *expected_output = NULL;
    get_expected_output(template, "", &expected_output);
    unit_check(expected_output != NULL, "Expected output generated");

    unit_check(compare_outputs(app_output, expected_output), "Outputs match exactly");

    free(app_output);
    free(expected_output);
    cleanup(template);

    unit_test_finish();
}

static void test_mangen_with_exclusion(const char *excluding_pattern)
{
    unit_test_start();

    srand(time(NULL));
    char template[] = TEST_DIR_TEMPLATE;
    create_temp_dir(template);

    generate_random_structure(template, 0, 3);

    char *app_output = NULL;
    run(template, excluding_pattern, &app_output);
    unit_check(app_output != NULL, "App ran with exclusion");

    char *expected_output = NULL;
    get_expected_output(template, excluding_pattern, &expected_output);
    unit_check(expected_output != NULL, "Expected output with exclusion");

    unit_check(compare_outputs(app_output, expected_output),
            "Outputs match with exclusion");

    free(app_output);
    free(expected_output);
    cleanup(template);

    unit_test_finish();
}

void create_temp_dir(char *template)
{
    unit_check(mkdtemp(template) != NULL, "Create temp directory");
}

void generate_random_structure(const char *path, int depth, int max_depth)
{
    if (depth >= max_depth) return;

    // Create files with different extensions
    const char *extensions[] = {".txt", ".log", ".sh"};
    for (int i = 0; i < 3; ++i) {
        char filename[MAX_PATH];
        snprintf(filename, sizeof(filename), "%s/file_%d_%d%s",
                 path, depth, i, extensions[i % 3]);
        fill_file_with_random_data(filename);
    }

    // Create subdirectories
    for (int i = 0; i < 2; ++i) {
        char dirname[MAX_PATH];
        snprintf(dirname, sizeof(dirname), "%s/dir_%d_%d", path, depth, i);
        unit_check(mkdir(dirname, 0755) == 0, "Create subdirectory");
        generate_random_structure(dirname, depth + 1, max_depth);
    }
}

void fill_file_with_random_data(const char *path)
{
    FILE *f = fopen(path, "wb");
    unit_check(f != NULL, "Open file for writing");
    unit_msg("Writing to %s", path);

    char *buf = malloc(BUFFER_SIZE);
    unit_check(buf != NULL, "Allocate buffer");

    for (size_t i = 0; i < BUFFER_SIZE; ++i)
        buf[i] = 'A' + (i % 26);

    unit_check(fwrite(buf, 1, BUFFER_SIZE, f) == BUFFER_SIZE, "Write full buffer");

    free(buf);
    fclose(f);
}

void run(const char *path, const char *excluding_pattern, char **output)
{
    char cmd[MAX_PATH * 2 + 100];
    snprintf(cmd, sizeof(cmd), "%s \"%s\" -e \"%s\" | sort > /tmp/mangen_output",
             MANGEN_PATH, path, excluding_pattern);

    int rc = system(cmd);
    unit_check(rc == 0, "Mangen executed successfully");

    FILE *f = fopen("/tmp/mangen_output", "rb");
    unit_check(f != NULL, "Open app output");

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    rewind(f);

    *output = malloc(size + 1);
    unit_check(*output != NULL, "Allocate output buffer");

    size_t read = fread(*output, 1, size, f);
    (*output)[read] = '\0';
    fclose(f);
    remove("/tmp/mangen_output");
}

void get_expected_output(const char *path, const char *excluding_pattern, char **output)
{
    char cmd[MAX_PATH * 4];
    snprintf(cmd, sizeof(cmd),
        "find \"%s\" -type f ! -name \"%s\" -exec sha256sum {} \\+ "
        "| awk -v base=\"%s/\" '{ sub(base, \"\"); print $2 \" : \" $1 }' "
        "| sort > /tmp/sha256sum_output",
        path, excluding_pattern, path);

    int rc = system(cmd);
    unit_check(rc == 0, "Generated expected output");

    FILE *f = fopen("/tmp/sha256sum_output", "rb");
    unit_check(f != NULL, "Open expected output");

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    rewind(f);

    *output = malloc(size + 1);
    unit_check(*output != NULL, "Allocate expected buffer");

    size_t read = fread(*output, 1, size, f);
    (*output)[read] = '\0';
    fclose(f);
    remove("/tmp/sha256sum_output");
}

bool compare_outputs(const char *app_output, const char *expected_output)
{
    // Direct comparison works because both outputs are sorted
    return strcmp(app_output, expected_output) == 0;
}

void cleanup(const char *path)
{
    char cmd[MAX_PATH + 10];
    snprintf(cmd, sizeof(cmd), "rm -rf \"%s\"", path);
    unit_check(system(cmd) == 0, "Cleanup test directory");
}

int main(void)
{
    unit_test_start();
    test_mangen_basic();
    test_mangen_with_exclusion("*.txt");
    test_mangen_with_exclusion("a*.txt");
    test_mangen_with_exclusion(".");
    test_mangen_with_exclusion("*");
    unit_test_finish();
    return 0;
}
