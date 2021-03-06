#pragma once

#include <glib-object.h>

#include <libxml/HTMLparser.h>
#include <libxml/xpath.h>

G_BEGIN_DECLS;

#define XML_COPY_NODE_RECURSIVE 2 | 1
#define MG_TYPE_UTIL_XML mg_util_xml_get_type()

G_DECLARE_FINAL_TYPE (MgUtilXML, mg_util_xml, MG, UTIL_XML, GObject)

MgUtilXML *
mg_util_xml_new ();;

char *
mg_util_xml_get_attr (MgUtilXML *self, xmlNodePtr const node, const char *attr_name);

xmlNodePtr *
mg_util_xml_find_class (MgUtilXML *self, xmlNodePtr node, char *class,
        size_t *len, xmlNodePtr *nodes, int return_on_first);

xmlNodePtr *
mg_util_xml_loop_search_class (MgUtilXML *self, const xmlNodePtr node, xmlNodePtr *nodes,
        const char * class, size_t *len);
xmlXPathObjectPtr
mg_util_xml_get_nodes_xpath_expression (MgUtilXML *self,
        const xmlDocPtr document, const xmlNodePtr node, char *xpath);
int
mg_util_xml_has_class (MgUtilXML *self, 
        const char *class_attribute, const char *class_to_check);
char *
mg_util_xml_get_title_text (MgUtilXML *self,
        const char *const text);
void
mg_util_xml_print_debug_nodes (MgUtilXML *self,
        const xmlDocPtr html_document, xmlNodePtr *nodes,
        size_t nodes_len);
G_END_DECLS
