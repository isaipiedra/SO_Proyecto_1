#include "utils.h"
#include <stdbool.h>
#include <string.h>

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

bool is_valid_file_name(const char* name) {
    if (name == NULL) {
        return false;
    }

    size_t len = strlen(name);

    // empty or too long
    if (len == 0 || len > 255) {
        return false;
    }

    if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0) {
        return false;
    }

    // cannot end with space of period
    char last = name[len - 1];
    if (last == ' ' || last == '.') {
        return false;
    }

    // prohibited chars
    const char* invalid_chars = "/\\:*?\"<>|";

    for (size_t i = 0; i < len; i++) {
        char c = name[i];

        // non allowed control chars
        if ((unsigned char)c < 32) {
            return false;
        }

        if (strchr(invalid_chars, c) != NULL) {
            return false;
        }
    }

    return true;
}