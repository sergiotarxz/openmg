#include <stdio.h>
#include <libsoup/soup.h>
#include <manga.h>
#include <libxml/HTMLparser.h>
#include <libxml/xpath.h>

const char *mangafox_url =
	"https://mangafox.fun";

struct Manga *
parse_main_mangafox_page (
		const xmlDocPtr html_document,
		const size_t *size);
xmlXPathObjectPtr
get_nodes_xpath_expression (
		const xmlDocPtr document,
		char *xpath);
char *
get_request (const char *url, gsize *size_response_text);
void
retrieve_mangafox_title () {
	xmlDocPtr html_response;
	gsize *size_response_text = malloc (sizeof (gsize));
	char *response_text = get_request (mangafox_url,
			size_response_text);
	html_response = htmlReadMemory (response_text,
			*size_response_text,
			NULL,
			NULL,
			HTML_PARSE_RECOVER | HTML_PARSE_NODEFDTD 
			| HTML_PARSE_NOERROR | HTML_PARSE_NOWARNING
			);
	size_t manga_size;
	parse_main_mangafox_page (html_response, &manga_size);
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
			if (!xmlStrcmp(attrs->name, (const xmlChar *)"class")) {
				if (attrs->children
						&& attrs->children->content) {
					printf("%s\n", (const char *)attrs->children->content);
					break;
				}
			}
		}
		
	}
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
	return result;
}

