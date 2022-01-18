#include <gtk/gtk.h>
#include <adwaita.h>

#include <openmg/manga.h>
#include <openmg/chapter.h>

#include <openmg/backend/readmng.h>

#include <openmg/util/xml.h>
#include <openmg/util/gobject_utility_extensions.h>

#include <openmg/view/picture.h>
#include <openmg/view/detail_manga.h>
#include <openmg/view/list_view_chapter.h>

typedef struct {
    GtkWidget *reorder;
    GtkWidget *hide_description;
    AdwHeaderBar *header;
} HeaderButtons;
static void
hide_controls (GtkWidget *detail_view,
        gpointer user_data);
static void
show_controls (GtkWidget *detail_view,
        gpointer user_data);

static void
reverse_list (GtkButton *reverse_button,
        gpointer user_data) {
    GtkListView *list_view = GTK_LIST_VIEW (user_data);
    GtkSingleSelection *selection = GTK_SINGLE_SELECTION 
            (gtk_list_view_get_model (list_view));
    GListStore *model = G_LIST_STORE (gtk_single_selection_get_model (selection));
    GListStore *new_model = g_list_store_new (MG_TYPE_MANGA_CHAPTER);
    guint size_model = g_list_model_get_n_items (G_LIST_MODEL (model));
    for (int i = size_model - 1; i >= 0; i--) {
        g_list_store_append (new_model, MG_MANGA_CHAPTER
                (g_list_model_get_item (G_LIST_MODEL (model), i)));
    }
    GtkSingleSelection *new_selection = gtk_single_selection_new
            (G_LIST_MODEL (new_model));
    gtk_list_view_set_model (list_view, GTK_SELECTION_MODEL (new_selection));
    g_clear_object (&model);
}

static void
toggle_folded (GtkButton *toggle_folded_button,
        gpointer user_data) {
    GtkBox *box = GTK_BOX (user_data);
    gboolean visible = gtk_widget_get_visible (GTK_WIDGET (box));
    gtk_widget_set_visible (GTK_WIDGET (box), !visible);
    if (visible) {
        gtk_button_set_icon_name (toggle_folded_button,
                "go-bottom-symbolic");
    } else {
        gtk_button_set_icon_name (toggle_folded_button,
                "go-top-symbolic");
    }
}

static void
picture_ready_manga_detail (GObject *source_object,
        GAsyncResult *res,
        gpointer user_data) {
    GTask *task =  G_TASK (res);
    GtkWidget *picture = g_task_propagate_pointer (task, NULL);
    GtkBox *box = GTK_BOX (source_object);
    if (GTK_IS_WIDGET (picture)) {
        gtk_box_prepend (box, GTK_WIDGET (picture));
    }
} 

GtkBox *
create_detail_view (MgManga *manga, ControlsAdwaita *controls) {
    AdwLeaflet *views_leaflet = controls->views_leaflet;
    AdwHeaderBar *header = controls->header;
    MgBackendReadmng *readmng = mg_backend_readmng_new ();
    GtkWidget *scroll;
    GtkBox *detail_view = GTK_BOX (gtk_box_new (GTK_ORIENTATION_VERTICAL, 0));
    GtkBox *avatar_title_box = GTK_BOX (gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0));
    GtkBox *foldable_manga_data = GTK_BOX (gtk_box_new (GTK_ORIENTATION_VERTICAL, 0));
    MgUtilXML *xml_util = mg_util_xml_new ();
    GtkLabel *manga_title = NULL;
    GtkLabel *manga_description = NULL;
    GtkButton *toggle_folded_button = GTK_BUTTON (gtk_button_new_from_icon_name
            ("go-top-symbolic"));
    GtkButton *reverse_list_button = GTK_BUTTON (gtk_button_new_from_icon_name
            ("network-transmit-receive-symbolic"));
    GtkListView *chapter_list = NULL;
    char *url_image =  mg_manga_get_image_url(manga);
    GtkPicture *picture = create_picture_from_url (url_image, 200,
            picture_ready_manga_detail, avatar_title_box, NULL);
    char *manga_title_text = mg_manga_get_title (manga);
    char *title_text = mg_util_xml_get_title_text (
            xml_util, manga_title_text);
    char *description_text;
    HeaderButtons *buttons = g_malloc (sizeof *buttons);

    scroll = gtk_scrolled_window_new ();
    g_object_set_property_int (G_OBJECT (scroll), "vexpand", 1);

    mg_backend_readmng_retrieve_manga_details (readmng, manga);
    chapter_list = create_list_view_chapters (manga, views_leaflet);


    g_signal_connect (G_OBJECT (toggle_folded_button), "clicked", G_CALLBACK (toggle_folded), foldable_manga_data);
    g_signal_connect (G_OBJECT (reverse_list_button), "clicked", G_CALLBACK (reverse_list), chapter_list);

    description_text = mg_manga_get_description (manga);
    manga_title = GTK_LABEL (gtk_label_new (title_text));
    manga_description = GTK_LABEL (gtk_label_new (description_text));

    gtk_label_set_wrap (manga_title, 1);
    gtk_label_set_wrap (manga_description, 1);
    gtk_widget_set_size_request (GTK_WIDGET (manga_description), 200, -1);

    gtk_label_set_use_markup (GTK_LABEL (manga_title), 1);
    if (picture) {
        gtk_box_append (avatar_title_box, GTK_WIDGET (picture));
    }
    gtk_box_append (avatar_title_box, GTK_WIDGET (manga_title));

    gtk_box_append (foldable_manga_data, GTK_WIDGET (avatar_title_box));
    gtk_box_append (foldable_manga_data, GTK_WIDGET (manga_description));

    gtk_box_append (detail_view, GTK_WIDGET (foldable_manga_data));


    gtk_scrolled_window_set_child (GTK_SCROLLED_WINDOW (scroll), GTK_WIDGET (chapter_list));
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroll), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);

    gtk_box_append (detail_view, scroll);

    buttons->header = header;
    buttons->hide_description = GTK_WIDGET (toggle_folded_button);
    buttons->reorder = GTK_WIDGET (reverse_list_button);

    g_object_ref (G_OBJECT (reverse_list_button));
    g_object_ref (G_OBJECT (toggle_folded_button));

    g_signal_connect (detail_view, "map", G_CALLBACK (show_controls),
            buttons);
    g_signal_connect (detail_view, "unmap", G_CALLBACK (hide_controls),
            buttons);

    g_clear_object (&readmng);
    g_free (url_image);
    g_free (manga_title_text);
    g_free (title_text);
    g_free (description_text);
    g_clear_object (&xml_util);
    return detail_view;
}

static void
hide_controls (GtkWidget *detail_view,
        gpointer user_data) {
    HeaderButtons *buttons = (HeaderButtons *) user_data;
    AdwHeaderBar *header = buttons->header;
    adw_header_bar_remove (header, GTK_WIDGET (buttons->hide_description));
    adw_header_bar_remove (header, GTK_WIDGET (buttons->reorder));
}

static void
show_controls (GtkWidget *detail_view,
        gpointer user_data) {
    HeaderButtons *buttons = (HeaderButtons *) user_data;
    AdwHeaderBar *header = buttons->header;
    adw_header_bar_pack_end (header, GTK_WIDGET (buttons->hide_description));
    adw_header_bar_pack_end (header, GTK_WIDGET (buttons->reorder));
}
