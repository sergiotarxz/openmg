#include <math.h>

#include <gtk/gtk.h>
#include <gdk/gdk.h>

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
    AdwLeaflet *views_leaflet;
    GtkScrolledWindow *zoomable_picture_container;
    double zoom;
} ChapterVisorData;

static void
set_image_zoomable_picture_container (ChapterVisorData *chapter_visor_data);
static void
set_zoomable_picture_container_properties (
        GtkScrolledWindow *zoomable_picture_container,
        ChapterVisorData *chapter_visor_data);
static void
append_chapter_view_leaflet (AdwLeaflet *views_leaflet,
        GtkBox *chapter_view_container);
static void
add_controls_overlay (GtkOverlay *overlay, ChapterVisorData *chapter_visor_data);
static void
go_prev (GtkButton *prev,
        gpointer user_data);
static void
go_next (GtkButton *next,
        gpointer user_data);
static void
set_image_dimensions (GtkWidget *picture,
        ChapterVisorData *chapter_visor_data,
        gdouble scale);
static void
zoom_end (GtkGesture *zoom,
        GdkEventSequence *sequence,
        gpointer user_data);
static void
picture_ready_manga_page (GObject *source_object,
        GAsyncResult *res,
        gpointer user_data);
void
zoomable_container_keybinding_handle (GtkEventControllerKey *self,
        guint keyval, guint keycode, GdkModifierType state, gpointer user_data);

static void
image_page_show (GtkWidget *picture, gpointer user_data) {
    ChapterVisorData *chapter_visor_data = (ChapterVisorData *) user_data;

    set_image_dimensions (picture, chapter_visor_data, 1);
}

static void
set_image_dimensions (GtkWidget *picture,
        ChapterVisorData *chapter_visor_data,
        gdouble scale) {
    double final_width = 0;
    double final_height = 0;
    GdkPaintable *paintable = gtk_picture_get_paintable (GTK_PICTURE (picture));
    GtkWidget *views_leaflet = GTK_WIDGET (chapter_visor_data->views_leaflet);
    gdouble scale_factor = log (scale) / 20 + log (chapter_visor_data->zoom);
    gdouble final_zoom = pow (M_E, scale_factor);
    guint views_leaflet_width = gtk_widget_get_allocated_width (views_leaflet);
    if (final_zoom > 3) {
        final_zoom = 3;
    }
    if (final_zoom < 1/3) {
        final_zoom = 1/3;
    }
    chapter_visor_data->zoom = final_zoom;
    if (views_leaflet_width > 600) {
        views_leaflet_width = 300;
    }
    guint width = views_leaflet_width * chapter_visor_data->zoom;
    gdk_paintable_compute_concrete_size (
            paintable,
            width,
            0,
            gdk_paintable_get_intrinsic_width (paintable),
            gdk_paintable_get_intrinsic_height (paintable),
            &final_width,
            &final_height
            );
    g_object_set_property_int (G_OBJECT (picture),
            "width-request", (int) final_width);
    g_object_set_property_int (G_OBJECT (picture),
            "height-request", (int) final_height);
    g_object_set_property_int (G_OBJECT (picture),
            "width-request", (int) final_width);
    g_object_set_property_int (G_OBJECT (picture),
            "height-request", (int) final_height);
}

void
setup_chapter_view (MgMangaChapter *chapter, AdwLeaflet *views_leaflet) {
    MgBackendReadmng *readmng = mg_backend_readmng_new ();
    GtkBox *chapter_view_container = GTK_BOX (gtk_box_new (GTK_ORIENTATION_VERTICAL, 0));
    GtkScrolledWindow *zoomable_picture_container = GTK_SCROLLED_WINDOW (gtk_scrolled_window_new ());
    GtkOverlay *overlay = GTK_OVERLAY (gtk_overlay_new ());
    GListModel *pages = mg_backend_readmng_get_chapter_images (readmng, chapter);
    ChapterVisorData *chapter_visor_data = g_malloc (sizeof *chapter_visor_data);

    chapter_visor_data->current_page = 0;
    chapter_visor_data->pages = pages;
    chapter_visor_data->views_leaflet = views_leaflet;
    chapter_visor_data->zoom = 1;
    chapter_visor_data->zoomable_picture_container = zoomable_picture_container;

    set_zoomable_picture_container_properties (zoomable_picture_container,
            chapter_visor_data);
    set_image_zoomable_picture_container (chapter_visor_data);

    gtk_overlay_set_child (overlay, GTK_WIDGET (zoomable_picture_container));
    add_controls_overlay (overlay, chapter_visor_data);
    gtk_box_append (chapter_view_container, GTK_WIDGET (overlay));
    append_chapter_view_leaflet (views_leaflet, chapter_view_container);

    g_clear_object (&readmng);
}

static void
add_controls_overlay (GtkOverlay *overlay, ChapterVisorData *chapter_visor_data) {
    GtkButton *next_button = GTK_BUTTON (gtk_button_new_from_icon_name
            ("go-next-symbolic"));
    GtkButton *previous_button = GTK_BUTTON (gtk_button_new_from_icon_name
            ("go-previous-symbolic"));
    g_signal_connect (G_OBJECT (next_button), "clicked", G_CALLBACK (go_next), chapter_visor_data);
    g_signal_connect (G_OBJECT (previous_button), "clicked", G_CALLBACK (go_prev), chapter_visor_data);
    gtk_widget_set_valign (GTK_WIDGET (next_button), GTK_ALIGN_CENTER);
    gtk_widget_set_halign (GTK_WIDGET (next_button), GTK_ALIGN_END);
    gtk_widget_set_valign (GTK_WIDGET (previous_button), GTK_ALIGN_CENTER);
    gtk_widget_set_halign (GTK_WIDGET (previous_button), GTK_ALIGN_START);
    gtk_overlay_add_overlay (overlay, GTK_WIDGET (next_button));
    gtk_overlay_add_overlay (overlay, GTK_WIDGET (previous_button));
}
static void
go_next (GtkButton *next,
        gpointer user_data) {
    ChapterVisorData *chapter_visor_data = (ChapterVisorData *) user_data;
    GListModel *pages = chapter_visor_data->pages;
    if (chapter_visor_data->current_page < g_list_model_get_n_items (pages) -1) {
        chapter_visor_data->current_page = chapter_visor_data->current_page + 1;
        gtk_widget_grab_focus (GTK_WIDGET (chapter_visor_data->zoomable_picture_container));
        set_image_zoomable_picture_container (chapter_visor_data);
    }
}
static void
go_prev (GtkButton *prev,
        gpointer user_data) {
    ChapterVisorData *chapter_visor_data = (ChapterVisorData *) user_data;
    if (chapter_visor_data->current_page > 0) {
        chapter_visor_data->current_page = chapter_visor_data->current_page - 1;
        gtk_widget_grab_focus (GTK_WIDGET (chapter_visor_data->zoomable_picture_container));
        set_image_zoomable_picture_container (chapter_visor_data);
    }
}

static void
set_image_zoomable_picture_container (ChapterVisorData *chapter_visor_data) {
    GtkScrolledWindow *zoomable_picture_container = chapter_visor_data->zoomable_picture_container;
    MgUtilString *string_util = mg_util_string_new ();
    GListModel *pages = chapter_visor_data->pages;
    guint current_page = chapter_visor_data->current_page;
    const char *url_image_not_owned =
        gtk_string_list_get_string (GTK_STRING_LIST 
                (pages), current_page);
    char *url_image = mg_util_string_alloc_string (string_util, strlen (url_image_not_owned));
    mg_util_string_copy_substring (string_util,
            url_image_not_owned, url_image,
            strlen(url_image_not_owned) + 1, 0,
            strlen (url_image_not_owned));

    GtkPicture *picture = create_picture_from_url (url_image, 0, picture_ready_manga_page,
            zoomable_picture_container, chapter_visor_data, false);
    if (picture) {
        chapter_visor_data->current_picture = GTK_PICTURE (picture);
        g_signal_connect (G_OBJECT (picture), "map",
                G_CALLBACK (image_page_show), chapter_visor_data);
        gtk_scrolled_window_set_child (zoomable_picture_container, GTK_WIDGET (picture));
    }
    g_free (url_image);
    g_clear_object (&string_util);
}

static void
picture_ready_manga_page (GObject *source_object,
        GAsyncResult *res,
        gpointer user_data) {
    GTask *task =  G_TASK (res);
    ChapterVisorData *chapter_visor_data = (ChapterVisorData *) user_data;
    GtkWidget *picture = g_task_propagate_pointer (task, NULL);
    GtkScrolledWindow *zoomable_picture_container = GTK_SCROLLED_WINDOW (source_object);
    if (GTK_IS_WIDGET (picture)) {
        chapter_visor_data->current_picture = GTK_PICTURE (picture);
        g_signal_connect (G_OBJECT (picture), "map",
                G_CALLBACK (image_page_show), chapter_visor_data);
        gtk_scrolled_window_set_child (zoomable_picture_container, GTK_WIDGET (picture));
    }
}

static void
set_zoomable_picture_container_properties (
        GtkScrolledWindow *zoomable_picture_container,
        ChapterVisorData *chapter_visor_data) {
    GtkGesture *zoom_controller = gtk_gesture_zoom_new ();
    GtkEventController *key_controller = gtk_event_controller_key_new ();

    g_signal_connect (G_OBJECT (key_controller), "key-pressed", G_CALLBACK (zoomable_container_keybinding_handle),
            chapter_visor_data);
    gtk_widget_add_controller (GTK_WIDGET (zoomable_picture_container),
            key_controller);

    g_object_set_property_int (G_OBJECT (zoomable_picture_container), "hexpand", 1);
    g_object_set_property_int (G_OBJECT (zoomable_picture_container), "vexpand", 1);
    gtk_widget_add_controller (GTK_WIDGET (zoomable_picture_container),
            GTK_EVENT_CONTROLLER (zoom_controller));
    g_signal_connect (G_OBJECT (zoom_controller), "scale-changed", G_CALLBACK (fire_zoom), chapter_visor_data);
    g_signal_connect (G_OBJECT (zoom_controller), "end", G_CALLBACK (zoom_end), chapter_visor_data);
}

void
zoomable_container_keybinding_handle (GtkEventControllerKey *self,
        guint keyval, guint keycode, GdkModifierType state, gpointer user_data) {
    ChapterVisorData *chapter_visor_data = (ChapterVisorData *) user_data;
    GtkScrolledWindow *zoomable_picture_container = chapter_visor_data->zoomable_picture_container;
    GtkAdjustment *vadjustment = gtk_scrolled_window_get_vadjustment (zoomable_picture_container);
    GtkAdjustment *hadjustment = gtk_scrolled_window_get_hadjustment (zoomable_picture_container);
    GValue adjustment = G_VALUE_INIT;
    gdouble current_adjustment;
    gdouble change_rate_key_movement = 50;
    printf ("got here\n");

    if (state & GDK_CONTROL_MASK ) {
        if ( keyval == '+' ) {
            set_image_dimensions (GTK_WIDGET (chapter_visor_data->current_picture),
                    chapter_visor_data, 2);
        }
        if ( keyval == '-' ) {
            set_image_dimensions (GTK_WIDGET (chapter_visor_data->current_picture),
                    chapter_visor_data, 0.5);
        }
    }
    if (state & GDK_SHIFT_MASK ) {
        g_object_get_property (G_OBJECT (hadjustment), "value", &adjustment);
        current_adjustment = g_value_get_double (&adjustment);
        if (keyval == 65361) {
            // LEFT
            g_object_set_property_double (G_OBJECT (hadjustment), "value", current_adjustment - change_rate_key_movement);
        }
        if (keyval == 65363) {
            // RIGHT 
            g_object_set_property_double (G_OBJECT (hadjustment), "value", current_adjustment + change_rate_key_movement);
        }
        g_object_get_property (G_OBJECT (vadjustment), "value", &adjustment);
        current_adjustment = g_value_get_double (&adjustment);
        if (keyval == 65362) {
            // UP 
            g_object_set_property_double (G_OBJECT (vadjustment), "value", current_adjustment - change_rate_key_movement);

        }
        if (keyval == 65364) {
            // UP 
            g_object_set_property_double (G_OBJECT (vadjustment), "value", current_adjustment + change_rate_key_movement);
        }

    }
}

static void
append_chapter_view_leaflet (AdwLeaflet *views_leaflet,
        GtkBox *chapter_view_container) {
    GtkWidget *widget = adw_leaflet_get_adjacent_child
        (views_leaflet, ADW_NAVIGATION_DIRECTION_FORWARD);
    for (;widget;widget = adw_leaflet_get_adjacent_child
            (views_leaflet, ADW_NAVIGATION_DIRECTION_FORWARD)) {
        adw_leaflet_remove (views_leaflet, widget);
    }
    adw_leaflet_append (views_leaflet, GTK_WIDGET (chapter_view_container));
    adw_leaflet_navigate (views_leaflet, ADW_NAVIGATION_DIRECTION_FORWARD);

}

static void
fire_zoom (GtkGestureZoom *zoom,
        gdouble scale,
        gpointer user_data) {
    ChapterVisorData *chapter_visor_data = (ChapterVisorData *) user_data;
    set_image_dimensions (GTK_WIDGET (chapter_visor_data->current_picture),
            chapter_visor_data, scale);
}

static void
zoom_end (GtkGesture *zoom,
        GdkEventSequence *sequence,
        gpointer user_data) {
    ChapterVisorData *chapter_visor_data = (ChapterVisorData *) user_data;
    gdouble scale = gtk_gesture_zoom_get_scale_delta
        (GTK_GESTURE_ZOOM (zoom));
    gdouble scale_factor = log (scale) / 20 + log (chapter_visor_data->zoom);
    chapter_visor_data->zoom = pow (M_E, scale_factor);
}
