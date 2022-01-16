#pragma once
#include <glib-object.h>

G_BEGIN_DECLS;

#define MG_TYPE_UTIL_REGEX mg_util_regex_get_type()
G_DECLARE_FINAL_TYPE (MgUtilRegex, mg_util_regex, MG, UTIL_REGEX, GObject)

MgUtilRegex *mg_util_regex_new ();


struct SplittedString {
    struct String *substrings;
    size_t n_strings;
};

struct String {
    char *content;
    size_t size;
};

struct SplittedString *
mg_util_regex_split (MgUtilRegex *self,
        char *re_str, size_t re_str_size, const char *subject, size_t subject_size);
void
mg_util_regex_splitted_string_free (MgUtilRegex *self,
        struct SplittedString *splitted_string);
char *
mg_util_regex_match_1 (MgUtilRegex *self,
        const char *re_str, const char *subject);

G_END_DECLS
