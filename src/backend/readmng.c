#include <gtk/gtk.h>
#include <json-glib/json-glib.h>

#include <libxml/HTMLparser.h>

#ifndef PCRE2_CODE_UNIT_WIDTH
#define PCRE2_CODE_UNIT_WIDTH 8
#include <pcre2.h>
#endif

#include <openmg/backend/readmng.h>

#include <openmg/util/soup.h>
#include <openmg/util/regex.h>
#include <openmg/util/xml.h>
#include <openmg/util/string.h>

#include <openmg/manga.h>
#include <openmg/chapter.h>

typedef enum {
    MG_BACKEND_READMNG_BASE_URL = 1,
    MG_BACKEND_READMNG_N_PROPERTIES
} MgBackendReadmngProperties;

struct _MgBackendReadmng {
    GObject parent_instance;
    char *base_url;
    size_t main_page_html_len;
    char *main_page_html;
    MgUtilXML *xml_utils;
    GListStore *(*get_featured_manga) ();
};

G_DEFINE_TYPE (MgBackendReadmng, mg_backend_readmng, G_TYPE_OBJECT)

static GParamSpec *mg_backend_readmng_properties[MG_BACKEND_READMNG_N_PROPERTIES] = { NULL, };

static void
mg_backend_readmng_dispose (GObject *object);

static void
mg_backend_readmng_class_init (MgBackendReadmngClass *class) {
    GObjectClass *object_class = G_OBJECT_CLASS (class);
    object_class->set_property = mg_backend_readmng_set_property;
    object_class->get_property = mg_backend_readmng_get_property;
    object_class->dispose = mg_backend_readmng_dispose;

    mg_backend_readmng_properties[MG_BACKEND_READMNG_BASE_URL] = g_param_spec_string ("base_url",
            "BaseURL",
            "Url of the backend.",
            NULL,
            G_PARAM_READWRITE);

    g_object_class_install_properties (object_class,
            MG_BACKEND_READMNG_N_PROPERTIES,
            mg_backend_readmng_properties);
}

static char *
mg_backend_readmng_fetch_search (MgBackendReadmng *self,
        const char *search_query, size_t *response_len);
static GListModel *
mg_backend_readmng_parse_page (MgBackendReadmng *self,
        xmlDocPtr html_document);
static xmlDocPtr
mg_backend_readmng_fetch_page_url (MgBackendReadmng *self,
        MgMangaChapter *chapter);
static GListStore *
mg_backend_readmng_recover_chapter_list (MgBackendReadmng *self,
        xmlDocPtr html_document_details, MgManga *manga);
static xmlDocPtr
mg_backend_readmng_fetch_xml_details (MgBackendReadmng *self,
        MgManga *manga);
static const char *
mg_backend_readmng_get_main_page (MgBackendReadmng *self, size_t *len);
static GListStore *
mg_backend_readmng_parse_main_page (MgBackendReadmng *self, const xmlDocPtr html_document);
static xmlDocPtr
mg_backend_readmng_fetch_xml_main_page (MgBackendReadmng *self);
static char *
mg_backend_readmng_get_id_manga_link_from_string (MgBackendReadmng *self, const char *url);
static char *
mg_backend_readmng_get_manga_id_main_page (MgBackendReadmng *self,
        xmlDocPtr html_document, xmlNodePtr manga_slider_card);
static char *
mg_backend_readmng_get_manga_title_main_page (MgBackendReadmng *self,
        xmlDocPtr html_document, xmlNodePtr manga_slider_card); 
static char *
mg_backend_readmng_get_manga_image_main_page (MgBackendReadmng *self,
        xmlDocPtr html_document, xmlNodePtr manga_slider_card); 
static MgManga *
mg_backend_readmng_extract_from_manga_slider_card (MgBackendReadmng *self,
        xmlDocPtr html_document, xmlNodePtr node);
static MgMangaChapter *
mg_backend_readmng_get_data_from_check_box_card (MgBackendReadmng *self,
		xmlDocPtr html_document, xmlNodePtr check_box_card, MgManga *manga);

MgBackendReadmng *
mg_backend_readmng_new(void) {
    return (MG_BACKEND_READMNG) (g_object_new (MG_TYPE_BACKEND_READMNG, NULL));
}

static void
mg_backend_readmng_init (MgBackendReadmng *self) {
    if (!self->base_url) {
        self->base_url = "https://www.readmng.com";
    }
    self->xml_utils = mg_util_xml_new ();
}
static void
mg_backend_readmng_dispose (GObject *object) {
    MgBackendReadmng *self = MG_BACKEND_READMNG (object);
    g_clear_object (&self->xml_utils);
    if (self->main_page_html) {
        g_free (self->main_page_html);
        self->main_page_html = NULL;
    }
    G_OBJECT_CLASS (mg_backend_readmng_parent_class)->dispose (object);
}

char *
mg_backend_readmng_get_base_url (MgBackendReadmng *self) {
    GValue value = G_VALUE_INIT;
    g_value_init (&value, G_TYPE_STRING);
    g_object_get_property (G_OBJECT (self),
            "base_url",
            &value);
    return g_value_dup_string (&value);
}

void
mg_backend_readmng_set_property (GObject *object,
        guint property_id,
        const GValue *value,
        GParamSpec *pspec) {
    MgBackendReadmng *self = MG_BACKEND_READMNG (object);
    switch ((MgBackendReadmngProperties) property_id) {
        case MG_BACKEND_READMNG_BASE_URL:
            g_free (self->base_url);
            self->base_url = g_value_dup_string (value);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
            break;
    }
}

void
mg_backend_readmng_get_property (GObject *object,
        guint property_id,
        GValue *value,
        GParamSpec *pspec) {
    MgBackendReadmng *self = MG_BACKEND_READMNG (object);
    switch ((MgBackendReadmngProperties) property_id) {
        case MG_BACKEND_READMNG_BASE_URL:
            g_value_set_string (value, self->base_url);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
            break;
    }
}

GListModel *
mg_backend_readmng_get_chapter_images (MgBackendReadmng *self, MgMangaChapter *chapter) {
    GListModel *images;
    xmlDocPtr html_document;
    html_document = mg_backend_readmng_fetch_page_url (self, chapter);
    images = mg_backend_readmng_parse_page (self, html_document);

    xmlFreeDoc (html_document);
    return images;
}

static GListModel *
mg_backend_readmng_parse_page (MgBackendReadmng *self,
        xmlDocPtr html_document) {
    GListModel *images             = G_LIST_MODEL (gtk_string_list_new (NULL));
    MgUtilXML *xml_utils           = self->xml_utils;
	MgUtilRegex *regex_util        = mg_util_regex_new ();
    xmlNodeSetPtr node_set         = NULL;
	xmlXPathObjectPtr xpath_result = NULL;
	xmlNodePtr script              = NULL;
	JsonParser *parser             = json_parser_new ();
	JsonNode *root                 = NULL;
	JsonObject *root_object        = NULL;
	JsonArray *sources             = NULL;
	JsonArray *images_json_object  = NULL;
	JsonObject *source             = NULL;
	guint sources_len;
    GError *error                  = NULL;
	char *ts_reader_run            = NULL;
	char *ts_reader_run_json       = NULL;
    xpath_result = mg_util_xml_get_nodes_xpath_expression (xml_utils,
            html_document, NULL, "//script[contains(., 'ts_reader')]");
	if (!xpath_result) {
		fprintf(stderr, "No match for images.\n");
	}
	node_set = xpath_result->nodesetval;
	if (!node_set) {
		fprintf(stderr, "No match for images.\n");
        goto cleanup_mg_backend_readmng_parse_page;
	}
	script = node_set->nodeTab[0];
	ts_reader_run = (char *)xmlNodeGetContent (script);
	ts_reader_run_json = mg_util_regex_match_1 (regex_util,
			"^\\s+ts_reader\\.run\\(((?:.|\\r|\\n)+)\\);", ts_reader_run);
	json_parser_load_from_data (parser, ts_reader_run_json, -1,
			&error);
	if (error) {
        g_warning ("Unable to parse json: %s.", error->message);
        g_clear_error (&error);
        goto cleanup_mg_backend_readmng_parse_page;
	}
	root = json_parser_get_root (parser); 
	if (json_node_get_node_type (root) != JSON_NODE_OBJECT) {
		fprintf(stderr, "Expected object as JSON root.\n");	
		goto cleanup_mg_backend_readmng_parse_page;
	}
	root_object  = json_node_get_object (root);
	sources = json_object_get_array_member (root_object, "sources");
	if (!sources) {
		fprintf(stderr, "No source in JSON.\n");	
		goto cleanup_mg_backend_readmng_parse_page;
	}
	sources_len = json_array_get_length (sources);
	if (!sources_len) {
		fprintf(stderr, "No source element in JSON.\n");
		goto cleanup_mg_backend_readmng_parse_page;
	}
	source = json_array_get_object_element (sources, 0);
	images_json_object = json_object_get_array_member (source, "images");
	if (!images_json_object) {
		fprintf(stderr, "No images in JSON.\n");
		goto cleanup_mg_backend_readmng_parse_page;
	}
	for (int i = 0; i < json_array_get_length(images_json_object); i++) {
		gtk_string_list_append (GTK_STRING_LIST (images), 
				json_array_get_string_element (images_json_object, i));
	}

cleanup_mg_backend_readmng_parse_page:
	if (ts_reader_run) {
		g_free (ts_reader_run);
	}
	if (ts_reader_run_json) {
		pcre2_substring_free ((PCRE2_UCHAR8 *)ts_reader_run_json);
	}
	if (xpath_result) {
		xmlXPathFreeObject(xpath_result);
	}
	if (parser) {
		g_clear_object (&parser);
	}
    return images;
}


static MgManga *
mg_backend_readmng_extract_from_manga_slider_card (MgBackendReadmng *self,
        xmlDocPtr html_document, xmlNodePtr node) {
    MgManga *manga = NULL;
    char *image = NULL;
    char *title = NULL;
    char *id    = NULL;
    image = mg_backend_readmng_get_manga_image_main_page (self, html_document,
            node);
    title = mg_backend_readmng_get_manga_title_main_page (self, html_document,
            node);
    id    = mg_backend_readmng_get_manga_id_main_page (self, html_document,
            node);
    if (!image) {
        fprintf (stderr, "Failed to find image\n");
        goto cleanup_mg_backend_readmng_extract_from_manga_slider_card;
    }
    if (!title) {
        fprintf (stderr, "Failed to find title\n");
        goto cleanup_mg_backend_readmng_extract_from_manga_slider_card;
    }
    if (!id) {
        fprintf (stderr, "Failed to find id\n");
        goto cleanup_mg_backend_readmng_extract_from_manga_slider_card;
    }

    manga = mg_manga_new (image, title, id);
cleanup_mg_backend_readmng_extract_from_manga_slider_card:
    if (image) {
        g_free (image);
    }
    if (title) {
        g_free (title);
    }
    if (id) {
        pcre2_substring_free ((PCRE2_UCHAR8 *)id);
    }
    return manga;
}

static char *
mg_backend_readmng_get_manga_id_main_page (MgBackendReadmng *self,
        xmlDocPtr html_document, xmlNodePtr manga_slider_card) {
    MgUtilXML *xml_utils            = self->xml_utils;
    MgUtilRegex *regex_util         = mg_util_regex_new ();
    xmlXPathObjectPtr xpath_result  = NULL;
    char *id                        = NULL;
    char *new_id                    = NULL;
    xmlNodeSetPtr node_set          = NULL;
    xpath_result = mg_util_xml_get_nodes_xpath_expression (xml_utils, html_document,
        manga_slider_card, "./a");
    if (!xpath_result) {
        fprintf (stderr, "No matching id.\n");
        goto cleanup_mg_backend_readmng_get_manga_id_main_page;
    }
    node_set = xpath_result->nodesetval;
    if (!node_set) {
        fprintf (stderr, "No matching id node set.\n");
        goto cleanup_mg_backend_readmng_get_manga_id_main_page;
    }
    xmlNodePtr a = node_set->nodeTab[0];
    id = mg_util_xml_get_attr (xml_utils, a, "href");
    if (id) {
        new_id = mg_util_regex_match_1 (regex_util, "^/([^/]+)", id);
        g_free (id);
        id = new_id;
    }

cleanup_mg_backend_readmng_get_manga_id_main_page:
    if (xpath_result) {
        xmlXPathFreeObject (xpath_result);
    }
    g_clear_object (&regex_util);
    return id;
} 

static char *
mg_backend_readmng_get_manga_title_main_page (MgBackendReadmng *self,
        xmlDocPtr html_document, xmlNodePtr manga_slider_card) {
    MgUtilXML *xml_utils            = self->xml_utils;
    xmlXPathObjectPtr xpath_result = NULL;
    char *title                     = NULL;
    xmlNodeSetPtr node_set          = NULL;
    xpath_result = mg_util_xml_get_nodes_xpath_expression (xml_utils, html_document,
        manga_slider_card, ".//div[@class='postDetail']//h2");
    if (!xpath_result) {
        fprintf (stderr, "No matching title.\n");
        goto cleanup_mg_backend_readmng_get_manga_title_main_page;
    }
    node_set = xpath_result->nodesetval;
    if (!node_set) {
        fprintf (stderr, "No matching title node set.\n");
        goto cleanup_mg_backend_readmng_get_manga_title_main_page;
    }
    xmlNodePtr h2 = node_set->nodeTab[0];
    title = (char *)xmlNodeGetContent (h2);
cleanup_mg_backend_readmng_get_manga_title_main_page:
    if (xpath_result) {
        xmlXPathFreeObject (xpath_result);
    }
    return title;
}

static char *
mg_backend_readmng_get_manga_image_main_page (MgBackendReadmng *self,
        xmlDocPtr html_document, xmlNodePtr manga_slider_card) {
    MgUtilXML *xml_utils           = self->xml_utils;
    xmlXPathObjectPtr xpath_result = NULL;
    char *image                    = NULL;
    xmlNodeSetPtr node_set         = NULL;
    xpath_result = mg_util_xml_get_nodes_xpath_expression (xml_utils, html_document,
            manga_slider_card, ".//div[@class='sliderImg']//img");
    if (!xpath_result) {
        fprintf (stderr, "No matching image.\n");
        goto cleanup_mg_backend_readmng_get_manga_image_main_page;
    }
    node_set = xpath_result->nodesetval;
    if (!node_set) {
        fprintf (stderr, "No matching image node set.\n");
        goto cleanup_mg_backend_readmng_get_manga_image_main_page;
    }
    xmlNodePtr img = node_set->nodeTab[0];
    image = mg_util_xml_get_attr (xml_utils, img, "src");
    
cleanup_mg_backend_readmng_get_manga_image_main_page:
    if (xpath_result) {
        xmlXPathFreeObject (xpath_result);
    }
    return image;
}

static xmlDocPtr
mg_backend_readmng_fetch_page_url (MgBackendReadmng *self,
        MgMangaChapter *chapter) {
    size_t response_len = 0;
    MgUtilSoup *util_soup = mg_util_soup_new ();
    char *url = mg_manga_chapter_get_url (chapter);
    size_t concat_all_pages_len;
    char *concated_url;

    concat_all_pages_len = snprintf (NULL, 0, "%s/all-pages", url);
    concated_url = g_malloc 
        (sizeof *concated_url * (concat_all_pages_len + 1));
    snprintf (concated_url, concat_all_pages_len + 1,
            "%s/all-pages", url);

    char *html_response = mg_util_soup_get_request (util_soup, concated_url,
            &response_len);

    g_clear_object (&util_soup);
    g_free (url);
    g_free (concated_url);
    xmlDocPtr document = htmlReadMemory (html_response, response_len,
            NULL, NULL,
            HTML_PARSE_RECOVER | HTML_PARSE_NODEFDTD
            | HTML_PARSE_NOERROR | HTML_PARSE_NOWARNING );
    g_free (html_response);
    return document;
}

GListStore *
mg_backend_readmng_search (MgBackendReadmng *self,
        const char *search_query) {
    size_t response_len = 0;
    char *response = mg_backend_readmng_fetch_search (self, search_query,
            &response_len);
    JsonParser *parser           = json_parser_new ();
    GListStore *mangas           = g_list_store_new (MG_TYPE_MANGA);
    GError *error                = NULL;
    JsonNode *root               = NULL;
    JsonArray *mangas_json_array = NULL;
	JsonObject *root_object      = NULL;
    guint mangas_json_array_len  = 0;

    if (!response) {
        g_warning ("Json search response is null.");
        goto cleanup_mg_backend_readmng_search;
    }
    json_parser_load_from_data (parser, response, response_len, &error);
    if (error) {
        g_warning ("Unable to parse json: %s.", error->message);
        g_clear_error (&error);
        goto cleanup_mg_backend_readmng_search;
    }
    root = json_parser_get_root (parser); 
    if (json_node_get_node_type (root) != JSON_NODE_OBJECT) {
        goto cleanup_mg_backend_readmng_search;
    }
	root_object       = json_node_get_object (root);
	mangas_json_array = json_object_get_array_member (root_object, "manga");
    mangas_json_array_len = json_array_get_length (mangas_json_array);
    for (guint i = 0; i < mangas_json_array_len && i < 5; i++) {
        JsonObject *manga_json_object =
            json_array_get_object_element (mangas_json_array, i);
        char *id_manga = NULL;
        const char *url = json_object_get_string_member
            (manga_json_object, "url");
        const char *title = json_object_get_string_member
            (manga_json_object, "title");
        const char *image = json_object_get_string_member
            (manga_json_object, "image");

        id_manga = mg_backend_readmng_get_id_manga_link_from_string (self, url);
        g_list_store_append (mangas, mg_manga_new (image, title, id_manga));

        pcre2_substring_free ((PCRE2_UCHAR8 *) id_manga);
    }
cleanup_mg_backend_readmng_search:
    g_clear_object (&parser);
    g_free (response);
    response = NULL;
    return mangas;
}

static char *
mg_backend_readmng_fetch_search (MgBackendReadmng *self,
        const char *search_query, size_t *response_len) {
    MgUtilSoup *util_soup;
    MgUtilString *string_util;

    char *request_url;

    util_soup = mg_util_soup_new (); 
    string_util = mg_util_string_new ();
    g_asprintf ( &request_url, "%s/%s/", self->base_url, "search/live");

    SoupParam headers[] = {
        {
            .key = "Accept",
            .value = "application/json, text/javascript, */*; q=0.01"
        },
        {
            .key = "Content-Type",
            .value = "application/x-www-form-urlencoded; charset=UTF-8"
        },
        {
            .key = "X-Requested-With",
            .value = "XMLHttpRequest"
        }
    };

    char *phrase = g_malloc (strlen (search_query) + 1);
    snprintf ( phrase, strlen (search_query) + 1, "%s", search_query);

    SoupParam body[] = {
        {
            .key = "dataType",
            .value = "json"
        },
        {
            .key = "phrase",
            .value = phrase 
        }
    };

    size_t headers_len = sizeof headers / sizeof *headers;
    size_t body_len = sizeof body / sizeof *body;

    char *text_response = mg_util_soup_post_request_url_encoded (util_soup,
            request_url, body, body_len, headers, headers_len, response_len);

    g_free (request_url);
    g_free (phrase);
    request_url = NULL;
    g_clear_object (&util_soup);
    g_clear_object (&string_util);

    return text_response;
}


GListStore *
mg_backend_readmng_get_featured_manga (MgBackendReadmng *self) {
    GListStore *mangas;
    xmlDocPtr html_document;
    html_document = mg_backend_readmng_fetch_xml_main_page (self);
    if (!html_document) {
        return NULL;
    }
    mangas = mg_backend_readmng_parse_main_page (self, html_document);

    xmlFreeDoc (html_document);
    return mangas;

}

void
mg_backend_readmng_retrieve_manga_details (MgBackendReadmng *self,
        MgManga *manga) {
    MgUtilXML *xml_utils;

    xmlDocPtr html_document = NULL;
    xmlNodePtr *movie_detail = NULL;
    xmlXPathObjectPtr xpath_result = NULL;
    xmlNodeSetPtr node_set = NULL;
    GListStore *manga_chapters = NULL;

    size_t movie_detail_len = 0;

    if (mg_manga_has_details (manga)) {
        goto cleanup_mg_backend_readmng_retrieve_manga_details;
    }

    xml_utils = self->xml_utils;
    html_document = mg_backend_readmng_fetch_xml_details (self,
            manga);
    xpath_result = mg_util_xml_get_nodes_xpath_expression (xml_utils,
            html_document, NULL, "//div[@class='infox']//div[@class='wd-full'][2]");
    node_set = xpath_result->nodesetval;
    if (!node_set) {
        fprintf(stderr, "No match\n");
        goto cleanup_mg_backend_readmng_retrieve_manga_details;
    }
	xmlNodePtr description_node = node_set->nodeTab[0];
	char *description = (char *) xmlNodeGetContent (description_node);
	mg_manga_set_description (manga, description);
	g_free (description);
    manga_chapters = mg_backend_readmng_recover_chapter_list (self, html_document, manga);
    mg_manga_set_chapter_list (manga, manga_chapters);
    mg_manga_details_recovered (manga);
cleanup_mg_backend_readmng_retrieve_manga_details:
    for (size_t i = 0; i < movie_detail_len; i++) {
        xmlFreeNode (movie_detail[i]);
    }
    if (xpath_result) {
        xmlXPathFreeObject(xpath_result);
    }
    if (movie_detail) {
        g_free (movie_detail);
    }
    if (html_document) {
        xmlFreeDoc(html_document);
    }
}

static GListStore *
mg_backend_readmng_recover_chapter_list (MgBackendReadmng *self,
        xmlDocPtr html_document_details, MgManga *manga) {
    MgUtilXML *xml_utils           = self->xml_utils;
    xmlXPathObjectPtr xpath_result = NULL;
    xmlNodeSetPtr node_set         = NULL;
    GListStore *return_value       = g_list_store_new (
            MG_TYPE_MANGA_CHAPTER);

    xpath_result = mg_util_xml_get_nodes_xpath_expression (xml_utils,
            html_document_details, NULL, "//div[@class='checkBoxCard']");
    node_set = xpath_result->nodesetval;

    if (!node_set) {
        fprintf(stderr, "No matching chapter\n");
        goto cleanup_mg_backend_readmng_recover_chapter_list;
    }

    for (int i = 0; i < node_set->nodeNr; i++) {
        xmlNodePtr check_box_card = node_set->nodeTab[i];
		MgMangaChapter *chapter = mg_backend_readmng_get_data_from_check_box_card
				(self, html_document_details, check_box_card, manga);
		if (chapter) {
			g_list_store_append (return_value, chapter);
		}
    }
cleanup_mg_backend_readmng_recover_chapter_list:
    if (xpath_result) {
        xmlXPathFreeObject(xpath_result);
    }
    return return_value;
}

static MgMangaChapter *
mg_backend_readmng_get_data_from_check_box_card (MgBackendReadmng *self,
		xmlDocPtr html_document, xmlNodePtr check_box_card, MgManga *manga) {
	xmlXPathObjectPtr xpath_result = NULL;
	xmlNodeSetPtr node_set         = NULL;
	MgMangaChapter *chapter        = NULL;
	MgUtilXML *xml_utils           = self->xml_utils;
	char *chapter_id               = NULL;
	char *title                    = NULL;
    char *url                      = NULL;
    char *manga_id                 = mg_manga_get_id (manga);
	xpath_result = mg_util_xml_get_nodes_xpath_expression (xml_utils,
			html_document, check_box_card, ".//label[@data-chapter-id]");
	if (!xpath_result) {
		fprintf(stderr, "Unable to parse chapter, xpath failed.\n");
		goto cleanup_mg_backend_readmng_get_data_from_check_box_card;
	}
	node_set = xpath_result->nodesetval;
	if (!node_set) {
		fprintf(stderr, "Unable to parse chapter, no nodeset.\n");
		goto cleanup_mg_backend_readmng_get_data_from_check_box_card;
	}
	xmlNodePtr chapter_node = node_set->nodeTab[0];
	chapter_id = mg_util_xml_get_attr (xml_utils, chapter_node, "data-chapter-id");
	
	g_asprintf (&title, "Chapter %s", chapter_id);
	g_asprintf (&url, "%s/%s/%s", self->base_url, manga_id, chapter_id);
	chapter = mg_manga_chapter_new (title, "", url);
	
cleanup_mg_backend_readmng_get_data_from_check_box_card:
	if (xpath_result) {
		xmlXPathFreeObject (xpath_result);
	}
	if (chapter_id) {
		g_free (chapter_id);
	}
	return chapter;
}

static xmlDocPtr
mg_backend_readmng_fetch_xml_details (MgBackendReadmng *self,
        MgManga *manga) {
    MgUtilSoup *util_soup;
    MgUtilString *string_util;

    char *request_url;
    char *manga_id;

    size_t response_len = 0;

    util_soup = mg_util_soup_new (); 
    string_util = mg_util_string_new ();
    manga_id = mg_manga_get_id (manga);
    g_asprintf ( &request_url, "%s/%s", self->base_url, manga_id);
    g_free (manga_id);

    char *html_response = mg_util_soup_get_request (util_soup,
            request_url, &response_len);
    g_free (request_url);
    request_url = NULL;
    g_clear_object (&util_soup);
    g_clear_object (&string_util);
    xmlDocPtr document = htmlReadMemory (html_response, response_len, NULL, NULL,
            HTML_PARSE_RECOVER | HTML_PARSE_NODEFDTD
            | HTML_PARSE_NOERROR | HTML_PARSE_NOWARNING );
    g_free (html_response);
    return document;
}

static xmlDocPtr
mg_backend_readmng_fetch_xml_main_page (MgBackendReadmng *self) {
    size_t size_response_text = 0;
    const char *html_response = mg_backend_readmng_get_main_page (self, &size_response_text);

    xmlDocPtr document = htmlReadMemory (html_response,
            size_response_text,
            NULL,
            NULL,
            HTML_PARSE_RECOVER | HTML_PARSE_NODEFDTD 
            | HTML_PARSE_NOERROR | HTML_PARSE_NOWARNING
            );
    return document;
}

static const char *
mg_backend_readmng_get_main_page (MgBackendReadmng *self, size_t *len) {
    if (!self->main_page_html) {
        MgUtilSoup *util_soup = mg_util_soup_new ();
        self->main_page_html = mg_util_soup_get_request (util_soup,
                self->base_url, &self->main_page_html_len);
        g_clear_object (&util_soup);
    }
    if (len) {
        *len = self->main_page_html_len;
    }
    return self->main_page_html;
}

static GListStore *
mg_backend_readmng_parse_main_page (MgBackendReadmng *self, const xmlDocPtr html_document) {
    GListStore *mangas = g_list_store_new (MG_TYPE_MANGA);

    MgUtilXML *xml_utils = self->xml_utils;
    xmlXPathObjectPtr xpath_result = NULL;
    xmlNodeSetPtr node_set = NULL;
    xpath_result = mg_util_xml_get_nodes_xpath_expression (xml_utils,
            html_document, NULL, "//div[@class='mangaSliderCard']");
    node_set = xpath_result->nodesetval;
    if (!node_set) {
        fprintf(stderr, "No match for mangas.\n");
        goto cleanup_mg_backend_readmng_parse_main_page;
    }
    for (int i = 0; i < node_set->nodeNr; i++) {
        xmlNodePtr node = node_set->nodeTab[i];
        MgManga *manga = mg_backend_readmng_extract_from_manga_slider_card (self,
                html_document, node);
        if (!manga) {
            continue;
        }
        g_list_store_append (mangas, manga); 
    }
cleanup_mg_backend_readmng_parse_main_page:
    if (xpath_result) {
        xmlXPathFreeObject (xpath_result);
    }
    return mangas;
}

static char *
mg_backend_readmng_get_id_manga_link_from_string (MgBackendReadmng *self, const char *url) {
    MgUtilRegex *regex_util = mg_util_regex_new ();
    char *re_str = "/([^/]+)";
    char *result = mg_util_regex_match_1 (regex_util, re_str, url);
    g_clear_object (&regex_util);
    return result;
}
