#pragma once
#include <gtk/gtk.h>
void
create_picture_from_url (const char *const url, gint picture_size,
        GAsyncReadyCallback ready, gpointer source_object,
        gpointer callback_data);
