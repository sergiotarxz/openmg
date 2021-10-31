#include <gtk/gtk.h>
#include <adwaita.h>

#include <openmg/backend/readmng.h>
#include <openmg/manga.h>
#include <openmg/view/list_view_manga.h>

static AdwHeaderBar *
create_headerbar (GtkBox *box);
static GtkBox *
create_main_box (AdwApplicationWindow *window);

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
    adw_leaflet_set_can_swipe_back (views_leaflet, 1);

    create_headerbar (box);

    mangas = mg_backend_readmng_get_featured_manga (readmng);
    list_view = create_list_view_mangas (mangas, views_leaflet);
    scroll = gtk_scrolled_window_new ();

    gtk_widget_set_valign (scroll, GTK_ALIGN_FILL);
    gtk_widget_set_vexpand (scroll, 1);
    gtk_scrolled_window_set_child (GTK_SCROLLED_WINDOW (scroll), GTK_WIDGET (list_view));
    
    adw_leaflet_append (views_leaflet, scroll);
    adw_leaflet_set_can_unfold (views_leaflet, false);

    gtk_box_append (box, GTK_WIDGET (views_leaflet));

    gtk_widget_show (window);
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

static AdwHeaderBar *
create_headerbar (GtkBox *box) {
    GtkWidget *title =
        adw_window_title_new ("Window", NULL);
    GtkWidget *header =
        adw_header_bar_new();
    adw_header_bar_set_title_widget(
            ADW_HEADER_BAR (header),
            GTK_WIDGET (title));
    gtk_box_append (box, header);
    GtkWidget *previous = gtk_button_new_from_icon_name ("go-previous-symbolic");

    adw_header_bar_pack_start (ADW_HEADER_BAR (header), previous);
 

    return ADW_HEADER_BAR (header);
}

int
main_view_run (int argc,
        char **argv)
{
    AdwApplication *app;
    int status;

    app = adw_application_new ("org.mangareader", G_APPLICATION_FLAGS_NONE);
    g_signal_connect (app, "activate", G_CALLBACK (activate), NULL);
    status = g_application_run (G_APPLICATION (app), argc, argv);
    g_object_unref (app);
    return status;
}
