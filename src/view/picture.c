#include <stdio.h>

#include <gtk/gtk.h>

#include <openmg/util/soup.h>
#include <openmg/util/gobject_utility_extensions.h>

GtkPicture *
create_picture_from_url (const char *const url, gint picture_size) {
    GtkPicture *picture = NULL;
    GFileIOStream *iostream;
    GFile *tmp_image;
    GError *error = NULL;
    GdkTexture *texture;

    printf("%s\n", url);

    size_t size_downloaded_image = 0;
    char *downloaded_image;

    MgUtilSoup *util_soup = mg_util_soup_new ();
    downloaded_image = mg_util_soup_get_request (util_soup,
            url, &size_downloaded_image);
    tmp_image = g_file_new_tmp ("mangareadertmpfileXXXXXX", &iostream, &error);
    if (error) {
        fprintf (stderr, "Unable to read file: %s\n", error->message);
        g_clear_error (&error);
        goto cleanup_create_picture_from_url;
    }
    g_output_stream_write (g_io_stream_get_output_stream (G_IO_STREAM (iostream)),
            downloaded_image, size_downloaded_image, NULL, &error);
    if (error) {
        fprintf (stderr, "Unable to write file: %s\n", error->message);
        g_clear_error (&error);
        goto cleanup_create_picture_from_url;
    }
    texture = gdk_texture_new_from_file (tmp_image, &error);
    if (error) {
        fprintf (stderr, "Texture malformed.");
        goto cleanup_create_picture_from_url;
    }
    picture = GTK_PICTURE (gtk_picture_new_for_paintable (GDK_PAINTABLE (texture)));
    g_object_set_property_int (G_OBJECT(picture), "height-request", picture_size);
    g_object_set_property_int (G_OBJECT(picture), "width-request", picture_size);
    g_object_set_property_int (G_OBJECT(picture), "margin-end", 5);

cleanup_create_picture_from_url:
    g_free (downloaded_image);
    g_clear_object (&util_soup);
    g_clear_object (&iostream);
    g_clear_object (&tmp_image);
    return picture;
}
