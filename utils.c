#include "utils.h"

void free_callback_data(gpointer data, GClosure *closure) {
    (void) closure;
    g_free(data);
}

void clear_box(GtkBox *box) {
    GtkWidget *child;
    while ((child = gtk_widget_get_first_child(GTK_WIDGET(box))) != NULL) {
        gtk_box_remove(box, child);
    }
}

void add_file_to_collection(GFile* file, GHashTable* collection){
        char* path = g_file_get_path(file);
        g_hash_table_add(collection, path);
}