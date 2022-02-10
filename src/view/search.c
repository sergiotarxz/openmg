#include <openmg/manga.h>

#include <openmg/backend/readmng.h>

#include <openmg/view/search.h>
#include <openmg/view/list_view_manga.h>

static void
search_text_changed (GtkEntry *entry,
        gpointer user_data);

typedef struct {
    GtkListView *list_view_mangas;
    ControlsAdwaita *controls;
} SearchTextData;

GtkWidget *
create_search_view (ControlsAdwaita *controls) {
    GtkWidget *search_view = gtk_box_new (GTK_ORIENTATION_VERTICAL, 10);

    GtkWidget *search_entry = gtk_entry_new ();
    GtkWidget *scroll = gtk_scrolled_window_new ();
    GListStore *mangas = g_list_store_new(MG_TYPE_MANGA);
    GtkListView *list_view_mangas;
    SearchTextData *search_text_data = g_malloc (sizeof *search_text_data);

    gtk_box_append (GTK_BOX (search_view), search_entry);

    list_view_mangas = create_list_view_mangas (mangas, controls);
    search_text_data->list_view_mangas = list_view_mangas;
    search_text_data ->controls = controls;
    g_signal_connect (search_entry, "activate",
            G_CALLBACK (search_text_changed), search_text_data);

    gtk_scrolled_window_set_child (GTK_SCROLLED_WINDOW (scroll),
            GTK_WIDGET (list_view_mangas));
    gtk_box_append (GTK_BOX (search_view), scroll);
    gtk_widget_set_valign (scroll, GTK_ALIGN_FILL);
    gtk_widget_set_vexpand (scroll, 1);
 
    return search_view;
}

static void
search_text_changed (GtkEntry *entry,
        gpointer user_data) {
    SearchTextData *search_text_data = (SearchTextData *) user_data;
    ControlsAdwaita *controls = search_text_data->controls;
    GtkListView *list_view_mangas = search_text_data->list_view_mangas;
    GtkEntryBuffer *buffer = gtk_entry_get_buffer (entry);
    MgBackendReadmng *readmng = mg_backend_readmng_new ();
    const char *search_string = gtk_entry_buffer_get_text (buffer);
    GListStore *mangas = mg_backend_readmng_search (readmng, search_string);
    for (size_t i = 0; i < controls->image_threads_len; i++) {
        g_cancellable_cancel (controls->image_threads[i]);
    }
    if (controls->image_threads) {
        g_free (controls->image_threads);
    }
    controls->image_threads = NULL;
    controls->image_threads_len = 0;
    controls->avoid_list_image_downloads = true;
    if (!mangas) return;
    GtkSingleSelection *selection = GTK_SINGLE_SELECTION (
            gtk_list_view_get_model (list_view_mangas));
    controls->avoid_list_image_downloads = false;
    gtk_single_selection_set_model (selection,
            G_LIST_MODEL (mangas));
}
