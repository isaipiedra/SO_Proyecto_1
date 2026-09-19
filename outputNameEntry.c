#include "outputNameEntry.h"

void on_update_output_entry(GtkEditable* editable, gpointer user_data){
    GtkLabel** lbl_ptr = (GtkLabel**) user_data;
    GtkLabel* folder_name = *lbl_ptr;

    if(!GTK_IS_LABEL(folder_name)) return;

    GtkEntry* entry = GTK_ENTRY(editable);

    GtkEntryBuffer* buffer = gtk_entry_get_buffer(entry);
    const char* text = gtk_entry_buffer_get_text(buffer);

    gtk_label_set_text(GTK_LABEL(folder_name), text);
}
