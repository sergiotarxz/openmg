#include <adwaita.h>
#include <gtk/gtk.h>

#include <openmg/view/list_view_manga.h>
#include <openmg/view/controls.h>
#include <openmg/view/explore.h>

#include <openmg/backend/readmng.h>
#include <openmg/manga.h>

static void
hide_main_view (GtkWidget *main_view,
        gpointer user_data);
static void
show_main_view (GtkWidget *main_view,
        gpointer user_data);

GtkWidget *
create_explore_view (ControlsAdwaita *controls) {
    GtkListView *list_view;
    GListStore *mangas;
    GtkWidget *scroll = gtk_scrolled_window_new ();
    MgBackendReadmng *readmng = mg_backend_readmng_new ();
    mangas = mg_backend_readmng_get_featured_manga (readmng);
    g_signal_connect (scroll, "map", G_CALLBACK (show_main_view), controls);
    g_signal_connect (scroll, "unmap", G_CALLBACK (hide_main_view), controls);
    list_view = create_list_view_mangas (mangas, controls);
    gtk_widget_set_valign (scroll, GTK_ALIGN_FILL);
    gtk_widget_set_vexpand (scroll, 1);
    gtk_scrolled_window_set_child (GTK_SCROLLED_WINDOW (scroll), GTK_WIDGET (list_view));
    g_clear_object (&readmng);
    return scroll;
}

static void
show_main_view (GtkWidget *main_view,
        gpointer user_data) {
    ControlsAdwaita *controls = (ControlsAdwaita *) user_data;
    GtkWidget *previous = GTK_WIDGET (controls->previous);
    AdwHeaderBar *header = controls->header;
    if (controls->is_set_previous) {
        adw_header_bar_remove (header, previous);
        controls->is_set_previous = 0;
    }
}

static void
hide_main_view (GtkWidget *main_view,
        gpointer user_data) {
    ControlsAdwaita *controls = (ControlsAdwaita *) user_data;
    GtkWidget *previous = GTK_WIDGET (controls->previous);
    AdwHeaderBar *header = controls->header;
    if (!controls->is_set_previous) {
        adw_header_bar_pack_start (header, previous);
        controls->is_set_previous = 1;
    }
}
