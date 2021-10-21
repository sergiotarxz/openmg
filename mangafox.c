#include <stdio.h>

#include <libsoup/soup.h>
#include <libxml/HTMLparser.h>
#include <libxml/xpath.h>

#ifndef PCRE2_CODE_UNIT_WIDTH
#define PCRE2_CODE_UNIT_WIDTH 8
#include <pcre2.h>
#endif

#include <manga.h>

const char *mangafox_url =
	"https://mangafox.fun";
struct String {
	char *content;
	size_t size;
};

struct SplittedString {
	struct String *substrings;
	size_t n_strings;
};

struct Manga *
parse_main_mangafox_page (
		const xmlDocPtr html_document,
		const size_t *size);
xmlXPathObjectPtr
get_nodes_xpath_expression (
		const xmlDocPtr document,
		char *xpath);
char *
alloc_string(size_t len);
void
copy_substring(const char *origin, char *dest, 
		size_t dest_len, size_t start, size_t len);
void
print_classes (const char *class_attribute,
		size_t class_attribute_size);
int
has_class (const char *class_attribute,
		char *class_to_check);
void
splitted_string_free (struct SplittedString *splitted_string);

struct SplittedString *
split(char *re_str, size_t re_str_size, const char *subject,
		size_t subject_size);
void
iterate_string_to_split(struct SplittedString *splitted_string, pcre2_code *re, int *will_break, const char *subject,
		size_t subject_size, size_t *start_pos, size_t *offset);
char *
get_request (const char *url, gsize *size_response_text);

void
retrieve_mangafox_title () {
	xmlDocPtr html_response;
	gsize size_response_text;
	char *response_text = get_request (mangafox_url,
			&size_response_text);
	html_response = htmlReadMemory (response_text,
			size_response_text,
			NULL,
			NULL,
			HTML_PARSE_RECOVER | HTML_PARSE_NODEFDTD 
			| HTML_PARSE_NOERROR | HTML_PARSE_NOWARNING
			);
	size_t manga_size;
	parse_main_mangafox_page (html_response, &manga_size);
	xmlFreeDoc (html_response);
	free (response_text);
}

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

	printf("%u\n", status);
	const char *html_response = g_bytes_get_data ((GBytes *)
			g_value_peek_pointer (&response),
			size_response_text);

	char *return_value = g_strndup (html_response, *size_response_text);

	g_value_unset (&response);
	g_object_unref (soup_session);
	g_object_unref (msg);

	return return_value;
}


struct Manga *
parse_main_mangafox_page (const xmlDocPtr html_document,
		const size_t *size) {
	xmlIndentTreeOutput = 1;
	xmlXPathObjectPtr xpath_result = get_nodes_xpath_expression (html_document,
			"//div[@class]");
	if (!xpath_result) {
		fprintf(stderr, "Empty xpath result\n");
		return NULL;
	}
	xmlNodeSetPtr node_set = xpath_result->nodesetval;
	if (!node_set) {
		fprintf(stderr, "No match\n");
		return NULL;
	}
	for (int i = 0; i < node_set->nodeNr; i++) {
		xmlNodePtr node = node_set->nodeTab[i];
		for (xmlAttr *attrs = node->properties; attrs; attrs=attrs->next) {
			if (!xmlStrcmp(attrs->name, (const xmlChar *)"class") 
					&& attrs->children && attrs->children->content) {
				const char *content = (char *) attrs->children->content;
				if (has_class (content, "manga-slide")) {
					printf("%s\n", content);
				}
			}
		}
	}

	xmlXPathFreeObject (xpath_result);
}

int
has_class (const char *class_attribute,
		char *class_to_check) {
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

void
splitted_string_free (struct SplittedString *splitted_string) {
	for (int i = 0; i<splitted_string->n_strings; i++) {
		g_free (splitted_string->substrings[i].content);
	}

	g_free (splitted_string->substrings);
	g_free (splitted_string);
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

void
iterate_string_to_split(struct SplittedString *splitted_string, pcre2_code *re, int *will_break, const char *subject,
		size_t subject_size, size_t *start_pos, size_t *offset) {
	pcre2_match_data_8 *match_data;
	PCRE2_SIZE *ovector;
	int rc;

	splitted_string->n_strings++;
	match_data = pcre2_match_data_create_from_pattern_8 (re, NULL);
	rc = pcre2_match_8 ( re, (PCRE2_SPTR8) subject, subject_size, *start_pos, 0, match_data,
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
			ovector[0] - *start_pos - 1);
	splitted_string->substrings[*offset].size =
		ovector[0] - *start_pos - 1;

	*start_pos = ovector[1];

	*offset += 1;

cleanup_iterate_string_to_split:
	pcre2_match_data_free (match_data);
}

char *
alloc_string(size_t len) {
	char * return_value;
	return g_malloc (len + 1 * sizeof *return_value);
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

xmlXPathObjectPtr
get_nodes_xpath_expression (const xmlDocPtr document, char *xpath) {
	xmlXPathContextPtr context;
	xmlXPathObjectPtr result;

	context = xmlXPathNewContext (document);
	if (!context) {
		fprintf(stderr, "Error in xmlXpathNewContext\n");
		return NULL;
	}
	result = xmlXPathEvalExpression ((const xmlChar *)xpath, context);

	xmlXPathFreeContext (context);

	return result;
}

