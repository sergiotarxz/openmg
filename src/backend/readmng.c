#include <libxml/HTMLparser.h>

#include <openmg/backend/readmng.h>
#include <openmg/manga.h>

#include <manga.h>

typedef enum {
    MG_BACKEND_READMNG_BASE_URL = 1,
    MG_BACKEND_READMNG_N_PROPERTIES
} MgBackendReadmngProperties;

struct _MgBackendReadmng {
    GObject parent_instance;
    char *base_url;
    size_t main_page_html_len;
    char *main_page_html;
    MgManga *(*get_featured_manga) ();
};

G_DEFINE_TYPE (MgBackendReadmng, mg_backend_readmng, G_TYPE_OBJECT)

static GParamSpec *mg_backend_readmng_properties[MG_BACKEND_READMNG_N_PROPERTIES] = { NULL, };

static void
mg_backend_readmng_class_init (MgBackendReadmngClass *class) {
    GObjectClass *object_class = G_OBJECT_CLASS (class);
    object_class->set_property = mg_backend_readmng_set_property;
    object_class->get_property = mg_backend_readmng_get_property;

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
mg_backend_readmng_retrieve_img_from_thumbnail (MgBackendReadmng *self, xmlNodePtr thumbnail);
static xmlNodePtr
mg_backend_readmng_retrieve_ul_slides(MgBackendReadmng *self, xmlNodePtr slides) ;
static MgManga **
mg_backend_readmng_extract_manga_info_from_current_li (MgBackendReadmng *self, 
        MgManga **mangas, xmlNodePtr current_li, size_t *len);
static xmlNodePtr *
mg_backend_readmng_retrieve_li_slides (MgBackendReadmng *self, const xmlNodePtr slides, size_t *li_len);
static xmlNodePtr
mg_backend_readmng_retrieve_slides (MgBackendReadmng *self, const xmlDocPtr html_document);
static const char *
mg_backend_readmng_get_main_page (MgBackendReadmng *self, size_t *len);
static MgManga **
mg_backend_readmng_parse_main_page (MgBackendReadmng *self, size_t *len, const xmlDocPtr html_document);
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
}

const char *
mg_backend_readmng_get_base_url (MgBackendReadmng *self) {
    return self->base_url;
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

MgManga **
mg_backend_readmng_get_featured_manga (MgBackendReadmng *self, size_t *len) {
    MgManga **mangas;
    xmlDocPtr html_document;
    html_document = mg_backend_readmng_fetch_xml_main_page (self);
    mangas = mg_backend_readmng_parse_main_page (self, len, html_document);
    return mangas;

}

static xmlDocPtr
mg_backend_readmng_fetch_xml_main_page (MgBackendReadmng *self) {
    size_t size_response_text = 0;
    return htmlReadMemory (mg_backend_readmng_get_main_page (self, &size_response_text),
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
        self->main_page_html = get_request (self->base_url,
                &self->main_page_html_len);
    }
    if (len) {
        *len = self->main_page_html_len;
    }
    return self->main_page_html;
}

static MgManga **
mg_backend_readmng_parse_main_page (MgBackendReadmng *self, size_t *len, const xmlDocPtr html_document) {
    MgManga **mangas = NULL;
    xmlNodePtr *li;

    *len = 0;
    xmlNodePtr slides = mg_backend_readmng_retrieve_slides (self, html_document);

    size_t li_len = 0;
    li = mg_backend_readmng_retrieve_li_slides (self, slides, &li_len);
    for (int i = 0; i<li_len; i++) {
        xmlNodePtr current_li = li[i];
        mangas = mg_backend_readmng_extract_manga_info_from_current_li (self, mangas, current_li,
                len);
    }
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
    xpath_result = get_nodes_xpath_expression (html_document,
            "//div[@class]");
    xmlNodePtr slides = NULL;
    xmlNodeSetPtr node_set = NULL;
    size_t matching_classes_len = 0;

    node_set = xpath_result->nodesetval;
    if (!node_set) {
        fprintf(stderr, "No match\n");
        return NULL;
    }
    for (int i = 0; i < node_set->nodeNr; i++) {
        xmlNodePtr node = node_set->nodeTab[i];
        nodes = loop_search_class (node, nodes, "slides", &matching_classes_len);
    }
    if (nodes) {
        slides = nodes[0];
    }
    if (xpath_result) {
        xmlXPathFreeObject(xpath_result);
    }
    return slides;
}

static xmlNodePtr
mg_backend_readmng_retrieve_thumbnail_from_li (MgBackendReadmng *self, xmlNodePtr current_li) {
    size_t thumbnail_len = 0;
    xmlNodePtr *thumbnail = find_class (current_li, "thumbnail",
            &thumbnail_len, NULL, 1);
    if (thumbnail_len) return thumbnail[0];
    return NULL;
}

static xmlNodePtr
mg_backend_readmng_retrieve_title_from_li (MgBackendReadmng *self, xmlNodePtr li) {
    size_t title_len = 0;
    xmlNodePtr *title = find_class (li, "title", &title_len, NULL, 1);
    if (title_len) return title[0];
    return NULL;
}

static MgManga **
mg_backend_readmng_extract_manga_info_from_current_li (MgBackendReadmng *self, 
    MgManga **mangas, xmlNodePtr current_li, size_t *len) {

    xmlNodePtr thumbnail = mg_backend_readmng_retrieve_thumbnail_from_li (self, current_li);
    xmlNodePtr title = mg_backend_readmng_retrieve_title_from_li (self, current_li);
    xmlNodePtr img;

    if (thumbnail && title && (img = mg_backend_readmng_retrieve_img_from_thumbnail (self, thumbnail))) {
        (*len)++;
        mangas = g_realloc(mangas, sizeof *mangas * *len);
        mangas[*len-1] = mg_manga_new (get_attr (img, "src"), (char *)xmlNodeGetContent (title));
    }
    return mangas;
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
