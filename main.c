#include "dragAndDrop.h"
#include "widgets.h"

GtkBuilder *builder;

static GHashTable* dropped_files = NULL;

static void set_up_widgets(GtkBuilder* builder){
    // ------------- drag and drop area -------------

    dropped_files = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);
    GtkWidget *dnd_box = GTK_WIDGET(gtk_builder_get_object(builder, "box_dnd_container"));
    set_drop_in_box(dnd_box, dropped_files);
}

static void activate(GtkApplication *app) {

    //load the css
    GtkCssProvider *css_provider = gtk_css_provider_new();
    gtk_css_provider_load_from_path(css_provider, "styles.css");
    gtk_style_context_add_provider_for_display(
        gdk_display_get_default(),
        GTK_STYLE_PROVIDER(css_provider),
        GTK_STYLE_PROVIDER_PRIORITY_USER
    );
    g_object_unref(css_provider);

    //create builder from Cambalache
    GtkBuilder *builder = gtk_builder_new();
    gtk_builder_add_from_file(builder, "UI/builder.ui", NULL);

    GObject* window = gtk_builder_get_object(builder, "window");
    if (!window) {
        g_error("Failed to get window from builder");
        return;
    }
    gtk_window_maximize(GTK_WINDOW(window));

    set_up_widgets(builder);

    GObject* box_file_list = gtk_builder_get_object(builder, "box_file_explorer");

    GList *inner_files = NULL;
    inner_files = g_list_append(inner_files, file_node_new_file("readme.txt"));

    GList *root_children = NULL;
    root_children = g_list_append(root_children, file_node_new_folder("subfolder", inner_files));
    root_children = g_list_append(root_children, file_node_new_file("notes.txt"));

    FileNode* root = file_node_new_folder("Output", root_children);
    GtkWidget* tree_widget = build_node_widget(root, 0);
    gtk_box_append(GTK_BOX(box_file_list), tree_widget);
    
    gtk_window_set_application(GTK_WINDOW(window), app);
    gtk_widget_set_visible(GTK_WIDGET(window), TRUE);

    g_object_unref(builder);

}


int main(int argc, char *argv[]) {
    GtkApplication *app = gtk_application_new("org.gtk.example", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);

    int status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);

    return status;
}