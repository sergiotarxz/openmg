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

static xmlNodePtr
mg_backend_readmng_get_a_for_chapter (
        MgBackendReadmng *self,
        xmlNodePtr li);
static MgMangaChapter *
mg_backend_readmng_loop_li_chapter (
        MgBackendReadmng *self,
        xmlNodePtr li);
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
        xmlDocPtr html_document_details);
static xmlDocPtr
mg_backend_readmng_fetch_xml_details (MgBackendReadmng *self,
        MgManga *manga);
static xmlNodePtr
mg_backend_readmng_retrieve_img_from_thumbnail (MgBackendReadmng *self, xmlNodePtr thumbnail);
static xmlNodePtr
mg_backend_readmng_retrieve_ul_slides (MgBackendReadmng *self, xmlNodePtr slides) ;
static void
mg_backend_readmng_extract_manga_info_from_current_li (MgBackendReadmng *self, 
        GListStore *mangas, xmlNodePtr current_li);
static xmlNodePtr *
mg_backend_readmng_retrieve_li_slides (MgBackendReadmng *self, const xmlNodePtr slides, size_t *li_len);
static xmlNodePtr
mg_backend_readmng_retrieve_slides (MgBackendReadmng *self, const xmlDocPtr html_document);
static const char *
mg_backend_readmng_get_main_page (MgBackendReadmng *self, size_t *len);
static GListStore *
mg_backend_readmng_parse_main_page (MgBackendReadmng *self, const xmlDocPtr html_document);
static xmlDocPtr
mg_backend_readmng_fetch_xml_main_page (MgBackendReadmng *self);
static char *
mg_backend_readmng_get_id_manga_link_from_string (MgBackendReadmng *self, const char *url);

MgBackendReadmng *
mg_backend_readmng_new(void) {
    return (MG_BACKEND_READMNG) (g_object_new (MG_TYPE_BACKEND_READMNG, NULL));
}

static void
mg_backend_readmng_init (MgBackendReadmng *self) {
    if (!self->base_url) {
        self->base_url = "https://www.readmng.com/";
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
    GListModel *images = G_LIST_MODEL 
        (gtk_string_list_new (NULL));

    MgUtilXML *xml_utils = self->xml_utils;
    xmlXPathObjectPtr xpath_result = NULL;
    xmlNodeSetPtr node_set = NULL;
    xpath_result = mg_util_xml_get_nodes_xpath_expression (xml_utils,
            html_document, "//img[@class]");
    node_set = xpath_result->nodesetval;
    if (!node_set) {
        fprintf(stderr, "No match for images.\n");
        goto cleanup_mg_backend_readmng_parse_page;
    }
    for (int i = 0; i < node_set->nodeNr; i++) {
        xmlNodePtr node = node_set->nodeTab[i];
        char *image_url = mg_util_xml_get_attr (xml_utils, node, "src");
        gtk_string_list_append (GTK_STRING_LIST (images), image_url);
        g_free (image_url);
    }
cleanup_mg_backend_readmng_parse_page:
    xmlXPathFreeObject(xpath_result);
    return images;
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
    JsonParser *parser = json_parser_new ();
    GListStore *mangas = g_list_store_new (MG_TYPE_MANGA);
    GError *error = NULL;
    JsonNode *root = NULL;
    JsonArray *mangas_json_array = NULL;
    guint mangas_json_array_len = 0;

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
    if (json_node_get_node_type (root) != JSON_NODE_ARRAY) {
        goto cleanup_mg_backend_readmng_search;
    }
    mangas_json_array = json_node_get_array (root);
    mangas_json_array_len = json_array_get_length (
            mangas_json_array);
    for (guint i = 0; i < mangas_json_array_len && i < 19; i++) {
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

    size_t request_url_len;

    util_soup = mg_util_soup_new (); 
    string_util = mg_util_string_new ();
    request_url_len = snprintf ( NULL, 0, "%s/%s/", self->base_url, "service/search");
    request_url = mg_util_string_alloc_string (string_util, request_url_len); 
    snprintf ( request_url, request_url_len+1, "%s/%s/", self->base_url, "service/search");

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
            html_document, "//li[@class]");
    node_set = xpath_result->nodesetval;
    if (!node_set) {
        fprintf(stderr, "No match\n");
        goto cleanup_mg_backend_readmng_retrieve_manga_details;
    }
    for (int i = 0; i < node_set->nodeNr; i++) {
        xmlNodePtr node = node_set->nodeTab[i];
        movie_detail = mg_util_xml_loop_search_class (xml_utils,
                node, movie_detail, "movie-detail", &movie_detail_len);
    } 
    if (movie_detail) {
        char *description = (char *) xmlNodeGetContent (movie_detail[0]);
        mg_manga_set_description (manga, description);
        g_free (description);
    }
    manga_chapters = mg_backend_readmng_recover_chapter_list (self, html_document);
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
        xmlDocPtr html_document_details) {
    MgUtilXML *xml_utils = self->xml_utils;
    xmlXPathObjectPtr xpath_result = NULL;
    xmlNodeSetPtr node_set = NULL;
    xmlNodePtr *uls = NULL;
    xmlNodePtr ul;
    GListStore *return_value = g_list_store_new (
            MG_TYPE_MANGA_CHAPTER);
    size_t ul_len = 0;

    xpath_result = mg_util_xml_get_nodes_xpath_expression (xml_utils,
            html_document_details, "//ul[@class]");
    node_set = xpath_result->nodesetval;
    if (!node_set) {
        fprintf(stderr, "No matching ul\n");
        goto cleanup_mg_backend_readmng_recover_chapter_list;
    }
    for (int i = 0; i < node_set->nodeNr; i++) {
        xmlNodePtr node = node_set->nodeTab[i];
        uls = mg_util_xml_loop_search_class (xml_utils,
                node, uls, "chp_lst", &ul_len);
    }
    if (!ul_len) {
        fprintf(stderr, "No matching chp_lst\n");
        goto cleanup_mg_backend_readmng_recover_chapter_list;
    }
    ul = uls[0];
    for (xmlNodePtr li = ul->children; li; li = li->next) {
        if (!strcmp ((char *) li->name, "li")) {
            MgMangaChapter *chapter = mg_backend_readmng_loop_li_chapter (self, li);
            if (chapter) {
                g_list_store_append (return_value, chapter);
            }
        }
    }
cleanup_mg_backend_readmng_recover_chapter_list:
    if (xpath_result) {
        xmlXPathFreeObject(xpath_result);
    }
    if (uls) {
        for (size_t i = 0; i < ul_len; i++) {
            xmlFreeNode(uls[i]);
        }
        g_free (uls);
    }
    return return_value;
}

static MgMangaChapter *
mg_backend_readmng_loop_li_chapter (
        MgBackendReadmng *self,
        xmlNodePtr li) {
    MgUtilXML *xml_utils = self->xml_utils;
    MgMangaChapter *chapter = NULL;
    xmlNodePtr a = mg_backend_readmng_get_a_for_chapter (
            self, li);
    if (!a) return NULL;

    char *url = mg_util_xml_get_attr (xml_utils, a, "href");
    size_t val_len = 0;
    size_t dte_len = 0;

    xmlNodePtr *val = mg_util_xml_find_class (xml_utils, a, "val", &val_len, NULL, 1);
    xmlNodePtr *dte = mg_util_xml_find_class (xml_utils, a, "dte", &dte_len, NULL, 1);
    if (val_len && dte_len) {
        char *val_str = (char *) xmlNodeGetContent (val[0]);
        char *dte_str = (char *) xmlNodeGetContent (dte[0]);

        chapter = mg_manga_chapter_new (val_str, dte_str, url);

        g_free (val_str);
        g_free (dte_str);
    }
    if (url) {
        g_free (url);
    }
    if (val) {
        g_free (val);
        val = NULL;
    }
    if (dte) {
        g_free (dte);
        dte = NULL;
    }

    return chapter;
}

static xmlNodePtr
mg_backend_readmng_get_a_for_chapter (
        MgBackendReadmng *self,
        xmlNodePtr li) {
    for (xmlNodePtr child = li->children; child; child = child->next) {
        if (!strcmp((char *) child->name, "a")) {
            return child;
        }
    }
    return NULL;
}

static xmlDocPtr
mg_backend_readmng_fetch_xml_details (MgBackendReadmng *self,
        MgManga *manga) {
    MgUtilSoup *util_soup;
    MgUtilString *string_util;

    char *request_url;
    char *manga_id;

    size_t request_url_len;
    size_t response_len = 0;

    util_soup = mg_util_soup_new (); 
    string_util = mg_util_string_new ();
    manga_id = mg_manga_get_id (manga);
    request_url_len = snprintf ( NULL, 0, "%s/%s/", self->base_url, manga_id);
    request_url = mg_util_string_alloc_string (string_util, request_url_len); 
    snprintf ( request_url, request_url_len+1, "%s/%s/", self->base_url, manga_id);
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
    GListStore *mangas = g_list_store_new(MG_TYPE_MANGA);
    xmlNodePtr *li;

    xmlNodePtr slides = mg_backend_readmng_retrieve_slides (self, html_document);

    size_t li_len = 0;
    li = mg_backend_readmng_retrieve_li_slides (self, slides, &li_len);
    for (int i = 0; i<li_len; i++) {
        xmlNodePtr current_li = li[i];
        mg_backend_readmng_extract_manga_info_from_current_li (self, 
                mangas, current_li);
        xmlFreeNode (current_li);
        li[i] = NULL;
    }
    xmlFreeNode(slides);
    g_free (li);
    return mangas;
}

static xmlNodePtr *
mg_backend_readmng_retrieve_li_slides (MgBackendReadmng *self, const xmlNodePtr slides, size_t *li_len) {
    xmlNodePtr ul_slides = mg_backend_readmng_retrieve_ul_slides (self, slides);
    xmlNodePtr *li = NULL;
    for (xmlNodePtr child = ul_slides->children; child; child=child->next) {
        (*li_len)++;
        li = g_realloc(li, sizeof *li * *li_len);
        li[*li_len-1] = xmlCopyNode(child, XML_COPY_NODE_RECURSIVE);
    }
    return li;
}

static xmlNodePtr
mg_backend_readmng_retrieve_ul_slides(MgBackendReadmng *self, xmlNodePtr slides) {
    for (xmlNodePtr child = slides->children; child; child = child->next) {
        if (!strcmp((char *) child->name, "ul")) {
            return child;
        }
    }
    return NULL;
}

static xmlNodePtr
mg_backend_readmng_retrieve_slides (MgBackendReadmng *self, const xmlDocPtr html_document) {
    xmlNodePtr *nodes = NULL;
    xmlXPathObjectPtr xpath_result = NULL;
    MgUtilXML *xml_utils = self->xml_utils;
    xpath_result = mg_util_xml_get_nodes_xpath_expression (xml_utils,
            html_document, "//div[@class]");
    xmlNodePtr slides = NULL;
    xmlNodeSetPtr node_set = NULL;
    size_t matching_classes_len = 0;

    node_set = xpath_result->nodesetval;
    if (!node_set) {
        fprintf(stderr, "No match\n");
        goto cleanup_mg_backend_readmng_retrieve_slides;
    }
    for (int i = 0; i < node_set->nodeNr; i++) {
        xmlNodePtr node = node_set->nodeTab[i];
        nodes = mg_util_xml_loop_search_class (xml_utils, node, nodes,
                "slides", &matching_classes_len);
    }
    if (nodes) {
        slides = nodes[0];
    }
cleanup_mg_backend_readmng_retrieve_slides:
    if (xpath_result) {
        xmlXPathFreeObject(xpath_result);
    }
    if (nodes) {
        for (size_t i = 1; i < matching_classes_len; i++)
        {
            xmlFreeNode(nodes[i]);
        }
        
        g_free (nodes);
    }
    return slides;
}

static xmlNodePtr
mg_backend_readmng_retrieve_thumbnail_from_li (MgBackendReadmng *self, xmlNodePtr current_li) {
    size_t thumbnail_len = 0;
    MgUtilXML *xml_utils = self->xml_utils;
    xmlNodePtr return_value = NULL;
    xmlNodePtr *thumbnail = mg_util_xml_find_class (xml_utils, current_li, "thumbnail",
            &thumbnail_len, NULL, 1);
    if (!thumbnail_len) goto cleanup_mg_backend_retrieve_thumbnail_from_li;
    return_value = thumbnail[0];
cleanup_mg_backend_retrieve_thumbnail_from_li:
    if (thumbnail) {
        g_free (thumbnail);
    }
    return return_value;
}

static xmlNodePtr
mg_backend_readmng_retrieve_title_from_li (MgBackendReadmng *self, xmlNodePtr li) {
    size_t title_len = 0;
    MgUtilXML *xml_utils = self->xml_utils;
    xmlNodePtr return_value = NULL;
    xmlNodePtr *title = mg_util_xml_find_class (xml_utils, li, "title", &title_len, NULL, 1);
    if (title_len) {
        return_value = title[0];
    }
    if (title) {
        g_free (title);
    }
    return return_value;
}

static xmlNodePtr
mg_backend_readmng_find_a_link_chapter (MgBackendReadmng *self,
        xmlNodePtr current_li) {
    for (xmlNodePtr child = current_li->children; child; child = child->next) {
        if (!strcmp((char *)child->name, "a")) {
            return child;
        }
    }
    return NULL;
}

static char *
mg_backend_readmng_get_id_manga_link (MgBackendReadmng *self, xmlNodePtr a) {
    MgUtilXML *xml_utils = self->xml_utils;
    char *href = mg_util_xml_get_attr (xml_utils, a, "href");
    char *result = mg_backend_readmng_get_id_manga_link_from_string (self, href);
    g_free (href);
    return result;
}

static char *
mg_backend_readmng_get_id_manga_link_from_string (MgBackendReadmng *self, const char *url) {
    MgUtilRegex *regex_util = mg_util_regex_new ();
    char *re_str = "readmng\\.com/([^/]+)";
    char *result = mg_util_regex_match_1 (regex_util, re_str, url);
    g_clear_object (&regex_util);
    return result;
}

static void
mg_backend_readmng_extract_manga_info_from_current_li (MgBackendReadmng *self, 
        GListStore *mangas, xmlNodePtr current_li) {

    xmlNodePtr thumbnail = mg_backend_readmng_retrieve_thumbnail_from_li (self, current_li);
    xmlNodePtr title = mg_backend_readmng_retrieve_title_from_li (self, current_li);
    xmlNodePtr a = mg_backend_readmng_find_a_link_chapter (self, current_li);
    xmlNodePtr img;
    MgUtilXML *xml_utils = self->xml_utils;
    char *id_manga = NULL;


    if (thumbnail && title && (img = mg_backend_readmng_retrieve_img_from_thumbnail (self, thumbnail))
            && a && (id_manga = mg_backend_readmng_get_id_manga_link (self, a))) {
        char *src = mg_util_xml_get_attr (xml_utils, img, "src");
        char *title_string = (char *)xmlNodeGetContent (title);
        g_list_store_append (mangas, mg_manga_new (src, title_string, id_manga));

        g_free (src);
        g_free (title_string);
        pcre2_substring_free ((PCRE2_UCHAR8 *) id_manga);
    }
}

static xmlNodePtr
mg_backend_readmng_retrieve_img_from_thumbnail (MgBackendReadmng *self, xmlNodePtr thumbnail) {
    for (xmlNodePtr child = thumbnail->children; child; child=child->next) {
        if (!strcmp((char *)child->name, "img")) {
            return child;
        }
    }
    return NULL;
}
