#ifndef __CYGWIN64__
#include <dlfcn.h>
#endif

#include <gtk/gtk.h>
#include <adwaita.h>

#include <openmg/view/controls.h>
#include <openmg/view/explore.h>
#include <openmg/view/search.h>

static AdwHeaderBar *
create_headerbar (GtkBox *box, ControlsAdwaita *controls, GtkButton **out_previous);
static GtkBox *
create_main_box (AdwApplicationWindow *window);
static void
go_back_view (GtkButton *previous, gpointer user_data);
typedef void (*swipe_back_t)(AdwLeaflet *, gboolean);
static AdwLeaflet *
create_explore_leaflet (ControlsAdwaita *controls, swipe_back_t swipe_back);
static AdwLeaflet *
create_search_leaflet (ControlsAdwaita *controls, swipe_back_t swipe_back);
static void
map_leaflet (GtkWidget *leaflet_widget, gpointer user_data);

static void
activate (AdwApplication *app,
        gpointer user_data)
{
    GtkWidget *window =
        adw_application_window_new (GTK_APPLICATION (app));
    GtkBox *box = create_main_box(
            ADW_APPLICATION_WINDOW
            (window));
    AdwViewStack *view_stack = ADW_VIEW_STACK (adw_view_stack_new ());
    ControlsAdwaita *controls = g_malloc (sizeof *controls);
    GtkButton *previous = NULL;
    AdwLeaflet *views_leaflet_explore;
    AdwLeaflet *views_leaflet_search;
    AdwHeaderBar *header_bar;

#ifndef __CYGWIN64__
    swipe_back_t swipe_back = (swipe_back_t) dlsym
        (NULL, "adw_leaflet_set_can_navigate_back");

    if (!swipe_back) {
        swipe_back = (swipe_back_t) dlsym
        (NULL, "adw_leaflet_set_can_swipe_back");
    }
#else
    swipe_back_t swipe_back = adw_leaflet_set_can_navigate_back;
#endif

    controls->is_set_previous = 0;
    controls->header = NULL;
    controls->view_stack = view_stack;


    views_leaflet_explore = create_explore_leaflet (controls, swipe_back);
    views_leaflet_search = create_search_leaflet (controls, swipe_back);
    header_bar = create_headerbar (box, controls, &previous);
    controls->header = header_bar;
    controls->previous = previous;

    AdwViewStackPage *explore_page = adw_view_stack_add_titled (view_stack, GTK_WIDGET (views_leaflet_explore),
            "explore",
            "Explore");
    AdwViewStackPage *search_page = adw_view_stack_add_titled (view_stack, GTK_WIDGET (views_leaflet_search),
            "search",
            "Search");
    g_signal_connect (G_OBJECT (views_leaflet_search), "map", G_CALLBACK (map_leaflet), controls);
    g_signal_connect (G_OBJECT (views_leaflet_explore), "map", G_CALLBACK (map_leaflet), controls);

    adw_view_stack_page_set_icon_name (explore_page, "view-list-symbolic");
    adw_view_stack_page_set_icon_name (search_page, "system-search-symbolic");

    gtk_box_append (box, GTK_WIDGET (view_stack));

    gtk_widget_show (window);
}

static void
map_leaflet (GtkWidget *leaflet_widget, gpointer user_data) {
    ControlsAdwaita *controls = (ControlsAdwaita *) user_data;
    AdwLeaflet *leaflet = ADW_LEAFLET (leaflet_widget);
    controls->views_leaflet = leaflet;
}

static AdwLeaflet *
create_search_leaflet (ControlsAdwaita *controls, swipe_back_t swipe_back) {
    AdwLeaflet *views_leaflet = ADW_LEAFLET (adw_leaflet_new ());
    GtkWidget *search_view;
    swipe_back (views_leaflet, 1);
    search_view = create_search_view (controls); 

    adw_leaflet_append (views_leaflet, search_view);

    adw_leaflet_set_can_unfold (views_leaflet, false);

    return views_leaflet;
}

static AdwLeaflet *
create_explore_leaflet (ControlsAdwaita *controls, swipe_back_t swipe_back) {
    AdwLeaflet *views_leaflet_explore = ADW_LEAFLET (adw_leaflet_new ());
    GtkWidget *explore_view;

    controls->views_leaflet = views_leaflet_explore;

    swipe_back (views_leaflet_explore, 1);
    explore_view = create_explore_view (controls);
    adw_leaflet_append (views_leaflet_explore, explore_view);
    adw_leaflet_set_can_unfold (views_leaflet_explore, false);

    return views_leaflet_explore;
}

static GtkBox *
create_main_box (AdwApplicationWindow *window) {
    GtkWidget *box = gtk_box_new (GTK_ORIENTATION_VERTICAL,
            10);
    adw_application_window_set_content(
            window,
            box);
    return GTK_BOX (box);
}

static AdwHeaderBar *
create_headerbar (GtkBox *box, ControlsAdwaita *controls, GtkButton **out_previous) {
    GtkWidget *header = adw_header_bar_new();
    GValue value = G_VALUE_INIT;
    GtkWidget *view_switcher = adw_view_switcher_new ();

    adw_view_switcher_set_stack (ADW_VIEW_SWITCHER (view_switcher),
            controls->view_stack);
    adw_header_bar_set_title_widget (ADW_HEADER_BAR (header), GTK_WIDGET (view_switcher));

    g_value_init (&value, GTK_TYPE_WIDGET);
    g_value_unset (&value);

    gtk_box_append (box, header);

    GtkWidget *previous = gtk_button_new_from_icon_name ("go-previous-symbolic");
    g_signal_connect (G_OBJECT (previous), "clicked", G_CALLBACK (go_back_view),
            controls);

    if (out_previous) {
        *out_previous = GTK_BUTTON (previous);
        g_object_ref (*out_previous);
    }

    return ADW_HEADER_BAR (header);
}

static void
go_back_view (GtkButton *previous, gpointer user_data) {
    ControlsAdwaita *controls = (ControlsAdwaita *) user_data;
    AdwLeaflet *views_leaflet = controls->views_leaflet;
    adw_leaflet_navigate (views_leaflet, ADW_NAVIGATION_DIRECTION_BACK);
}
    int
main_view_run (int argc,
        char **argv)
{
    AdwApplication *app = adw_application_new ("me.sergiotarxz.mangareader", G_APPLICATION_FLAGS_NONE);
    int status = 0;
    g_signal_connect (app, "activate", G_CALLBACK (activate), NULL);
    status = g_application_run (G_APPLICATION (app), argc, argv);
    g_clear_object (&app);
    return status;
}
