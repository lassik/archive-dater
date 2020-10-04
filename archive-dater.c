// Copyright 2020 Lassi Kortela
// SPDX-License-Identifier: ISC

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <archive.h>
#include <archive_entry.h>

static void panic(const char *msg)
{
    fprintf(stderr, "%s\n", msg);
    exit(2);
}

static void panic_memory(void) { panic("out of memory"); }

static size_t string_vector_length(char **sv)
{
    size_t n = 0;
    for (; *sv; sv++) {
        n++;
    }
    return n;
}

static char **string_vector_new(void)
{
    char **sv;

    if (!(sv = calloc(1, sizeof(*sv)))) {
        panic_memory();
    }
    return sv;
}

static char **string_vector_append(char **sv, char *s)
{
    size_t len = string_vector_length(sv);
    size_t cap = 1;

    while (cap <= len + 1) {
        cap *= 2;
    }
    if (!(sv = realloc(sv, cap * sizeof(*sv)))) {
        panic_memory();
    }
    sv[len++] = s;
    sv[len] = 0;
    return sv;
}

#define MAX_DATES 1024
#define DATE_SIZE 16

struct dateinfo {
    char date[DATE_SIZE];
    char **files;
};

static struct dateinfo dates[MAX_DATES];
static size_t ndates;

static int compare_dates(const void *a_void, const void *b_void)
{
    const struct dateinfo *a = a_void;
    const struct dateinfo *b = b_void;

    return strcmp(b->date, a->date);
}

static void date_add_file(const char *date, const char *file)
{
    struct dateinfo *dateinfo;
    char *file_copy;

    if (ndates >= MAX_DATES) {
        panic("too many dates");
    }
    if (!(dateinfo
            = bsearch(date, dates, ndates, sizeof(*dates), compare_dates))) {
        dateinfo = &dates[ndates++];
        snprintf(dateinfo->date, sizeof(dateinfo->date), "%s", date);
        dateinfo->files = string_vector_new();
        qsort(dates, ndates, sizeof(*dates), compare_dates);
    }
    if (!(file_copy = strdup(file))) {
        panic_memory();
    }
    dateinfo->files = string_vector_append(dateinfo->files, file_copy);
}

static void show_all_dates(void)
{
    struct dateinfo *dateinfo;
    char **filep;
    char *file;

    for (dateinfo = dates; dateinfo < dates + ndates; dateinfo++) {
        printf("%s\n", dateinfo->date);
        for (filep = dateinfo->files; (file = *filep); filep++) {
            printf("* %s\n", file);
        }
        printf("\n");
    }
}

static int check(int rv)
{
    if (rv < 0) {
        panic("error");
    }
    return rv;
}

static void generic_usage(FILE *stream, int status)
{
    fprintf(stream, "usage\n");
    exit(status);
}

static void usage(void) { generic_usage(stderr, 2); }

static void do_it(const char *filename)
{
    struct archive *archive;
    struct archive_entry *entry;
    struct tm *tm;
    time_t t;
    unsigned int y, m, d;
    char date[DATE_SIZE];

    if (!(archive = archive_read_new())) {
        panic_memory();
    }
    check(archive_read_support_filter_all(archive));
    check(archive_read_support_format_all(archive));
    check(archive_read_open_filename(archive, filename, 10240));
    while (check(archive_read_next_header(archive, &entry)) != ARCHIVE_EOF) {
        if (archive_entry_mtime_is_set(entry)) {
            t = archive_entry_mtime(entry);
            tm = gmtime(&t);
            y = tm->tm_year + 1900;
            m = tm->tm_mon + 1;
            d = tm->tm_mday;
            snprintf(date, sizeof(date), "%u-%02u-%02u", y, m, d);
            date_add_file(date, archive_entry_pathname(entry));
        }
    }
    show_all_dates();
}

int main(int argc, char **argv)
{
    if (argc != 2) {
        usage();
    }
    do_it(argv[1]);
    return 0;
}
