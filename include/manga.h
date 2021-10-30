#pragma once
#include <libsoup/soup.h>
#include <libxml/HTMLparser.h>
#include <libxml/xpath.h>
#ifndef PCRE2_CODE_UNIT_WIDTH
#define PCRE2_CODE_UNIT_WIDTH 8
#include <pcre2.h>
#endif

#define XML_COPY_NODE_RECURSIVE 2 | 1

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

char *
get_request (const char *url, gsize *size_response_text);
xmlNodePtr *
find_class (xmlNodePtr node, char *class, size_t *len, xmlNodePtr *nodes,
        int return_on_first);
void
print_debug_nodes (const xmlDocPtr html_document,
        xmlNodePtr *nodes, size_t nodes_len);
char *
get_attr (xmlNodePtr const node, const char *attr_name);
void
copy_substring(const char *origin, char *dest, size_t dest_len, size_t start,
        size_t len);
int
has_class (const char *class_attribute,
        const char *class_to_check);
struct SplittedString *
split(char *re_str, size_t re_str_size, const char *subject, size_t subject_size);
char *
alloc_string(size_t len);
void
splitted_string_free (struct SplittedString *splitted_string);
void
iterate_string_to_split(struct SplittedString *splitted_string,
        pcre2_code *re, int *will_break, const char *subject,
        size_t subject_size, size_t *start_pos, size_t *offset);
xmlXPathObjectPtr
get_nodes_xpath_expression (const xmlDocPtr document, char *xpath);
xmlNodePtr *
loop_search_class (const xmlNodePtr node, xmlNodePtr *nodes,
        const char * class, size_t *len);
char *
copy_binary_data (const char *input, size_t size);
char *
match_1 (char *re_str, char *subject);
