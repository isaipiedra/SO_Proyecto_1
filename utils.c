#include <stdbool.h>
#include <string.h>
#include <sys/stat.h>
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

static bool directory_exists(const char* path) {
    struct stat st;

    if (stat(path, &st) != 0) {
        return false;
    }

    return S_ISDIR(st.st_mode);
}

bool is_valid_file_path(const char* path) {
    if (path == NULL) {
        return false;
    }

    char path_copy[512];
    size_t len = strlen(path);

    if (len == 0 || len >= sizeof(path_copy)) {
        return false;
    }

    strcpy(path_copy, path);

    // separate the filename from the path
    char* last_slash = strrchr(path_copy, '/');

    char* file_name;
    char* dir_path;

    if (last_slash == NULL) {
        // theres is no directory
        file_name = path_copy;
        dir_path = NULL;
    } else {
        *last_slash = '\0';       // cuts in the last '/'
        dir_path = path_copy;     // starts before the last '/'
        file_name = last_slash + 1; // starts after the last '/'
    }

    if (!is_valid_file_name(file_name)) {
        return false;
    }

    if (dir_path != NULL && !directory_exists(dir_path)) {
        return false;
    }

    return true;
}