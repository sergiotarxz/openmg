#include <glib-object.h>

#include <libxml/HTMLparser.h>
#include <libxml/xpath.h>

#include <openmg/util/xml.h>

struct _MgUtilXML {
    GObject parent_instance;
};

G_DEFINE_TYPE (MgUtilXML, mg_util_xml, G_TYPE_OBJECT)


static void
mg_util_xml_class_init (MgUtilXMLClass *class) {
}
static void
mg_util_xml_init (MgUtilXML *self) {
}

MgUtilXML *
mg_util_xml_new () {
    MgUtilXML *self = NULL;
    self = MG_UTIL_XML ((g_object_new (MG_TYPE_UTIL_XML, NULL)));
    return self;
}

xmlNodePtr *
mg_util_xml_find_class (MgUtilXML *self, xmlNodePtr node, char *class,
        size_t *len, xmlNodePtr *nodes, int return_on_first) {
    for (xmlNodePtr child = node->children; child; child=child->next) {
        char *attr = mg_util_xml_get_attr (self, child, "class");
        if (attr && mg_util_xml_has_class (self, attr, class)) {
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
                nodes = mg_util_xml_find_class (self, child, class,
                        len, nodes, return_on_first);
                if (*len) {
                    return nodes;
                }
            }
        }
    }
    return nodes;
}

char *
mg_util_xml_get_attr (MgUtilXML *self, xmlNodePtr const node, const char *attr_name) {
    char *return_value = NULL;
    if (!node) {
        return NULL;
    }
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
mg_util_xml_print_debug_nodes (MgUtilXML *self,
        const xmlDocPtr html_document, xmlNodePtr *nodes,
        size_t nodes_len) {
    xmlBufferPtr buffer = xmlBufferCreate ();
    for (int i = 0; i < nodes_len; i++) {
        xmlNodeDump (buffer, html_document, nodes[i],
                0, 1);
    }
    xmlBufferDump (stdout, buffer);
    xmlBufferFree (buffer);
}

int
mg_util_xml_has_class (MgUtilXML *self, 
        const char *class_attribute, const char *class_to_check) {
    char *re = "\\s+";
    struct SplittedString *classes;
    int return_value = 0;
    classes = split (re, strlen(re), class_attribute,
            strlen (class_attribute));
    for (int i = 0; i<classes->n_strings; i++) {
        if (strcmp (classes->substrings[i].content, class_to_check) == 0) {
            return_value = 1;
            goto cleanup_has_class;
        }
    }

cleanup_has_class:
    splitted_string_free (classes);
    return return_value;
}

xmlNodePtr *
mg_util_xml_loop_search_class (MgUtilXML *self, const xmlNodePtr node, xmlNodePtr *nodes,
        const char * class, size_t *len) {
    char *content = mg_util_xml_get_attr (self, node, "class");
    if (!content) {
        return nodes;
    }
    if (mg_util_xml_has_class (self, content, class)) {
        (*len)++;
        nodes = g_realloc (nodes, (sizeof *nodes) * *len);
        nodes[(*len)-1] = xmlCopyNode(node, XML_COPY_NODE_RECURSIVE);
    }
    g_free (content);
    return nodes;
}

xmlXPathObjectPtr
mg_util_xml_get_nodes_xpath_expression (MgUtilXML *self,
        const xmlDocPtr document, char *xpath) {
    xmlXPathContextPtr context;
    xmlXPathObjectPtr result;

    context = xmlXPathNewContext (document);
    result = xmlXPathEvalExpression ((const xmlChar *)xpath, context);

    xmlXPathFreeContext (context);

    return result;
}
