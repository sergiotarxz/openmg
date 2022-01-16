#pragma once
#include <glib-object.h>

G_BEGIN_DECLS

#define MG_TYPE_UTIL_SOUP mg_util_soup_get_type()
G_DECLARE_FINAL_TYPE (MgUtilSoup, mg_util_soup, MG, UTIL_SOUP, GObject)

MgUtilSoup *
mg_util_soup_new ();

typedef struct { 
    char *key;
    char *value;
} SoupParam;

char *
mg_util_soup_get_request (MgUtilSoup *self, const char *const url, gsize *size_response_text);
char *
mg_util_soup_post_request_url_encoded (MgUtilSoup *self,
        const char *url, SoupParam *body, gsize body_len,
        SoupParam *headers, gsize headers_len,
        gsize *size_response_text);

G_END_DECLS
