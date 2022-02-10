#include <stdbool.h>
#include <stdio.h>

#include <libsoup/soup.h>

#include <openmg/util/soup.h>

struct _MgUtilSoup {
    GObject parent_instance;
};

G_DEFINE_TYPE (MgUtilSoup, mg_util_soup, G_TYPE_OBJECT)

MgUtilSoup *
mg_util_soup_new () {
    MgUtilSoup *self = NULL;
    self = MG_UTIL_SOUP (g_object_new (MG_TYPE_UTIL_SOUP, NULL));
    return self;
}

static char *
mg_util_soup_copy_binary_data (MgUtilSoup *self, const char *input, size_t size);
static void
mg_util_soup_class_init (MgUtilSoupClass *class) {
}
static void
mg_util_soup_init (MgUtilSoup *self) {
}

char *
mg_util_soup_get_request (MgUtilSoup *self, const char *url, gsize *size_response_text) {
    SoupSession *soup_session;
    SoupMessage *msg;
    GValue response = G_VALUE_INIT;

    *size_response_text = 0;

    g_value_init (&response, G_TYPE_BYTES);

    soup_session = soup_session_new ();
    msg = soup_message_new ("GET", url);
    soup_session_send_message (soup_session, msg);
    g_object_get_property(
            G_OBJECT (msg),
            "response-body-data",
            &response);

    const char *html_response = g_bytes_get_data ((GBytes *)
            g_value_peek_pointer (&response),
            size_response_text);

    char *return_value = mg_util_soup_copy_binary_data(self, html_response, *size_response_text);

    g_value_unset (&response);
    g_clear_object (&soup_session);
    g_clear_object (&msg);
    return return_value;
}

char *
mg_util_soup_post_request_url_encoded (MgUtilSoup *self,
        const char *url, SoupParam *body, gsize body_len,
        SoupParam *headers, gsize headers_len,
        gsize *size_response_text) {
    SoupSession *soup_session;
    SoupMessage *msg;
    SoupMessageBody *request_body;
    SoupMessageHeaders *request_headers;

    GValue response = G_VALUE_INIT;
    GValue request = G_VALUE_INIT;
    GValue request_headers_value = G_VALUE_INIT;

    *size_response_text = 0;

    g_value_init (&response, G_TYPE_BYTES);
    g_value_init (&request, SOUP_TYPE_MESSAGE_BODY);
    g_value_init (&request_headers_value,
            SOUP_TYPE_MESSAGE_HEADERS);

    soup_session = soup_session_new ();
    msg = soup_message_new ("POST", url);
    g_object_get_property (
            G_OBJECT (msg),
            "request-body",
            &request);

    g_object_get_property (
            G_OBJECT (msg),
            "request-headers",
            &request_headers_value);

    soup_message_set_request (msg,
            "application/x-www-form-urlencoded; charset=UTF-8",
            SOUP_MEMORY_COPY,  "", 1);

    request_body = g_value_peek_pointer (&request);
    request_headers = g_value_peek_pointer (
            &request_headers_value);

    for (int i = 0; i < body_len; i++) {
        char *key = g_uri_escape_string (body[i].key,
                NULL, false);
        size_t key_len = strlen (key) + 1;
        char *value = g_uri_escape_string (body[i].value,
                NULL, false);
        size_t value_len = strlen (value) + 1;

        if (body_len) {
            soup_message_body_append (request_body,
                    SOUP_MEMORY_COPY, "&", 1);
        }

        soup_message_body_append (request_body,
                SOUP_MEMORY_COPY, key, key_len);
        soup_message_body_append (request_body,
                SOUP_MEMORY_COPY, "=", 1);
        soup_message_body_append (request_body,
                SOUP_MEMORY_COPY, value, value_len);

        g_free (key);
        g_free (value);
    }
    soup_message_body_append (request_body,
            SOUP_MEMORY_COPY, "", 1);

    for (int i = 0; i < headers_len; i++) {
        soup_message_headers_append (request_headers,
                headers[i].key,
                headers[i].value);
    }

    soup_session_send_message (soup_session, msg);
    g_object_get_property(
            G_OBJECT (msg),
            "response-body-data",
            &response);

    const char *html_response = g_bytes_get_data ((GBytes *)
            g_value_peek_pointer (&response),
            size_response_text);

    char *return_value = mg_util_soup_copy_binary_data(self, html_response, *size_response_text);

    g_value_unset (&response);
    g_value_unset (&request);

    g_clear_object (&soup_session);
    g_clear_object (&msg);

    return return_value;
}

static char *
mg_util_soup_copy_binary_data (MgUtilSoup *self, const char *input, size_t size) {
    char *response = NULL;
    if (size) {
        response = g_realloc(response, sizeof *response * size);
        for (size_t i = 0; i<size; i++) {
            response[i] = input[i];
        }
    }
    return response;
}
