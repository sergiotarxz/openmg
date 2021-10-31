#pragma once
#include <glib-object.h>

G_BEGIN_DECLS;

#define MG_TYPE_UTIL_STRING mg_util_string_get_type()
G_DECLARE_FINAL_TYPE (MgUtilString, mg_util_string, MG, UTIL_STRING, GObject)

MgUtilString *mg_util_string_new ();
char *
mg_util_string_alloc_string(MgUtilString *self, size_t len);
void
mg_util_string_copy_substring(MgUtilString *self,
        const char *origin, char *dest, size_t dest_len, size_t start,
        size_t len);

G_END_DECLS
