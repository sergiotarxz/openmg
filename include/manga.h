#pragma once
#include <libsoup/soup.h>
#include <libxml/HTMLparser.h>
#include <libxml/xpath.h>
#ifndef PCRE2_CODE_UNIT_WIDTH
#define PCRE2_CODE_UNIT_WIDTH 8
#include <pcre2.h>
#endif

struct Manga {
	char *title;
	char *image_url;
};

struct SplittedString {
    struct String *substrings;
    size_t n_strings;
};

struct String {
    char *content;
    size_t size;
};

struct SplittedString *
split(char *re_str, size_t re_str_size, const char *subject, size_t subject_size);
char *
alloc_string(size_t len);
void
splitted_string_free (struct SplittedString *splitted_string);
char *
match_1 (char *re_str, char *subject);
void
copy_substring(const char *origin, char *dest, size_t dest_len, size_t start,
        size_t len);
