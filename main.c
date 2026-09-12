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

    //load the css
    GtkCssProvider *css_provider = gtk_css_provider_new();
    GError *css_error = NULL;
    gtk_css_provider_load_from_path(css_provider, "styles.css", &css_error);
    if(css_error){
        g_warning("An error occurred while loading the css file");
    }
    gtk_style_context_add_provider_for_screen(
        gdk_screen_get_default(),
        GTK_STYLE_PROVIDER(css_provider),
        GTK_STYLE_PROVIDER_PRIORITY_USER
    );
    g_object_unref(css_provider);

    window = GTK_WINDOW(gtk_builder_get_object(builder, "window"));
    gtk_window_maximize(window);
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