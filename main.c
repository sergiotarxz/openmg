#include <gtk/gtk.h>
#include <adwaita.h>

#include <openmg/backend/readmng.h>
#include <openmg/manga.h>

#include <readmng.h>
#include <manga.h>

AdwHeaderBar *
create_headerbar (GtkBox *box);
GtkBox *
create_main_box (AdwApplicationWindow *window);
GtkBox *
create_manga_container ();
GtkListBox *
create_list_box (GtkBox *box);
void
g_object_set_property_int(GObject *object, char *property_key, int value);
static void
setup_list_view_mangas(GtkSignalListItemFactory *factory,
        GtkListItem *list_item,
        gpointer user_data);

void
activate (AdwApplication *app,
        gpointer user_data)
{
    GtkWidget *window =
        adw_application_window_new (GTK_APPLICATION (app));
    GtkBox *box = create_main_box(
            ADW_APPLICATION_WINDOW
            (window));
    create_headerbar (box);

    GListStore *mangas;

    MgBackendReadmng *readmng = mg_backend_readmng_new ();
    mangas = mg_backend_readmng_get_featured_manga (readmng);
    GtkSingleSelection *selection = gtk_single_selection_new (G_LIST_MODEL (mangas));
    GtkListItemFactory *factory = gtk_signal_list_item_factory_new ();
    g_signal_connect (G_OBJECT (factory), "bind",
        G_CALLBACK (setup_list_view_mangas),
        NULL);
    GtkWidget *list_view = gtk_list_view_new (GTK_SELECTION_MODEL (selection),
            factory);
    gtk_box_append (box, list_view);

    gtk_widget_show (window);
}

static void
setup_list_view_mangas(GtkSignalListItemFactory *factory,
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

    downloaded_image = get_request (mg_manga_get_image_url(manga), &size_downloaded_image);
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

void
g_object_set_property_int(GObject *object, char *property_key, int value) {
    GValue property = G_VALUE_INIT;
    g_value_init (&property, G_TYPE_INT);
    g_value_set_int (&property, value);
    g_object_set_property (object, property_key, &property);
}

GtkBox *
create_manga_container () {
    GtkBox *manga_container;
    manga_container = GTK_BOX (gtk_box_new(
                GTK_ORIENTATION_HORIZONTAL,
                0));
    return manga_container;
}

GtkBox *
create_main_box (AdwApplicationWindow *window) {
    GtkWidget *box = gtk_box_new(
            GTK_ORIENTATION_VERTICAL,
            10);
    adw_application_window_set_content(
            window,
            box);
    return GTK_BOX (box);
}

GtkListBox *
create_list_box (GtkBox *box) {
    GtkWidget *list = gtk_list_box_new ();
    gtk_box_append (box, list);
    return GTK_LIST_BOX (list);
}

AdwHeaderBar *
create_headerbar (GtkBox *box) {
    GtkWidget *title =
        adw_window_title_new ("Window", NULL);
    GtkWidget *header =
        adw_header_bar_new();
    adw_header_bar_set_title_widget(
            ADW_HEADER_BAR (header),
            GTK_WIDGET (title));
    gtk_box_append (box, header);


    return ADW_HEADER_BAR (header);
}

int
main (int argc,
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
