#pragma once

#include <gtk/gtk.h>
#include <adwaita.h>

#include <openmg/manga.h>

GtkListView *
create_list_view_chapters (MgManga *manga, AdwLeaflet *views_leaflet);
