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

void
fill_list_of_mangas (GtkListBox *list);

void
activate (AdwApplication *app,
        gpointer user_data)
{
    GtkWidget *window =
        adw_application_window_new (GTK_APPLICATION (app));
    GtkBox *box = create_main_box(
            ADW_APPLICATION_WINDOW
            (window));
    GtkListBox *list;
    create_headerbar (box);

    list = create_list_box (box);
    gtk_widget_set_vexpand (GTK_WIDGET (list), 1);
    fill_list_of_mangas (list);

    gtk_widget_show (window);
}

void
fill_list_of_mangas (GtkListBox *list) {
    MgManga **mangas;
    MgManga *manga;
    GtkWidget *row;
    size_t len_mangas = 0;

    
    MgBackendReadmng *readmng = mg_backend_readmng_new ();
    mangas = mg_backend_readmng_get_featured_manga (readmng, &len_mangas);
    for (int i = 0; i<len_mangas; i++) {
        GtkWidget *picture;
        GFileIOStream *iostream;
        GFile *tmp_image;
        GError *error = NULL;

        size_t size_downloaded_image = 0;
        char *downloaded_image;

        manga = mangas[i];

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
        row = gtk_list_box_row_new ();
        gtk_list_box_row_set_child (GTK_LIST_BOX_ROW (row), picture);
        gtk_list_box_append (list, row);
    }
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
