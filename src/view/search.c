#include <openmg/view/search.h>

GtkWidget *
create_search_view (ControlsAdwaita *controls) {
    GtkWidget *search_view = gtk_box_new (
            GTK_ORIENTATION_VERTICAL, 10);

    GtkWidget *search_entry = gtk_entry_new ();
    gtk_box_append (GTK_BOX (search_view), search_entry);;
    return search_view;
}
