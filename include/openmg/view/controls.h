#pragma once

#include <glib.h>

#include <gtk/gtk.h>
#include <adwaita.h>

typedef struct {
    AdwHeaderBar *header;
    AdwLeaflet *views_leaflet;
    AdwViewStack *view_stack;
    GCancellable **image_threads;
    size_t image_threads_len;
    bool avoid_list_image_downloads;
    GtkButton *previous;
    gboolean is_set_previous;
} ControlsAdwaita;
