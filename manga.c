#include <libsoup/soup.h>
#ifndef PCRE2_CODE_UNIT_WIDTH
#define PCRE2_CODE_UNIT_WIDTH 8
#include <pcre2.h>
#endif

#include <manga.h>

// TODO: Split this file and delete it.
static void
iterate_string_to_split(struct SplittedString *splitted_string,
        pcre2_code *re, int *will_break, const char *subject,
        size_t subject_size, size_t *start_pos, size_t *offset);

void
copy_substring(const char *origin, char *dest, size_t dest_len, size_t start,
        size_t len) {
    size_t copying_offset = 0;
    while (copying_offset < len) {
        if (!(start+copying_offset <=dest_len)) {
            fprintf(stderr, "Read attempt out of bounds.%ld %ld %ld\n", dest_len, start, len);
            break;
        }
        dest[copying_offset] = origin[start+copying_offset];
        copying_offset++;
    }
    dest[len] = '\0';
}
struct SplittedString *
split(char *re_str, size_t re_str_size, const char *subject, size_t subject_size) {
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
        iterate_string_to_split(splitted_string, re, &will_break,
                subject, subject_size, &start_pos, &offset);
        if (will_break) {
            break;
        }
    }

    pcre2_code_free (re);
    re = NULL;

    return splitted_string;
}

char *
alloc_string(size_t len) {
    char * return_value = NULL;
    return g_malloc (len + 1 * sizeof *return_value);
}

void
splitted_string_free (struct SplittedString *splitted_string) {
    for (int i = 0; i<splitted_string->n_strings; i++) {
        g_free (splitted_string->substrings[i].content);
    }

    g_free (splitted_string->substrings);
    g_free (splitted_string);
}

static void
iterate_string_to_split(struct SplittedString *splitted_string, pcre2_code *re, int *will_break, const char *subject,
        size_t subject_size, size_t *start_pos, size_t *offset) {
    pcre2_match_data_8 *match_data;
    PCRE2_SIZE *ovector;
    int rc;

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
        current_substring->content = alloc_string (subject_size 
                - *start_pos);
        copy_substring (subject, current_substring->content,
                subject_size,
                *start_pos,
                subject_size - *start_pos);
        current_substring->size = subject_size - *start_pos;

        *will_break = 1;
        goto cleanup_iterate_string_to_split;
    }
    ovector = pcre2_get_ovector_pointer_8(match_data);
    splitted_string->substrings[*offset].content = alloc_string (
            ovector[0] - *start_pos);
    copy_substring (subject, splitted_string->substrings[*offset]
            .content,
            subject_size,
            *start_pos,
            ovector[0] - *start_pos);
    splitted_string->substrings[*offset].size =
        ovector[0] - *start_pos;

    *start_pos = ovector[1];

    *offset += 1;

cleanup_iterate_string_to_split:
    pcre2_match_data_free (match_data);
}

char *
match_1 (char *re_str, char *subject) {
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
