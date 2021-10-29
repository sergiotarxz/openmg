#include <gtk/gtk.h>
#include <adwaita.h>

#include <openmg/backend/readmng.h>
#include <openmg/manga.h>
#include <openmg/view/list_view_manga.h>

#include <readmng.h>
#include <manga.h>

static AdwHeaderBar *
create_headerbar (GtkBox *box);
static GtkBox *
create_main_box (AdwApplicationWindow *window);

static void
activate (AdwApplication *app,
        gpointer user_data)
{
    GtkWidget *window =
        adw_application_window_new (GTK_APPLICATION (app));
    GtkBox *box = create_main_box(
            ADW_APPLICATION_WINDOW
            (window));
    GListStore *mangas;
    MgBackendReadmng *readmng = mg_backend_readmng_new ();
    GtkListView *list_view;
    GtkWidget *scroll;
    AdwViewStack *views_stack = ADW_VIEW_STACK (adw_view_stack_new ());

    create_headerbar (box);

    mangas = mg_backend_readmng_get_featured_manga (readmng);
    list_view = create_list_view_mangas (mangas);
    scroll = gtk_scrolled_window_new ();

    gtk_widget_set_valign (scroll, GTK_ALIGN_FILL);
    gtk_widget_set_vexpand (scroll, 1);
    gtk_scrolled_window_set_child (GTK_SCROLLED_WINDOW (scroll), GTK_WIDGET (list_view));
    
    adw_view_stack_add_named (views_stack, scroll, "manga-list");
    adw_view_stack_set_visible_child_name (views_stack, "manga-list");
    AdwViewStackPage *page = adw_view_stack_get_page (views_stack, scroll);
    adw_view_stack_page_set_title (page, "Manga List");

    gtk_box_append (box, GTK_WIDGET (views_stack));

    gtk_widget_show (window);
}

static GtkBox *
create_main_box (AdwApplicationWindow *window) {
    GtkWidget *box = gtk_box_new(
            GTK_ORIENTATION_VERTICAL,
            10);
    adw_application_window_set_content(
            window,
            box);
    return GTK_BOX (box);
}

static AdwHeaderBar *
create_headerbar (GtkBox *box) {
    GtkWidget *title =
        adw_window_title_new ("Window", NULL);
    GtkWidget *header =
        adw_header_bar_new();
    adw_header_bar_set_title_widget(
            ADW_HEADER_BAR (header),
            GTK_WIDGET (title));
    gtk_box_append (box, header);
    GtkWidget *previous = gtk_button_new_from_icon_name ("go-previous-symbolic");

    GValue margin_left = G_VALUE_INIT;
    g_value_init (&margin_left, G_TYPE_INT);
    g_value_set_int(&margin_left, 10);
    g_object_set_property (G_OBJECT (previous), "margin-start", &margin_left);
    adw_header_bar_pack_start (ADW_HEADER_BAR (header), previous);
 

    return ADW_HEADER_BAR (header);
}

int
main_view_run (int argc,
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