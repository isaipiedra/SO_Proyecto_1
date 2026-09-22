#include <stdlib.h>
#include <gtk/gtk.h>
#include <math.h>
#include <unistd.h>
#include <time.h>
#include <stdbool.h>

void set_drop_to_compress_in_box(GtkWidget* box, GHashTable* collection, GtkWidget* file_explorer, GtkWindow* window);
void set_drop_to_decompress_in_box(
    GtkWidget* box, 
    GtkWidget* bottom_section, 
    GtkWindow* window,
    GtkLabel* lbl_selected_file_name,
    GFile** selected_file,
    GtkEntry* output_path
);
