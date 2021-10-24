#include <gtk/gtk.h>
#include <adwaita.h>

#include <readmng.h>
#include <manga.h>

AdwHeaderBar *
create_headerbar (GtkBox *box);
GtkBox *
create_main_box (AdwApplicationWindow *window);
GtkBox *
create_manga_container ();
AdwCarousel *
create_adw_caroulsel (GtkBox *box);

void
fill_carousel_of_mangas (AdwCarousel *carousel);

void
activate (AdwApplication *app,
        gpointer user_data)
{
    GtkWidget *window =
        adw_application_window_new (GTK_APPLICATION (app));
    GtkBox *box = create_main_box(
            ADW_APPLICATION_WINDOW
            (window));
    AdwCarousel *carousel;
    create_headerbar (box);

    carousel = create_adw_caroulsel (box);
    fill_carousel_of_mangas (carousel);

    gtk_widget_show (window);
}

void
fill_carousel_of_mangas (AdwCarousel *carousel) {
    struct Manga *mangas;
    struct Manga *manga;
    GtkBox *manga_container;
    size_t len_mangas = 0;

    mangas = retrieve_readmng_title_mangas (&len_mangas);
    for (int i = 0; i<len_mangas; i++) {
        GtkWidget *picture;
        GFileIOStream *iostream;
        GFile *tmp_image;
        GError *error = NULL;

        size_t size_downloaded_image = 0;
        char *downloaded_image;

        manga = &mangas[i];
        manga_container = create_manga_container (manga);
        adw_carousel_append (carousel, GTK_WIDGET (manga_container));
        downloaded_image = get_request (manga->image_url, &size_downloaded_image);
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
        gtk_box_append (manga_container, picture);
    }
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

AdwCarousel *
create_adw_caroulsel (GtkBox *box) {
    GtkWidget *carousel = adw_carousel_new ();
    gtk_box_append (box, carousel);
    return ADW_CAROUSEL (carousel);
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
