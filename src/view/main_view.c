#include <dlfcn.h>

#include <gtk/gtk.h>
#include <adwaita.h>

#include <openmg/backend/readmng.h>
#include <openmg/manga.h>
#include <openmg/view/controls.h>
#include <openmg/view/list_view_manga.h>

static AdwHeaderBar *
create_headerbar (GtkBox *box, AdwLeaflet *views_leaflet, GtkButton **out_previous);
static GtkBox *
create_main_box (AdwApplicationWindow *window);
static void
go_back_view (GtkButton *previous, gpointer user_data);
static void
hide_main_view (GtkWidget *main_view,
        gpointer user_data);
static void
show_main_view (GtkWidget *main_view,
        gpointer user_data);

static void
activate (AdwApplication *app,
        gpointer user_data)
{
    GtkWidget *window =
        adw_application_window_new (GTK_APPLICATION (app));
    GtkBox *box = create_main_box(
            ADW_APPLICATION_WINDOW
            (window));
    GListStore *mangas;
    MgBackendReadmng *readmng = mg_backend_readmng_new ();
    GtkListView *list_view;
    GtkWidget *scroll;
    AdwLeaflet *views_leaflet = ADW_LEAFLET (adw_leaflet_new ());
    ControlsAdwaita *controls = g_malloc (sizeof *controls);
    GtkButton *previous = NULL;
    AdwHeaderBar *header_bar = create_headerbar (box, views_leaflet, &previous);

    typedef void (*swipe_back_t)(AdwLeaflet *, gboolean);
    swipe_back_t swipe_back = (swipe_back_t) dlsym
        (NULL, "adw_leaflet_set_can_navigate_back");


    if (!swipe_back) {
        swipe_back = (swipe_back_t) dlsym
        (NULL, "adw_leaflet_set_can_swipe_back");
    }
    swipe_back (views_leaflet, 1);

    controls->header = header_bar;
    controls->views_leaflet = views_leaflet;
    controls->previous = previous;

    mangas = mg_backend_readmng_get_featured_manga (readmng);
    list_view = create_list_view_mangas (mangas, controls);
    scroll = gtk_scrolled_window_new ();
    g_signal_connect (scroll, "map", G_CALLBACK (show_main_view), controls);
    g_signal_connect (scroll, "unmap", G_CALLBACK (hide_main_view), controls);

    gtk_widget_set_valign (scroll, GTK_ALIGN_FILL);
    gtk_widget_set_vexpand (scroll, 1);
    gtk_scrolled_window_set_child (GTK_SCROLLED_WINDOW (scroll), GTK_WIDGET (list_view));

    adw_leaflet_append (views_leaflet, scroll);
    adw_leaflet_set_can_unfold (views_leaflet, false);

    gtk_box_append (box, GTK_WIDGET (views_leaflet));

    gtk_widget_show (window);
    g_clear_object (&readmng);
}

static GtkBox *
create_main_box (AdwApplicationWindow *window) {
    GtkWidget *box = gtk_box_new(
            GTK_ORIENTATION_VERTICAL,
            10);
    adw_application_window_set_content(
            window,
            box);
    return GTK_BOX (box);
}

static void
show_main_view (GtkWidget *main_view,
        gpointer user_data) {
    ControlsAdwaita *controls = (ControlsAdwaita *) user_data;
    GtkWidget *previous = GTK_WIDGET (controls->previous);
    AdwHeaderBar *header = controls->header;
    adw_header_bar_remove (header, previous);
}

static void
hide_main_view (GtkWidget *main_view,
        gpointer user_data) {
    ControlsAdwaita *controls = (ControlsAdwaita *) user_data;
    GtkWidget *previous = GTK_WIDGET (controls->previous);
    AdwHeaderBar *header = controls->header;
    adw_header_bar_pack_start (header, previous);
}

static AdwHeaderBar *
create_headerbar (GtkBox *box, AdwLeaflet *views_leaflet, GtkButton **out_previous) {
    GtkWidget *title =
        adw_window_title_new ("Window", NULL);
    GtkWidget *header =
        adw_header_bar_new();
    adw_header_bar_set_title_widget(
            ADW_HEADER_BAR (header),
            GTK_WIDGET (title));
    gtk_box_append (box, header);
    GtkWidget *previous = gtk_button_new_from_icon_name ("go-previous-symbolic");
    g_signal_connect (G_OBJECT (previous), "clicked", G_CALLBACK (go_back_view),
            views_leaflet);

    if (out_previous) {
        *out_previous = GTK_BUTTON (previous);
        g_object_ref (*out_previous);
    }


    return ADW_HEADER_BAR (header);
}

static void
go_back_view (GtkButton *previous, gpointer user_data) {
    AdwLeaflet *views_leaflet = ADW_LEAFLET (user_data);
    adw_leaflet_navigate (views_leaflet, ADW_NAVIGATION_DIRECTION_BACK);
}
    int
main_view_run (int argc,
        char **argv)
{
    AdwApplication *app = adw_application_new ("me.sergiotarxz.mangareader", G_APPLICATION_FLAGS_NONE);
    int status = 0;
    g_signal_connect (app, "activate", G_CALLBACK (activate), NULL);
    status = g_application_run (G_APPLICATION (app), argc, argv);
    g_clear_object (&app);
    return status;
}
