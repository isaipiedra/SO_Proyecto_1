#include <gtk/gtk.h>

//wrapper fuction that matches the singnature
void free_callback_data(gpointer data, GClosure *closure);

void clear_box(GtkBox *box);

void add_file_to_collection(GFile* file, GHashTable* collection);