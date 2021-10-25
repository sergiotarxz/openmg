#include <libsoup/soup.h>
#include <libxml/HTMLparser.h>
#include <libxml/xpath.h>
#ifndef PCRE2_CODE_UNIT_WIDTH
#define PCRE2_CODE_UNIT_WIDTH 8
#include <pcre2.h>
#endif

#include <manga.h>

char *
get_request (const char *url, gsize *size_response_text) {
    SoupSession *soup_session;
    SoupMessage *msg;
    GValue response = G_VALUE_INIT;
    guint status;

    *size_response_text = 0;

    g_value_init (&response, G_TYPE_BYTES);

    soup_session = soup_session_new ();
    msg = soup_message_new ("GET", url);
    status = soup_session_send_message (soup_session, msg);
    g_object_get_property(
            G_OBJECT (msg),
            "response-body-data",
            &response);

    const char *html_response = g_bytes_get_data ((GBytes *)
            g_value_peek_pointer (&response),
            size_response_text);

    char *return_value = copy_binary_data(html_response, *size_response_text);

    g_value_unset (&response);
    g_object_unref (soup_session);
    g_object_unref (msg);

    return return_value;
}

char *
copy_binary_data (const char *input, size_t size) {
    char *response = NULL;
    if (size) {
        response = g_realloc(response, sizeof *response * size);
        for (size_t i = 0; i<size; i++) {
            response[i] = input[i];
        }
    }
    return response;
}

xmlNodePtr *
find_class (xmlNodePtr node, char *class, size_t *len, xmlNodePtr *nodes,
        int return_on_first) {
    for (xmlNodePtr child = node->children; child; child=child->next) {
        char *attr = get_attr (child, "class");
        if (attr && has_class (attr, class)) {
                (*len)++;
                nodes = g_realloc (nodes, sizeof *nodes * *len);
                nodes[*len-1] = child;
                if (return_on_first) {
                    return nodes;
                }
        }
        if (node->children) {
            xmlNodePtr child = node->children;
            for (;child;child=child->next) {
                nodes = find_class (child, class, len, nodes,
                        return_on_first);
                if (*len) {
                    return nodes;
                }
            }
        }
    }
    return nodes;
}

void
print_debug_nodes (const xmlDocPtr html_document,
        xmlNodePtr *nodes, size_t nodes_len) {
    xmlBufferPtr buffer = xmlBufferCreate ();
    for (int i = 0; i < nodes_len; i++) {
        xmlNodeDump (buffer, html_document, nodes[i],
                0, 1);
    }
    xmlBufferDump (stdout, buffer);
    xmlBufferFree (buffer);
}

char *
get_attr (xmlNodePtr const node, const char *attr_name) {
    char *return_value = NULL;
    for (xmlAttr *attr = node->properties; attr; attr=attr->next) {
        if (!xmlStrcmp(attr->name, (const xmlChar *) attr_name) 
                && attr->children && attr->children->content) {
            if (!attr->children->content) continue;
            size_t content_len = strlen((char *) 
                    attr->children->content);
            return_value = alloc_string(content_len);
            copy_substring ((char *) attr->children->content, return_value,
                    content_len,
                    0,
                    content_len);                    
            break;
        }
    }
    return return_value;
}

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

int
has_class (const char *class_attribute,
        const char *class_to_check) {
    char *re = "\\s+";
    struct SplittedString *classes;
    int return_value = 0;
    classes = split(re, strlen(re), class_attribute,
            strlen(class_attribute));
    for (int i = 0; i<classes->n_strings; i++) {
        if (strcmp(classes->substrings[i].content, class_to_check) == 0) {
            return_value = 1;
            goto cleanup_has_class;
        }
    }

cleanup_has_class:
    splitted_string_free (classes);
    return return_value;
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

void
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

xmlXPathObjectPtr
get_nodes_xpath_expression (const xmlDocPtr document, char *xpath) {
    xmlXPathContextPtr context;
    xmlXPathObjectPtr result;

    context = xmlXPathNewContext (document);
    result = xmlXPathEvalExpression ((const xmlChar *)xpath, context);

    xmlXPathFreeContext (context);

    return result;
}

xmlNodePtr *
loop_search_class (const xmlNodePtr node, xmlNodePtr *nodes,
        const char * class, size_t *len) {
    char *content = get_attr (node, "class");
    if (!content) {
        return nodes;
    }
    if (has_class (content, class)) {
        (*len)++;
        nodes = g_realloc (nodes, (sizeof *nodes) * *len);
        nodes[(*len)-1] = xmlCopyNode(node, XML_COPY_NODE_RECURSIVE);
    }
    g_free (content);
    return nodes;
}
