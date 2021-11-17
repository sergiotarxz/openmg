#include <gtk/gtk.h>

#include <openmg/manga.h>
#include <openmg/chapter.h>
#include <openmg/backend/readmng.h>
#include <openmg/view/list_view_chapter.h>
#include <openmg/view/chapter_view.h>

static void
activate_chapter (GtkListView *list_view, guint position, gpointer user_data);
static void
setup_list_view_chapter_list (GtkSignalListItemFactory *factory,
        GtkListItem *list_item,
        gpointer user_data);

GtkListView *
create_list_view_chapters (MgManga *manga, AdwLeaflet *views_leaflet) {
    GListStore *manga_chapter_list = NULL;
    GtkListView *list_view_chapters = NULL;
    GtkSingleSelection *selection = NULL;
    GtkListItemFactory *factory =
        gtk_signal_list_item_factory_new ();

    manga_chapter_list = mg_manga_get_chapter_list (manga);
    selection = gtk_single_selection_new
        (G_LIST_MODEL (manga_chapter_list));

    g_signal_connect (G_OBJECT (factory), "bind",
            G_CALLBACK (setup_list_view_chapter_list),
            NULL);

    list_view_chapters = GTK_LIST_VIEW (gtk_list_view_new (GTK_SELECTION_MODEL (selection),
                factory));
    gtk_list_view_set_single_click_activate (list_view_chapters, 1);
    g_signal_connect (G_OBJECT (list_view_chapters), "activate",
            G_CALLBACK (activate_chapter), views_leaflet);

    return list_view_chapters;
}

static void
setup_list_view_chapter_list (GtkSignalListItemFactory *factory,
        GtkListItem *list_item,
        gpointer user_data) {
    MgMangaChapter *manga_chapter = gtk_list_item_get_item (list_item);
    GtkBox *box = GTK_BOX (gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0));
    g_object_ref (G_OBJECT (box));
    GtkLabel *title = GTK_LABEL (gtk_label_new (mg_manga_chapter_get_title (manga_chapter)));
    GtkImage *icon = GTK_IMAGE (gtk_image_new_from_icon_name (
            "weather-clear-night-symbolic"));
    gtk_image_set_icon_size (icon, GTK_ICON_SIZE_LARGE);
    gtk_label_set_wrap (title, 1);
    gtk_box_append (box, GTK_WIDGET (icon));
    gtk_box_append (box, GTK_WIDGET (title));
    gtk_list_item_set_child (list_item, GTK_WIDGET (box));
}

static void
activate_chapter (GtkListView *list_view, guint position, gpointer user_data) {
    GtkSingleSelection *selection = GTK_SINGLE_SELECTION
        (gtk_list_view_get_model (list_view));
    GListModel *chapters = gtk_single_selection_get_model (selection);
    MgMangaChapter *chapter = MG_MANGA_CHAPTER (g_list_model_get_item (chapters, position));
    AdwLeaflet *views_leaflet = ADW_LEAFLET (user_data);
    setup_chapter_view (chapter, views_leaflet);
}
