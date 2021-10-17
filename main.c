#include <gtk/gtk.h>
#include <adwaita.h>

#include <mangafox.h>

AdwHeaderBar *
create_headerbar (GtkBox *box);
GtkBox *
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
	create_headerbar (box);

	gtk_widget_show (window);
}

GtkBox *
create_main_box (AdwApplicationWindow *window) {
	GtkWidget *box = gtk_box_new(
		GTK_ORIENTATION_VERTICAL,
		10);
	adw_application_window_set_content(
			window, 
			box);
	return GTK_BOX (box);
}

AdwHeaderBar *
create_headerbar (GtkBox *box) {
	GtkWidget *title =
		adw_window_title_new ("Window", NULL);
	GtkWidget *header =
		adw_header_bar_new();
	adw_header_bar_set_title_widget(
			ADW_HEADER_BAR (header),
			GTK_WIDGET (title));
	gtk_box_append (GTK_BOX (box), header);


	return ADW_HEADER_BAR (header);
}

int
main (int argc,
		char **argv)
{
	AdwApplication *app;
	retrieve_mangafox_title();
	int status;
	app = adw_application_new ("org.mangareader", G_APPLICATION_FLAGS_NONE);
	g_signal_connect (app, "activate", G_CALLBACK (activate), NULL);
	status = g_application_run (G_APPLICATION (app), argc, argv);
	g_object_unref (app);
	return status;
}
