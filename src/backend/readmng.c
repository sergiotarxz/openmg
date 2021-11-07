#include <libxml/HTMLparser.h>

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

GListStore *
mg_backend_readmng_get_featured_manga (MgBackendReadmng *self) {
    GListStore *mangas;
    xmlDocPtr html_document;
    html_document = mg_backend_readmng_fetch_xml_main_page (self);
    mangas = mg_backend_readmng_parse_main_page (self, html_document);

    xmlFreeDoc (html_document);
    return mangas;

}

void
mg_backend_readmng_retrieve_manga_details (MgBackendReadmng *self,
        MgManga *manga) {
    MgUtilXML *xml_utils;

    xmlDocPtr html_document;
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
        mg_manga_set_description (manga,
                (char *) xmlNodeGetContent (movie_detail[0]));
    }
    manga_chapters = mg_backend_readmng_recover_chapter_list (self, html_document);
    mg_manga_set_chapter_list (manga, manga_chapters);
    mg_manga_details_recovered (manga);
cleanup_mg_backend_readmng_retrieve_manga_details:
    g_free (movie_detail);
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
    if (uls) {
        g_free (uls);
    }
    return return_value;
}

static MgMangaChapter *
mg_backend_readmng_loop_li_chapter (
        MgBackendReadmng *self,
        xmlNodePtr li) {
    MgUtilXML *xml_utils = self->xml_utils;
    xmlNodePtr a = mg_backend_readmng_get_a_for_chapter (
            self, li);
    if (!a) return NULL;

    char *url = mg_util_xml_get_attr (xml_utils, a, "href");
    size_t val_len = 0;
    size_t dte_len = 0;

    xmlNodePtr *val = mg_util_xml_find_class (xml_utils, a, "val", &val_len, NULL, 1);
    xmlNodePtr *dte = mg_util_xml_find_class (xml_utils, a, "dte", &dte_len, NULL, 1);
    if (val_len && dte_len) {
        return mg_manga_chapter_new (
                (char *) xmlNodeGetContent (val[0]),
                (char *) xmlNodeGetContent(dte[0]),
                url);
    }
    return NULL;
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

    char *html_response = mg_util_soup_get_request (util_soup,
            request_url, &response_len);
    g_object_unref (util_soup);
    return htmlReadMemory (html_response, response_len, NULL, NULL,
            HTML_PARSE_RECOVER | HTML_PARSE_NODEFDTD
            | HTML_PARSE_NOERROR | HTML_PARSE_NOWARNING );
}

static xmlDocPtr
mg_backend_readmng_fetch_xml_main_page (MgBackendReadmng *self) {
    size_t size_response_text = 0;
    const char *html_response = mg_backend_readmng_get_main_page (self, &size_response_text);

    return htmlReadMemory (html_response,
            size_response_text,
            NULL,
            NULL,
            HTML_PARSE_RECOVER | HTML_PARSE_NODEFDTD 
            | HTML_PARSE_NOERROR | HTML_PARSE_NOWARNING
            );
}

static const char *
mg_backend_readmng_get_main_page (MgBackendReadmng *self, size_t *len) {
    if (!self->main_page_html) {
        MgUtilSoup *util_soup = mg_util_soup_new ();
        self->main_page_html = mg_util_soup_get_request (util_soup,
                self->base_url, &self->main_page_html_len);
        g_object_unref (util_soup);
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

    }
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
    if (xpath_result) {
        xmlXPathFreeObject(xpath_result);
    }
cleanup_mg_backend_readmng_retrieve_slides:
    g_free (nodes);
    return slides;
}

static xmlNodePtr
mg_backend_readmng_retrieve_thumbnail_from_li (MgBackendReadmng *self, xmlNodePtr current_li) {
    size_t thumbnail_len = 0;
    MgUtilXML *xml_utils = self->xml_utils;
    xmlNodePtr *thumbnail = mg_util_xml_find_class (xml_utils, current_li, "thumbnail",
            &thumbnail_len, NULL, 1);
    if (thumbnail_len) return thumbnail[0];
    return NULL;
}

static xmlNodePtr
mg_backend_readmng_retrieve_title_from_li (MgBackendReadmng *self, xmlNodePtr li) {
    size_t title_len = 0;
    MgUtilXML *xml_utils = self->xml_utils;
    xmlNodePtr *title = mg_util_xml_find_class (xml_utils, li, "title", &title_len, NULL, 1);
    if (title_len) return title[0];
    return NULL;
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
mg_backend_get_id_manga_link (MgBackendReadmng *self, xmlNodePtr a) {
    char *re_str = "readmng\\.com/([^/]+)";
    MgUtilXML *xml_utils = self->xml_utils;
    MgUtilRegex *regex_util = mg_util_regex_new ();
    return mg_util_regex_match_1 (regex_util, re_str, mg_util_xml_get_attr (xml_utils, a, "href"));
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
            && a && (id_manga = mg_backend_get_id_manga_link (self, a))) {
        g_list_store_append (mangas,
                mg_manga_new (mg_util_xml_get_attr (xml_utils, img, "src"),
                    (char *)xmlNodeGetContent (title), id_manga));
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
