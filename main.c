#include <stdlib.h>
#include <gtk/gtk.h>
#include <math.h>
#include <unistd.h>
#include <time.h>
#include <stdbool.h>

GtkBuilder *builder;
GtkWindow *window;

static void activate(GtkApplication *app) {
   
    builder = gtk_builder_new_from_file("build.glade");
    if (!builder) {
        g_error("Failed to load Glade file");
        return;
    }

    window = GTK_WINDOW(gtk_builder_get_object(builder, "window"));
    if (!window) {
        g_error("Failed to get window from builder");
        return;
    }
    
    gtk_window_set_application(window, GTK_APPLICATION(app));
    gtk_widget_show_all(GTK_WIDGET(window));
}

int main(int argc, char *argv[]) {
    GtkApplication *app = gtk_application_new("org.gtk.example", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);

    int status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);

    return status;
}