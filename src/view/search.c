#include <openmg/manga.h>

#include <openmg/backend/readmng.h>

#include <openmg/view/search.h>
#include <openmg/view/list_view_manga.h>

static void
search_text_changed (GtkEditable *self,
        gpointer user_data);
static void
async_search_manga (GTask *task,
        gpointer source_object,
        gpointer task_data,
        GCancellable *cancellable);
static void
set_mangas_to_list_view (GObject *source_object,
        GAsyncResult *res,
        gpointer user_data);

GtkWidget *
create_search_view (ControlsAdwaita *controls) {
    GtkWidget *search_view = gtk_box_new (GTK_ORIENTATION_VERTICAL, 10);

    GtkWidget *search_entry = gtk_entry_new ();
    GtkWidget *scroll = gtk_scrolled_window_new ();
    GListStore *mangas = g_list_store_new(MG_TYPE_MANGA);
    GtkListView *list_view_mangas;

    gtk_box_append (GTK_BOX (search_view), search_entry);

    list_view_mangas = create_list_view_mangas (mangas, controls);
    g_signal_connect (search_entry, "changed",
            G_CALLBACK (search_text_changed), list_view_mangas);

    gtk_scrolled_window_set_child (GTK_SCROLLED_WINDOW (scroll),
            GTK_WIDGET (list_view_mangas));
    gtk_box_append (GTK_BOX (search_view), scroll);
    gtk_widget_set_valign (scroll, GTK_ALIGN_FILL);
    gtk_widget_set_vexpand (scroll, 1);
 
    return search_view;
}

static GCancellable *cancellable = NULL;

static void
search_text_changed (GtkEditable *editable,
        gpointer user_data) {
    if (cancellable) {
        g_cancellable_cancel (cancellable);
    }
    cancellable = g_cancellable_new ();
    GtkListView *list_view_mangas = GTK_LIST_VIEW (user_data);
    GTask *task = g_task_new (editable, cancellable, set_mangas_to_list_view,list_view_mangas);
    g_task_set_return_on_cancel (task, true);
    g_task_run_in_thread (task, async_search_manga);
}

static void
set_mangas_to_list_view (GObject *source_object,
        GAsyncResult *res,
        gpointer user_data) {
    GListStore *mangas = G_LIST_STORE (g_task_propagate_pointer (G_TASK (res), NULL));
    if (!mangas) return;
    GtkListView *list_view_mangas = GTK_LIST_VIEW (user_data);
    GtkSingleSelection *selection = GTK_SINGLE_SELECTION (
            gtk_list_view_get_model (list_view_mangas));
    gtk_single_selection_set_model (selection,
            G_LIST_MODEL (mangas));
}

static void
async_search_manga (GTask *task,
        gpointer source_object,
        gpointer task_data,
        GCancellable *cancellable) {
    GtkEntry *entry = GTK_ENTRY (source_object);
    GtkEntryBuffer *buffer = gtk_entry_get_buffer (entry);
    MgBackendReadmng *readmng = mg_backend_readmng_new ();
    const char *search_string = gtk_entry_buffer_get_text (buffer);
    GListStore *mangas = mg_backend_readmng_search (readmng, search_string);
    g_task_return_pointer (task, mangas, g_object_unref);
}
