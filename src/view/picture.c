#include <stdio.h>

#include <gtk/gtk.h>

#include <openmg/util/soup.h>
#include <openmg/util/gobject_utility_extensions.h>

GtkPicture *
create_picture_from_url (char *url, gint picture_height) {
    GtkPicture *picture;
    GFileIOStream *iostream;
    GFile *tmp_image;
    GError *error = NULL;

    size_t size_downloaded_image = 0;
    char *downloaded_image;

    MgUtilSoup *util_soup = mg_util_soup_new ();
    downloaded_image = mg_util_soup_get_request (util_soup,
            url, &size_downloaded_image);
    tmp_image = g_file_new_tmp ("mangareadertmpfileXXXXXX", &iostream, &error);
    if (error) {
        fprintf (stderr, "Unable to read file: %s\n", error->message);
        return NULL;
    }
    error = NULL;
    g_output_stream_write (g_io_stream_get_output_stream (G_IO_STREAM (iostream)),
            downloaded_image, size_downloaded_image, NULL, &error);
    if (error) {
        fprintf (stderr, "Unable to write file: %s\n", error->message);
        return NULL;
    }
    picture = GTK_PICTURE (gtk_picture_new_for_file (tmp_image));
    g_object_set_property_int (G_OBJECT(picture), "height-request", picture_height);
    return picture;
}
