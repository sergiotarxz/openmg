#pragma once

#include <glib.h>

typedef struct {
    AdwHeaderBar *header;
    AdwLeaflet *views_leaflet;
    GtkButton *previous;
    gboolean is_set_previous;
} ControlsAdwaita;
