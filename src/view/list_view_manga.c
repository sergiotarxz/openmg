#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <adwaita.h>

#include <openmg/manga.h>
#include <openmg/util/soup.h>
#include <openmg/util/gobject_utility_extensions.h>

#include <openmg/view/detail_manga.h>
#include <openmg/view/list_view_manga.h>
#include <openmg/view/picture.h>

typedef struct {
    GtkListView *list_view;
    AdwLeaflet *views_leaflet;
} ActivationValues;

static void
setup_list_view_mangas (GtkSignalListItemFactory *factory,
        GtkListItem *list_item,
        gpointer user_data);

static void
manga_selected (GtkListView *list_view, 
        guint position,
        gpointer user_data) {
    AdwLeaflet *views_leaflet = ADW_LEAFLET (user_data);
    GtkSingleSelection *selection = GTK_SINGLE_SELECTION 
        (gtk_list_view_get_model (list_view));
    GListModel *mangas = gtk_single_selection_get_model (selection);
    MgManga *manga = MG_MANGA (g_list_model_get_item (mangas, position));

    GtkWidget *widget = adw_leaflet_get_adjacent_child (views_leaflet,
            ADW_NAVIGATION_DIRECTION_FORWARD);

    while (widget) {
        adw_leaflet_remove (views_leaflet, widget);
        widget = adw_leaflet_get_adjacent_child (views_leaflet,
                ADW_NAVIGATION_DIRECTION_FORWARD);
    }

    GtkBox *detail_view = create_detail_view (manga, views_leaflet);
    adw_leaflet_append (views_leaflet, GTK_WIDGET (detail_view));
    adw_leaflet_navigate (views_leaflet, ADW_NAVIGATION_DIRECTION_FORWARD);
}

static void
setup_list_view_mangas (GtkSignalListItemFactory *factory,
        GtkListItem *list_item,
        gpointer user_data) {
    MgManga *manga = gtk_list_item_get_item (list_item);
    GtkBox *box = GTK_BOX (gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0));
    char *manga_title = mg_manga_get_title (manga);
    char *image_url = mg_manga_get_image_url (manga);

    GtkWidget *label = gtk_label_new (manga_title);
    GtkWidget *picture = GTK_WIDGET (
            create_picture_from_url (image_url, 100));

    gtk_box_append (box, picture);
    gtk_box_append (box, label);
    gtk_list_item_set_child (list_item, GTK_WIDGET (box));
    g_free (manga_title);
    g_free (image_url);
}

GtkListView *
create_list_view_mangas (GListStore *mangas, AdwLeaflet *views_leaflet) {
    GtkSingleSelection *selection = gtk_single_selection_new (G_LIST_MODEL (mangas));
    GtkListItemFactory *factory = gtk_signal_list_item_factory_new ();
    GtkListView *list_view_manga = NULL;

    g_signal_connect (G_OBJECT (factory), "bind",
            G_CALLBACK (setup_list_view_mangas),
            views_leaflet);

    list_view_manga = GTK_LIST_VIEW (gtk_list_view_new (GTK_SELECTION_MODEL (selection),
                factory));
    g_object_set_property_int (G_OBJECT (list_view_manga),
            "single-click-activate", 1);

    g_signal_connect (G_OBJECT (list_view_manga), "activate",
            G_CALLBACK (manga_selected), views_leaflet);
    return list_view_manga;
}
