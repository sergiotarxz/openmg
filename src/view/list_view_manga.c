#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <adwaita.h>

#include <openmg/manga.h>
#include <openmg/util/soup.h>

#include <openmg/view/list_view_manga.h>

static void
g_object_set_property_int(GObject *object, char *property_key, int value);
static void
setup_list_view_mangas (GtkSignalListItemFactory *factory,
        GtkListItem *list_item,
        gpointer user_data);

typedef struct {
    GListStore *mangas;
    AdwLeaflet *views_leaflet;
} MangaPressedValues;

static void
manga_selected (GtkListView *list_view, 
        guint position,
        gpointer user_data) {
    MangaPressedValues *manga_pressed_values = (MangaPressedValues *) user_data;
    AdwLeaflet *views_leaflet = manga_pressed_values->views_leaflet;
    GtkBox *box = GTK_BOX (gtk_box_new (GTK_ORIENTATION_VERTICAL, 0));
    adw_leaflet_append (views_leaflet, GTK_WIDGET (box));
    adw_leaflet_navigate (views_leaflet, ADW_NAVIGATION_DIRECTION_FORWARD);
}

static void
setup_list_view_mangas (GtkSignalListItemFactory *factory,
        GtkListItem *list_item,
        gpointer user_data) {
    MgManga *manga = gtk_list_item_get_item (list_item);
    GtkBox *box = GTK_BOX (gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0));

    GtkWidget *label = gtk_label_new (mg_manga_get_title (manga));
    GtkWidget *picture;

    GFileIOStream *iostream;
    GFile *tmp_image;
    GError *error = NULL;

    size_t size_downloaded_image = 0;
    char *downloaded_image;

    MgUtilSoup *util_soup = mg_util_soup_new ();
    downloaded_image = mg_util_soup_get_request (util_soup, mg_manga_get_image_url(manga),
            &size_downloaded_image);
    tmp_image = g_file_new_tmp ("mangareadertmpfileXXXXXX",
            &iostream,
            &error
            );
    if (error) {
        fprintf (stderr, "Unable to read file: %s\n", error->message);
        return;
    }
    error = NULL;
    g_output_stream_write (g_io_stream_get_output_stream (G_IO_STREAM (iostream)),
            downloaded_image, size_downloaded_image, NULL, &error);
    if (error) {
        fprintf (stderr, "Unable to write file: %s\n", error->message);
        return;
    }
    picture = gtk_picture_new_for_file (tmp_image);
    g_object_set_property_int (G_OBJECT(picture), "height-request", 200);
    gtk_box_append (box, picture);
    gtk_box_append (box, label);
    gtk_list_item_set_child (list_item, GTK_WIDGET (box));
}

GtkListView *
create_list_view_mangas (GListStore *mangas, AdwLeaflet *views_leaflet) {
    GtkSingleSelection *selection = gtk_single_selection_new (G_LIST_MODEL (mangas));
    GtkListItemFactory *factory = gtk_signal_list_item_factory_new ();
    MangaPressedValues *manga_pressed_values = NULL;
    GtkListView *list_view_manga = NULL;

    manga_pressed_values = g_malloc (sizeof *manga_pressed_values);
    manga_pressed_values->mangas = mangas;
    manga_pressed_values->views_leaflet = views_leaflet;

    g_signal_connect (G_OBJECT (factory), "bind",
            G_CALLBACK (setup_list_view_mangas),
            views_leaflet);

    list_view_manga = GTK_LIST_VIEW (gtk_list_view_new (GTK_SELECTION_MODEL (selection),
                factory));

    g_signal_connect (G_OBJECT (list_view_manga), "activate",
            G_CALLBACK (manga_selected), manga_pressed_values);
    return list_view_manga;
}

static void
g_object_set_property_int(GObject *object, char *property_key, int value) {
    GValue property = G_VALUE_INIT;
    g_value_init (&property, G_TYPE_INT);
    g_value_set_int (&property, value);
    g_object_set_property (object, property_key, &property);
}


