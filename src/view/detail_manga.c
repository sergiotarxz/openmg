#include <gtk/gtk.h>
#include <adwaita.h>

#include <openmg/manga.h>

#include <openmg/backend/readmng.h>

#include <openmg/util/xml.h>
#include <openmg/util/gobject_utility_extensions.h>

#include <openmg/view/picture.h>
#include <openmg/view/detail_manga.h>
#include <openmg/view/list_view_chapter.h>

GtkBox *
create_detail_view (MgManga *manga) {
    MgBackendReadmng *readmng = mg_backend_readmng_new ();
    GtkWidget *scroll;
    GtkBox *detail_view = GTK_BOX (gtk_box_new (GTK_ORIENTATION_VERTICAL, 0));
    GtkBox *avatar_title_box = GTK_BOX (gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0));
    MgUtilXML *xml_util = mg_util_xml_new ();
    GtkLabel *manga_title = NULL;
    GtkLabel *manga_description = NULL;
    GtkListView *chapter_list = NULL;
    GtkPicture *manga_image = create_picture_from_url (
            mg_manga_get_image_url(manga), 200);
    char *title_text = mg_util_xml_get_title_text (
            xml_util, mg_manga_get_title (manga));
    char *description_text;

    scroll = gtk_scrolled_window_new ();

    mg_backend_readmng_retrieve_manga_details (readmng, manga);
    chapter_list = create_list_view_chapters (manga);
    description_text = mg_manga_get_description (manga);

    manga_title = GTK_LABEL (gtk_label_new (title_text));
    manga_description = GTK_LABEL (gtk_label_new (description_text));

    gtk_label_set_wrap (manga_title, 1);
    gtk_label_set_wrap (manga_description, 1);

    gtk_label_set_use_markup (GTK_LABEL (manga_title), 1);
    gtk_box_append (avatar_title_box, GTK_WIDGET (manga_image));
    gtk_box_append (avatar_title_box, GTK_WIDGET (manga_title));
    gtk_box_append (detail_view, GTK_WIDGET (avatar_title_box));
    gtk_box_append (detail_view, GTK_WIDGET (manga_description));
    gtk_scrolled_window_set_child (GTK_SCROLLED_WINDOW (scroll), GTK_WIDGET (chapter_list));
    g_object_set_property_int (G_OBJECT (scroll), "vexpand", 1);
    gtk_box_append (detail_view, GTK_WIDGET (scroll));

    return detail_view;
}
