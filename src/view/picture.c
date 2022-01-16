#include <stdio.h>

#include <glib.h>
#include <gtk/gtk.h>
#include <sqlite3.h>

#include <openmg/util/soup.h>
#include <openmg/util/gobject_utility_extensions.h>

#include <openmg/database.h>
#include <openmg/database/statement.h>

const char *const IMAGE_CACHE_FORMAT_STRING =
"%s/.cache/openmg/%s";

typedef struct {
    char *url;
    gint picture_size;
} PictureThreadAttributes;

static char *
generate_cache_file_name (void);
GFile *
get_image_for_url (const char *url);

static void
threaded_picture_recover (GTask *task, gpointer source_object,
        gpointer task_data, GCancellable *cancellable) {
    PictureThreadAttributes *attrs = (PictureThreadAttributes *) task_data; 
    const char *url = attrs->url;
    gint picture_size = attrs->picture_size;
    GFileIOStream *iostream = NULL;
    GFile *image = NULL;
    GError *error = NULL;
    GdkTexture *texture = NULL;
    GtkPicture *picture = NULL;

    size_t size_downloaded_image = 0;
    char *downloaded_image = NULL;

    MgUtilSoup *util_soup = mg_util_soup_new ();
    static GMutex mutex;
    g_mutex_lock (&mutex);
    image = get_image_for_url (url);
    g_mutex_unlock (&mutex);
    if (!g_file_query_exists (image, NULL)) {
        downloaded_image =
            mg_util_soup_get_request
            (util_soup,
             url, &size_downloaded_image);
        iostream = g_file_create_readwrite (image, G_FILE_CREATE_NONE,
                NULL, &error);
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
    }
    texture = gdk_texture_new_from_file (image, &error);
    if (error) {
        fprintf (stderr, "Texture malformed.");
        goto cleanup_create_picture_from_url;
    }
    picture = GTK_PICTURE (gtk_picture_new_for_paintable (GDK_PAINTABLE (texture)));
    if (GTK_IS_WIDGET (picture)) {
        g_object_set_property_int (G_OBJECT(picture), "height-request", picture_size);
        g_object_set_property_int (G_OBJECT(picture), "width-request", picture_size);
    }

cleanup_create_picture_from_url:
    if (downloaded_image) {
        g_free (downloaded_image);
    }
    g_clear_object (&util_soup);
    if (iostream) {
        g_clear_object (&iostream);
    }
    g_clear_object (&image);
    g_task_return_pointer (task, picture, NULL);
}

static void
free_picture_thread_attributes (gpointer user_data) {
    PictureThreadAttributes *attrs = (PictureThreadAttributes *) user_data;
    g_free (attrs->url);
    g_free (attrs);
}

void
create_picture_from_url (const char *const url, gint picture_size,
        GAsyncReadyCallback ready, gpointer source_object,
        gpointer callback_data) {
    GTask *task = g_task_new (source_object, NULL, ready, callback_data);
    size_t url_len = strlen (url) + 1;

    PictureThreadAttributes *attrs = g_malloc (sizeof *attrs);

    attrs->url = g_malloc (url_len * sizeof *url);
    snprintf (attrs->url, url_len, "%s", url);
    attrs->picture_size = picture_size;
    g_task_set_task_data (task, attrs, free_picture_thread_attributes);
    g_task_run_in_thread (task, threaded_picture_recover);

}

GFile *
get_image_for_url (const char *url) {
    GFile *image;
    MgDatabase *db = mg_database_new ();
    MgDatabaseStatement *statement = mg_database_prepare (db,
            "select file from images where url = ?;", NULL);
    char *file_name = NULL;
    mg_database_statement_bind_text (statement, 1, url);
    if (mg_database_statement_step (statement) == SQLITE_ROW) {
        const char *file_name = (const char *) mg_database_statement_column_text
            (statement, 0);
        image = g_file_new_for_path (file_name);
        goto cleanup_get_image_for_url;
    }
    g_clear_object (&statement);
    file_name = generate_cache_file_name ();
    statement = mg_database_prepare (db,
            "insert into images (url, file) values (?, ?);", NULL); 
    mg_database_statement_bind_text (statement, 1, url);
    mg_database_statement_bind_text (statement, 2, file_name);
    mg_database_statement_step (statement);
    image = g_file_new_for_path (file_name);
cleanup_get_image_for_url:
    if (file_name) {
        g_free (file_name);
    }
    g_clear_object (&statement);
    g_clear_object (&db);
    return image;
}

static char *
generate_cache_file_name (void) {
    const char *home_dir = g_get_home_dir();
    char *file_basename = g_uuid_string_random ();
    char *file_path;
    char *file_path_directory;
    size_t file_path_len;
    file_path_len = snprintf
        (NULL, 0, IMAGE_CACHE_FORMAT_STRING,
         home_dir, file_basename);
    file_path = g_malloc
        (sizeof *file_path
         * (file_path_len + 1));
    snprintf (file_path, file_path_len,
            IMAGE_CACHE_FORMAT_STRING,
            home_dir, file_basename);
    file_path_directory = g_path_get_dirname
        (file_path);
    g_mkdir_with_parents
        (file_path_directory, 00755);
    return file_path;
}
