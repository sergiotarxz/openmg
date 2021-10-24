#include <manga.h>
#include <libxml/HTMLparser.h>

const char *readmng_url = "https://www.readmng.com/";

struct Manga *
parse_readmng_title_page (const xmlDocPtr html_document,
        size_t *const len);
xmlNodePtr
retrieve_slides (const xmlDocPtr html_document);
xmlNodePtr
retrieve_ul_slides (xmlNodePtr const slides);
xmlNodePtr *
retrieve_li_slides (xmlNodePtr const slides, size_t *li_len);
xmlNodePtr
retrieve_img_from_thumnail (xmlNodePtr thumbnail);
xmlNodePtr
retrieve_thumbnail_from_li (xmlNodePtr current_li);
xmlNodePtr
retrieve_title_from_li (xmlNodePtr li);
struct Manga *
extract_manga_info_from_current_li (struct Manga *mangas,
    xmlNodePtr current_li, size_t *len);

struct Manga *
retrieve_readmng_title_mangas (size_t *const len) {
    xmlDocPtr html_response;
    gsize size_response_text;
    struct Manga *mangas;
    char *response_text = get_request (readmng_url,
            &size_response_text);
    html_response = htmlReadMemory (response_text,
            size_response_text,
            NULL,
            NULL,
            HTML_PARSE_RECOVER | HTML_PARSE_NODEFDTD 
            | HTML_PARSE_NOERROR | HTML_PARSE_NOWARNING
            );
    mangas = parse_readmng_title_page (html_response, len);
    xmlFreeDoc (html_response);
    g_free (response_text);
    return mangas;
}

struct Manga *
parse_readmng_title_page (const xmlDocPtr html_document,
        size_t *const len) {
    struct Manga *mangas = NULL;
    xmlNodePtr slides = retrieve_slides (html_document);
    *len = 0;
    size_t li_len = 0;
    xmlNodePtr *li = retrieve_li_slides (slides, &li_len);
    for (int i = 0; i<li_len; i++) {
        xmlNodePtr current_li = li[i];
        mangas = extract_manga_info_from_current_li (mangas, current_li,
                len);
    }
    return mangas;
}

struct Manga *
extract_manga_info_from_current_li (struct Manga *mangas, xmlNodePtr current_li,
        size_t *len) {
    xmlNodePtr thumbnail = retrieve_thumbnail_from_li (current_li);
    xmlNodePtr title = retrieve_title_from_li (current_li);
    xmlNodePtr img;

    if (thumbnail && title && (img = retrieve_img_from_thumnail (thumbnail))) {
        (*len)++;
        mangas = g_realloc(mangas, sizeof *mangas * *len);
        struct Manga *manga = &mangas[*len-1];
        manga->image_url = get_attr (img, "src");
        manga->title = (char *) xmlNodeGetContent (title);
    }
    return mangas;
}

xmlNodePtr
retrieve_title_from_li (xmlNodePtr li) {
    size_t title_len = 0;
    xmlNodePtr *title = find_class (li, "title", &title_len, NULL, 1);
    if (title_len) return title[0];
    return NULL;
}

xmlNodePtr
retrieve_img_from_thumnail (xmlNodePtr thumbnail) {
    for (xmlNodePtr child = thumbnail->children; child; child=child->next) {
        if (!strcmp((char *)child->name, "img")) {
            return child;
        }
    }
    return NULL;
}

xmlNodePtr
retrieve_thumbnail_from_li (xmlNodePtr current_li) {
    size_t thumbnail_len = 0;
    xmlNodePtr *thumbnail = find_class (current_li, "thumbnail",
            &thumbnail_len, NULL, 1);
    if (thumbnail_len) return thumbnail[0];
    return NULL;
}

xmlNodePtr *
retrieve_li_slides (xmlNodePtr const slides, size_t *li_len) {
    xmlNodePtr ul_slides = retrieve_ul_slides (slides);
    xmlNodePtr *li = NULL;
    for (xmlNodePtr child = ul_slides->children; child; child=child->next) {
        (*li_len)++;
        li = g_realloc(li, sizeof *li * *li_len);
        li[*li_len-1] = xmlCopyNode(child, XML_COPY_NODE_RECURSIVE);
    }
    return li;
}

xmlNodePtr
retrieve_ul_slides (xmlNodePtr const slides) {
    for (xmlNodePtr child = slides->children; child; child = child->next) {
        if (!strcmp((char *) child->name, "ul")) {
            return child;
        }
    }
    return NULL;
}

xmlNodePtr
retrieve_slides (const xmlDocPtr html_document) {
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
