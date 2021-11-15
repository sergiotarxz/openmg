#include <gtk/gtk.h>

#include <openmg/chapter.h>
#include <openmg/view/chapter_view.h>
#include <openmg/view/picture.h>
#include <openmg/util/gobject_utility_extensions.h>
#include <openmg/util/string.h>
#include <openmg/backend/readmng.h>

static void
fire_zoom (GtkGestureZoom *zoom,
        gdouble scale,
        gpointer user_data);

typedef struct {
    guint current_page;
    GListModel *pages; 
    GtkPicture *current_picture;
} ChapterVisorData;
void
setup_chapter_view (MgMangaChapter *chapter, AdwLeaflet *views_leaflet) {
    MgBackendReadmng *readmng = mg_backend_readmng_new ();
    MgUtilString *string_util = mg_util_string_new ();
    GtkBox *chapter_view_container = GTK_BOX (gtk_box_new (GTK_ORIENTATION_VERTICAL, 0));
    GtkScrolledWindow *zoomable_picture_container = GTK_SCROLLED_WINDOW (gtk_scrolled_window_new ());
    GtkGesture *zoom_controller = gtk_gesture_zoom_new ();
    GListModel *pages = mg_backend_readmng_get_chapter_images (readmng, chapter);
    ChapterVisorData *chapter_visor_data = g_malloc (sizeof *chapter_visor_data);
    GtkPicture *current_picture;
    chapter_visor_data->current_page = 0;
    chapter_visor_data->pages = pages;
    const char *url_image_not_owned =
    gtk_string_list_get_string (GTK_STRING_LIST 
                                     (pages), 0);
    char *url_image = mg_util_string_alloc_string (string_util, strlen (url_image_not_owned));
    mg_util_string_copy_substring (string_util,
            url_image_not_owned, url_image,
            strlen(url_image_not_owned) + 1, 0,
            strlen (url_image_not_owned));
 
    current_picture = create_picture_from_url
        (url_image, 200);
    g_object_set_property_int (G_OBJECT (current_picture), "vexpand", 1);
    g_object_set_property_int (G_OBJECT (current_picture), "hexpand", 1);
    chapter_visor_data->current_picture =
        current_picture;
    gtk_scrolled_window_set_child (zoomable_picture_container, GTK_WIDGET (current_picture));


    gtk_widget_add_controller (GTK_WIDGET (zoomable_picture_container),
            GTK_EVENT_CONTROLLER (zoom_controller));
    g_signal_connect (G_OBJECT (zoom_controller), "scale-changed", G_CALLBACK (fire_zoom), NULL);

    gtk_box_append (chapter_view_container, GTK_WIDGET (zoomable_picture_container));
    adw_leaflet_append (views_leaflet, GTK_WIDGET (chapter_view_container));
    adw_leaflet_navigate (views_leaflet, ADW_NAVIGATION_DIRECTION_FORWARD);
}

static void
fire_zoom (GtkGestureZoom *zoom,
        gdouble scale,
        gpointer user_data) {
    // Do something. 
}
