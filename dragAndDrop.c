#include <stdlib.h>
#include <gtk/gtk.h>
#include <math.h>
#include <unistd.h>
#include <time.h>
#include <stdbool.h>

static void add_file_to_collection(GFile* file, GHashTable* collection){
        char* basename = g_file_get_basename(file);
        g_hash_table_add(collection, basename);
        g_print("dropped: %s\n", basename);
}

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

static gboolean drop_files(
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
    GHashTable* collection = user_data;

    if(G_VALUE_HOLDS(value, GDK_TYPE_FILE_LIST)){ //more than one file
        GSList *files = g_value_get_boxed(value);
        for(GSList* l = files; l != NULL; l = l->next){
            GFile* file = G_FILE(l->data);
            add_file_to_collection(file, collection);
        }
        return TRUE;
    }else if(G_VALUE_HOLDS(value, G_TYPE_FILE)){ //one file
        GFile* file = g_value_get_object(value);
        add_file_to_collection(file, collection);
        return TRUE;
    }
    return FALSE;
}

void set_drop_in_box(GtkWidget* box, GHashTable* collection){
    GtkDropTarget* target = gtk_drop_target_new(GDK_TYPE_FILE_LIST, GDK_ACTION_COPY);
    g_signal_connect(target, "drop", G_CALLBACK(drop_files), collection);
    gtk_widget_add_controller(box, GTK_EVENT_CONTROLLER(target));
    g_signal_connect(target, "enter", G_CALLBACK(on_drag_enter), NULL);
    g_signal_connect(target, "leave", G_CALLBACK(on_drag_leave), NULL);
}
