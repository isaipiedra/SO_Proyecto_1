#include <string.h>
#include "outputNameEntry.h"
#include "utils.h"

static void on_wrong_name_alert(GObject *source_object, GAsyncResult *res, gpointer user_data) {
    GtkAlertDialog *alert = GTK_ALERT_DIALOG(source_object);
    GError *error = NULL;
    
    int button = gtk_alert_dialog_choose_finish(alert, res, &error);
    if (error) {
        g_error_free(error);
    }
}

static void show_warning_dialog(GtkWindow *parent) {
    GtkAlertDialog *dialog = gtk_alert_dialog_new("¡Warning!");
    
    gtk_alert_dialog_set_detail(dialog, "The saving path is not valid, resorting to default path");
    
    const char *buttons[] = {"Ok", NULL};
    gtk_alert_dialog_set_buttons(dialog, buttons);
    gtk_alert_dialog_set_cancel_button(dialog, 0); 
    
    gtk_alert_dialog_choose(dialog, parent, NULL, on_wrong_name_alert, NULL);
}

void on_update_output_entry(GtkEditable* editable, gpointer user_data){
    GtkLabel** lbl_ptr = (GtkLabel**) user_data;
    GtkLabel* folder_name = *lbl_ptr;

    if(!GTK_IS_LABEL(folder_name)) return;

    GtkEntry* entry = GTK_ENTRY(editable);

    GtkEntryBuffer* buffer = gtk_entry_get_buffer(entry);
    const char* text = gtk_entry_buffer_get_text(buffer);

    gtk_label_set_text(GTK_LABEL(folder_name), text);
}


void on_output_entry_focus_leave(GtkEventControllerFocus* self, gpointer user_data){
    (void) self;
    ON_OUTPUT_ENTRY_FOCUS_LEAVE_PARAMETERS* parameters = (ON_OUTPUT_ENTRY_FOCUS_LEAVE_PARAMETERS*) user_data;
    GtkLabel** lbl_pointer = parameters->folder_name;
    GtkLabel* folder_name = *lbl_pointer;
    GtkEntry* entry = parameters->entry;
    GtkWindow* window = parameters->window;

    GtkEntryBuffer* buffer = gtk_entry_get_buffer(entry);
    const char* text = gtk_entry_buffer_get_text(buffer);

    if(!is_valid_file_path(text)){
        show_warning_dialog(window);
        gtk_label_set_text(GTK_LABEL(folder_name), OUTPUT_FOLDER_DEFAULT_NAME);
        gtk_entry_buffer_set_text(buffer, OUTPUT_FOLDER_DEFAULT_NAME, strlen(OUTPUT_FOLDER_DEFAULT_NAME));
    }
}
