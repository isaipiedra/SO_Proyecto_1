#include <gtk/gtk.h>
void on_update_output_entry(GtkEditable* editable, gpointer user_data);

typedef struct{
    GtkEntry* entry;
    GtkLabel** folder_name;
    GtkWindow* window;
}ON_OUTPUT_ENTRY_FOCUS_LEAVE_PARAMETERS;
void on_output_entry_focus_leave(GtkEventControllerFocus* self, gpointer user_data);

typedef struct{
    GtkEntry* entry;
    GtkWindow* window;
}ON_OUTPUT_ENTRY_DECOMP_FOCUS_LEAVE_PARAMETERS;
void on_output_entry_decomp_focus_leave(GtkEventControllerFocus* self, gpointer user_data);