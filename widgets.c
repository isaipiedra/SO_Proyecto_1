#include "widgets.h"

// helpers

FileNode *file_node_new_file(const char *name) {
    FileNode *node = g_new0(FileNode, 1);
    node->type = NODE_FILE;
    node->name = g_strdup(name);
    node->children = NULL;
    return node;
}

FileNode *file_node_new_folder(const char *name, GList *children) {
    FileNode *node = g_new0(FileNode, 1);
    node->type = NODE_FOLDER;
    node->name = g_strdup(name);
    node->children = children;
    return node;
}

void file_node_free(FileNode *node) {
    if (!node) return;
    for (GList *l = node->children; l; l = l->next) {
        file_node_free((FileNode *)l->data);
    }
    g_list_free(node->children);
    g_free(node->name);
    g_free(node);
}

// builders

GtkWidget *create_file_widget(const char *name) {
    GtkWidget *row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_add_css_class(row, "file-row");

    GtkWidget *spacer = gtk_fixed_new();
    gtk_widget_set_size_request(spacer, 15, -1);
    gtk_box_append(GTK_BOX(row), spacer);

    GtkWidget *icon = gtk_picture_new_for_filename("images/file.png");
    gtk_widget_add_css_class(icon, "file-icon");
    gtk_box_append(GTK_BOX(row), icon);

    GtkWidget *label = gtk_label_new(name);
    gtk_box_append(GTK_BOX(row), label);

    return row;
}

typedef struct {
    GtkWidget* list;
    GtkWidget* arrow;
} ON_FOLDER_CLICKED_PARAMETERS;

//hides the children
static void on_folder_clicked(GtkButton *button, gpointer user_data) {
    (void)button;
    ON_FOLDER_CLICKED_PARAMETERS* data = (ON_FOLDER_CLICKED_PARAMETERS*)user_data;
    GtkWidget* list = data->list;
    GtkWidget* arrow = data->arrow;
    gboolean visible = !gtk_widget_get_visible(list);
    gtk_widget_set_visible(list, visible);
    if(visible){
        gtk_picture_set_filename(GTK_PICTURE(arrow), "images/down.png");
    }else{
        gtk_picture_set_filename(GTK_PICTURE(arrow), "images/up.png");
    }
}

//wrapper fuction that matches the singnature
static void free_callback_data(gpointer data, GClosure *closure) {
    (void) closure;
    g_free(data);
}

GtkWidget *create_folder_widget(const char *name, GList *children, int depth) {
    
    // header
    GtkWidget* button = gtk_button_new();
    gtk_widget_add_css_class(button, "folder-button");

    GtkWidget* header = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_add_css_class(header, "folder-header");

    GtkWidget* arrow = gtk_picture_new_for_filename("images/down.png");
    gtk_widget_add_css_class(arrow, "folder-arrow");
    gtk_widget_set_size_request(arrow, 10, 10);
    gtk_widget_set_halign(arrow, GTK_ALIGN_CENTER);
    gtk_widget_set_valign(arrow, GTK_ALIGN_CENTER);
    gtk_box_append(GTK_BOX(header), arrow);

    GtkWidget* icon = gtk_picture_new_for_filename("images/folder.png");
    gtk_widget_add_css_class(icon, "folder-icon");
    gtk_box_append(GTK_BOX(header), icon);

    GtkWidget* label = gtk_label_new(name);
    gtk_box_append(GTK_BOX(header), label);

    gtk_button_set_child(GTK_BUTTON(button), header);

    GtkWidget* outer = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_box_append(GTK_BOX(outer), button);

    //children
    GtkWidget *indent_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);

    GtkWidget *indent = gtk_fixed_new();
    gtk_widget_set_size_request(indent, 20, -1);
    gtk_box_append(GTK_BOX(indent_row), indent);

    GtkWidget *list_files = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_name(list_files, "list_files");

    for (GList *l = children; l; l = l->next) {
        FileNode *child = (FileNode *)l->data;
        gtk_box_append(GTK_BOX(list_files), build_node_widget(child, depth + 1));
    }

    gtk_box_append(GTK_BOX(indent_row), list_files);
    gtk_box_append(GTK_BOX(outer), indent_row);

    ON_FOLDER_CLICKED_PARAMETERS* parameters = g_new0(ON_FOLDER_CLICKED_PARAMETERS, 1);
    parameters->arrow = arrow;
    parameters->list = list_files;
    g_signal_connect_data(button, "clicked", G_CALLBACK(on_folder_clicked), parameters, free_callback_data, 0);

    return outer;
}

GtkWidget *build_node_widget(FileNode *node, int depth) {
    switch (node->type) {
        case NODE_FILE:
            return create_file_widget(node->name);
        case NODE_FOLDER:
            return create_folder_widget(node->name, node->children, depth);
        default:
            g_warn_if_reached();
            return gtk_label_new("?");
    }
}