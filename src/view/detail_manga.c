#include <gtk/gtk.h>

#include <openmg/manga.h>

#include <openmg/view/picture.h>
#include <openmg/view/detail_manga.h>

GtkBox *
create_detail_view (MgManga *manga) {
    GtkBox *detail_view = GTK_BOX (gtk_box_new (GTK_ORIENTATION_VERTICAL, 0));
    GtkBox *descriptive_box = GTK_BOX (gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0));
    GtkPicture *manga_image = create_picture_from_url (
            mg_manga_get_image_url(manga), 200);
    gtk_box_append (descriptive_box, GTK_WIDGET (manga_image));
    gtk_box_append (detail_view, GTK_WIDGET (descriptive_box));
    return detail_view;
}
