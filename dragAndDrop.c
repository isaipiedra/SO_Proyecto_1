#include <stdlib.h>
#include <gtk/gtk.h>
#include <math.h>
#include <unistd.h>
#include <time.h>
#include <stdbool.h>
#include "dragAndDrop.h"
#include "utils.h"
#include "fileExplorer.h"

static GdkDragAction on_drag_enter(GtkDropTarget* target, double x, double y, gpointer data){
    (void) data;
    (void) x;
    (void) y;
    GtkWidget *widget = gtk_event_controller_get_widget(GTK_EVENT_CONTROLLER(target));
    gtk_widget_add_css_class(widget, "box_dnd_container-hover");
    return GDK_ACTION_COPY;
}

static GdkDragAction on_drag_leave(GtkDropTarget* target, double x, double y, gpointer data){
    (void) data;
    (void) x;
    (void) y;
    GtkWidget *widget = gtk_event_controller_get_widget(GTK_EVENT_CONTROLLER(target));
    gtk_widget_remove_css_class(widget, "box_dnd_container-hover");
    return GDK_ACTION_COPY;
}

typedef struct{
    GHashTable* collection;
    GtkWidget* display_widget;
    GtkWindow* window;
}DROP_FILES_PARAMETERS;

static gboolean drop_files_to_compress(
    GtkDropTarget* target,
    const GValue* value,
    double x,
    double y,
    gpointer user_data
){
    //ignore not used parameters
    (void) x;
    (void) y;
    (void) target;
    DROP_FILES_PARAMETERS* data = (DROP_FILES_PARAMETERS*) user_data;
    GHashTable* collection = data->collection;
    GtkWidget* file_explorer = data->display_widget;
    GtkWindow* window = data->window;

    //cleans the files everytime a new one is dropped
    g_hash_table_remove_all(collection);

    GSList* files = g_value_get_boxed(value);
    guint length = g_slist_length(files);

    if(length > 1){
        show_warning_dialog(window, "Make sure you are uploading a single directory.");
        return FALSE;
    }

    GFile* file = G_FILE(files->data);

    char* path = g_file_get_path(file);
    if(!is_directory(path)){
        g_free(path);
        show_warning_dialog(window, "Make sure you are uploading a directory.");
        return FALSE;
    }

    g_free(path);
        
    add_file_to_collection(file, collection);
    build_file_hierarchy_widget(collection, GTK_WIDGET(file_explorer));

    return TRUE;
}


typedef struct{
    GtkWindow* window;
    GtkWidget* display_widget;
    GFile** result_file;
    GtkLabel* lbl_selected_file_name;
    GtkEntry* entry_output_path;
}DROP_DECOMP_FILE_PARAMETERS;

static gboolean drop_files_to_decompress(
    GtkDropTarget* target,
    const GValue* value,
    double x,
    double y,
    gpointer user_data
){
    //ignore not used parameters
    (void) x;
    (void) y;
    (void) target;
    DROP_DECOMP_FILE_PARAMETERS* data = (DROP_DECOMP_FILE_PARAMETERS*) user_data;
    GtkWindow* window = data->window;
    GtkWidget* bottom_section = data->display_widget;
    GtkLabel* lbl_file_name = data->lbl_selected_file_name;
    GtkEntry* output_path = data->entry_output_path;

    GSList* files = g_value_get_boxed(value);
    guint length = g_slist_length(files);

    if(length > 1){
        show_warning_dialog(window, "You can only decompress one file at a time.");
        return FALSE;
    }

    GFile* file = G_FILE(files->data);

    char* basename = g_file_get_basename(file);
    if(!has_extension(basename, ".jix")){
        g_free(basename);
        show_warning_dialog(window, "Only .jix files are allowed");
        return FALSE;
    }
    
    data->result_file = &file; //make pointer point to the file selected
    
    show_export_section(bottom_section, lbl_file_name, basename, output_path);    
    g_free(basename);

    return TRUE;
}

void set_drop_to_compress_in_box(GtkWidget* box, GHashTable* collection, GtkWidget* file_explorer, GtkWindow* window){
    GtkDropTarget* target = gtk_drop_target_new(GDK_TYPE_FILE_LIST, GDK_ACTION_COPY);

    DROP_FILES_PARAMETERS* drop_parameters = g_new0(DROP_FILES_PARAMETERS, 1);
    drop_parameters->collection = collection;
    drop_parameters->display_widget = file_explorer;
    drop_parameters->window = window;

    g_signal_connect_data(target, "drop", G_CALLBACK(drop_files_to_compress), drop_parameters, free_callback_data, 0);
    gtk_widget_add_controller(box, GTK_EVENT_CONTROLLER(target));
    g_signal_connect(target, "enter", G_CALLBACK(on_drag_enter), NULL);
    g_signal_connect(target, "leave", G_CALLBACK(on_drag_leave), NULL);
}

void set_drop_to_decompress_in_box(
    GtkWidget* box, 
    GtkWidget* bottom_section, 
    GtkWindow* window,
    GtkLabel* lbl_selected_file_name,
    GFile** selected_file,
    GtkEntry* entry_output_path
){
    GtkDropTarget* target = gtk_drop_target_new(GDK_TYPE_FILE_LIST, GDK_ACTION_COPY);

    DROP_DECOMP_FILE_PARAMETERS* drop_parameters = g_new0(DROP_DECOMP_FILE_PARAMETERS, 1);
    drop_parameters->window = window;
    drop_parameters->display_widget = bottom_section;
    drop_parameters->lbl_selected_file_name = lbl_selected_file_name;
    drop_parameters->result_file = selected_file;
    drop_parameters->entry_output_path = entry_output_path;

    g_signal_connect_data(target, "drop", G_CALLBACK(drop_files_to_decompress), drop_parameters, free_callback_data, 0);
    gtk_widget_add_controller(box, GTK_EVENT_CONTROLLER(target));
    g_signal_connect(target, "enter", G_CALLBACK(on_drag_enter), NULL);
    g_signal_connect(target, "leave", G_CALLBACK(on_drag_leave), NULL);
}
