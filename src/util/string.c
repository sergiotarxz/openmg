#include <stdio.h>

#include <glib/gprintf.h>
#include <glib-object.h>

#include <openmg/util/string.h>

struct _MgUtilString {
    GObject parent_instance;
};

G_DEFINE_TYPE (MgUtilString, mg_util_string, G_TYPE_OBJECT)

static void
mg_util_string_class_init (MgUtilStringClass *class) {
}

static void
mg_util_string_init (MgUtilString *class) {
}

void
mg_util_string_copy_substring(MgUtilString *self,
        const char *origin, char *dest, size_t dest_len, size_t start,
        size_t len) {
    size_t copying_offset = 0;
    while (copying_offset < len) {
        if (!(start+copying_offset <=dest_len)) {
            fprintf(stderr, "Read attempt out of bounds.%ld %ld %ld\n", dest_len, start, len);
            break;
        }
        dest[copying_offset] = origin[start+copying_offset];
        copying_offset++;
    }
    dest[len] = '\0';
}

char *
mg_util_string_alloc_string(MgUtilString *self, size_t len) {
    char * return_value = NULL;
    return g_malloc (len + 1 * sizeof *return_value);
}

MgUtilString *
mg_util_string_new () {
    MgUtilString *self = NULL;
    self = MG_UTIL_STRING ((g_object_new (MG_TYPE_UTIL_STRING, NULL)));
    return self;
}
int
g_asprintf (char **strp, const char *format, ...) {
    va_list ap;
    va_start (ap, format);
    int retval = g_vasprintf (strp, format, ap);
    va_end (ap);
    return retval;
}
