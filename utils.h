#include <gtk/gtk.h>

#define OUTPUT_FOLDER_DEFAULT_NAME "Output"

//wrapper fuction that matches the singnature
void free_callback_data(gpointer data, GClosure *closure);

void show_warning_dialog(GtkWindow *parent, char* message);

void clear_box(GtkBox *box);

void add_file_to_collection(GFile* file, GHashTable* collection);

bool is_valid_file_name(const char* name);

bool is_valid_file_path(const char* path);

bool is_directory(const char* path);

bool has_extension(const char* path, const char* extension);