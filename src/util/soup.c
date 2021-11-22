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
