#include <glib-object.h>

#ifndef PCRE2_CODE_UNIT_WIDTH
#define PCRE2_CODE_UNIT_WIDTH 8
#include <pcre2.h>
#endif

#include <openmg/util/regex.h>
#include <openmg/util/string.h>

struct _MgUtilRegex {
    GObject parent_instance;
};

G_DEFINE_TYPE (MgUtilRegex, mg_util_regex, G_TYPE_OBJECT)

static void
mg_util_regex_class_init (MgUtilRegexClass *class) {
}
static void
mg_util_regex_init (MgUtilRegex *class) {
}

static void
mg_util_regex_iterate_string_to_split (MgUtilRegex *self,
        struct SplittedString *splitted_string,
        pcre2_code *re, int *will_break, const char *subject,
        size_t subject_size, size_t *start_pos, size_t *offset);

MgUtilRegex *
mg_util_regex_new () {
    MgUtilRegex *self = NULL;
    self = MG_UTIL_REGEX ((g_object_new (MG_TYPE_UTIL_REGEX, NULL)));
    return self;
}

struct SplittedString *
mg_util_regex_split (MgUtilRegex *self,
        char *re_str, size_t re_str_size, const char *subject, size_t subject_size) {
    pcre2_code_8 *re;
    size_t start_pos = 0;
    size_t offset    = 0;
    int regex_compile_error;
    PCRE2_SIZE error_offset;
    struct SplittedString *splitted_string;

    splitted_string = g_malloc (sizeof *splitted_string);

    splitted_string->n_strings = 0;
    splitted_string->substrings = NULL;
    re = pcre2_compile ((PCRE2_SPTR8) re_str, 
            re_str_size, 0, &regex_compile_error, &error_offset, NULL);
    while (start_pos < subject_size) {
        int will_break = 0;
        mg_util_regex_iterate_string_to_split (self,
                splitted_string, re, &will_break,
                subject, subject_size, &start_pos, &offset);
        if (will_break) {
            break;
        }
    }

    pcre2_code_free (re);
    re = NULL;

    return splitted_string;
}

void
mg_util_regex_splitted_string_free (MgUtilRegex *self,
        struct SplittedString *splitted_string) {
    for (int i = 0; i<splitted_string->n_strings; i++) {
        g_free (splitted_string->substrings[i].content);
    }

    g_free (splitted_string->substrings);
    g_free (splitted_string);
}

static void
mg_util_regex_iterate_string_to_split (MgUtilRegex *self,
        struct SplittedString *splitted_string,
        pcre2_code *re, int *will_break, const char *subject,
        size_t subject_size, size_t *start_pos, size_t *offset) {
    pcre2_match_data_8 *match_data;
    PCRE2_SIZE *ovector;
    int rc;
    MgUtilString *string_util = mg_util_string_new ();

    splitted_string->n_strings++;
    match_data = pcre2_match_data_create_from_pattern_8 (re, NULL);
    rc = pcre2_match ( re, (PCRE2_SPTR8) subject, subject_size, *start_pos, 0, match_data,
            NULL);
    if (splitted_string->substrings) {
        splitted_string->substrings = g_realloc (splitted_string->substrings, 
                (sizeof *splitted_string->substrings) * (*offset + 1));
    } else {
        splitted_string->substrings = g_malloc (sizeof *splitted_string->substrings);
    }
    if (rc < 0) {
        struct String *current_substring = 
            &splitted_string->substrings [*offset];
        current_substring->content = mg_util_string_alloc_string (string_util,
                subject_size - *start_pos);
        mg_util_string_copy_substring (string_util, subject,
                current_substring->content, subject_size,
                *start_pos, subject_size - *start_pos);
        current_substring->size = subject_size - *start_pos;

        *will_break = 1;
        goto cleanup_iterate_string_to_split;
    }
    ovector = pcre2_get_ovector_pointer_8(match_data);
    splitted_string->substrings[*offset].content = 
        mg_util_string_alloc_string (string_util, ovector[0] - *start_pos);
    mg_util_string_copy_substring (string_util,
            subject, splitted_string->substrings[*offset].content,
            subject_size, *start_pos,
            ovector[0] - *start_pos);
    splitted_string->substrings[*offset].size =
        ovector[0] - *start_pos;

    *start_pos = ovector[1];

    *offset += 1;

cleanup_iterate_string_to_split:
    pcre2_match_data_free (match_data);
}

char *
mg_util_regex_match_1 (MgUtilRegex *self,
        char *re_str, char *subject) {
    pcre2_code *re;    
    pcre2_match_data *match_data;

    char *return_value;
    int regex_compile_error;
    int rc;
    size_t len_match = 0;

    return_value = NULL;
    PCRE2_SIZE error_offset;
    re = pcre2_compile ((PCRE2_SPTR8) re_str, strlen (re_str), 0,
            &regex_compile_error, &error_offset, NULL);
    match_data = pcre2_match_data_create_from_pattern (re, NULL);
    if (!subject) {
        goto cleanup_match;
    }
    rc = pcre2_match (re, (PCRE2_SPTR8) subject, strlen (subject),
            0, 0, match_data, NULL);
    if (rc < 0 ) {
        goto cleanup_match;
    }
    pcre2_substring_get_bynumber (match_data, 1, (PCRE2_UCHAR8**)
            &return_value, &len_match);
cleanup_match:
    pcre2_match_data_free (match_data);
    pcre2_code_free (re);
    return return_value;
}
