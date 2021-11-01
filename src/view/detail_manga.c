#include <gtk/gtk.h>
#include <adwaita.h>

#include <openmg/manga.h>

#include <openmg/util/xml.h>

#include <openmg/view/picture.h>
#include <openmg/view/detail_manga.h>

GtkBox *
create_detail_view (MgManga *manga) {
    GtkBox *detail_view = GTK_BOX (gtk_box_new (GTK_ORIENTATION_VERTICAL, 0));
    GtkBox *descriptive_box = GTK_BOX (gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0));
    MgUtilXML *xml_util = mg_util_xml_new ();
    GtkLabel *manga_title = NULL;
    GtkPicture *manga_image = create_picture_from_url (
            mg_manga_get_image_url(manga), 200);
    char *title_text = mg_util_xml_get_title_text (
            xml_util, mg_manga_get_title (manga));
    manga_title = GTK_LABEL (gtk_label_new (title_text));
    gtk_label_set_wrap (manga_title, 1);
    gtk_label_set_use_markup (GTK_LABEL (manga_title), 1);
    gtk_box_append (descriptive_box, GTK_WIDGET (manga_image));
    gtk_box_append (descriptive_box, GTK_WIDGET (manga_title));
    gtk_box_append (detail_view, GTK_WIDGET (descriptive_box));
    return detail_view;
}
