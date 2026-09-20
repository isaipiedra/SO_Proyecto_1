#include "fileExplorer.h"
#include "utils.h"



GtkLabel* lbl_output_folder_name;
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

FileNode *file_node_new_root(GList *children) {
    FileNode *node = g_new0(FileNode, 1);
    node->type = NODE_ROOT;
    node->name = OUTPUT_FOLDER_DEFAULT_NAME;
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

GtkWidget* create_root_widget(const char *name, GList *children, int depth){
    GtkWidget* result = create_folder_widget(name, children, depth);
    GtkWidget* holder = gtk_widget_get_first_child(result); // button
    holder = gtk_widget_get_first_child(holder); // header
    holder = gtk_widget_get_first_child(holder); // arrow
    holder = gtk_widget_get_next_sibling(holder); // icon
    holder = gtk_widget_get_next_sibling(holder); // label

    lbl_output_folder_name = GTK_LABEL(holder);

    return result;
}

GtkWidget *build_node_widget(FileNode *node, int depth) {
    switch (node->type) {
        case NODE_FILE:
            return create_file_widget(node->name);
        case NODE_FOLDER:
            return create_folder_widget(node->name, node->children, depth);
        case NODE_ROOT:
            return create_root_widget(node->name, node->children, depth);
        default:
            g_warn_if_reached();
            return gtk_label_new("?");
    }
}

FileNode *build_node_from_path(const char *path, const char *name) {
    if (g_file_test(path, G_FILE_TEST_IS_DIR)) {
        GList *children = NULL;
        GError *error = NULL;
        GDir *dir = g_dir_open(path, 0, &error);
 
        if (!dir) {
            g_warning("No se pudo abrir el directorio '%s': %s",
                      path, error ? error->message : "razón desconocida");
            g_clear_error(&error);
            return file_node_new_folder(name, NULL);
        }
 
        const char *entry_name;
        while ((entry_name = g_dir_read_name(dir)) != NULL) {
            char *child_path = g_build_filename(path, entry_name, NULL);
            children = g_list_append(children, build_node_from_path(child_path, entry_name));
            g_free(child_path);
        }
        g_dir_close(dir);
 
        return file_node_new_folder(name, children);
    }
 
    return file_node_new_file(name);
}

void build_file_hierarchy_widget(GHashTable* collection, GtkWidget* file_explorer){
    FileNode* root = build_tree_from_hashtable(collection);
    GtkWidget* tree_widget = build_node_widget(root, 0);

    GtkWidget* box_file_explorer = gtk_widget_get_first_child(file_explorer); //scroll
    box_file_explorer = gtk_widget_get_first_child(box_file_explorer); //viewport
    box_file_explorer = gtk_widget_get_first_child(box_file_explorer); //box

    clear_box(GTK_BOX(box_file_explorer));
    gtk_box_append(GTK_BOX(box_file_explorer), tree_widget);
    
    gtk_widget_set_visible(file_explorer, TRUE);
}
 
FileNode *build_tree_from_hashtable(GHashTable *files) {
    GList *children = NULL;
    GHashTableIter iter;
    gpointer key, value;
 
    g_hash_table_iter_init(&iter, files);
    while (g_hash_table_iter_next(&iter, &key, &value)) {
        (void)value;
        const char *path = (const char *)key;
 
        char *name = g_path_get_basename(path);
        children = g_list_append(children, build_node_from_path(path, name));
        g_free(name);
    }
 
    return file_node_new_root(children);
}

static void on_file_open_ready(GObject* source, GAsyncResult* result, gpointer user_data) {
    GtkFileDialog* dialog = GTK_FILE_DIALOG(source);
    GError* error = NULL;
    GFile* file = gtk_file_dialog_select_folder_finish(dialog, result, &error);
    BROWSE_FOR_DIR_PARAMETERS* params = user_data;
    GHashTable* collection = params->collection;
    GtkWidget* file_explorer = params->file_explorer;

    //cleans the files everytime a new one is dropped
    if(collection) g_hash_table_remove_all(collection);

    if(!file){
        return;
    }

    add_file_to_collection(file, collection);

    if (error) {
        g_error_free(error);
        return;
    }

    build_file_hierarchy_widget(collection, file_explorer);
}

static void on_output_dir_selected(GObject* source, GAsyncResult* result, gpointer user_data) {
    GtkFileDialog* dialog = GTK_FILE_DIALOG(source);
    GError* error = NULL;
    GFile* file = gtk_file_dialog_save_finish(dialog, result, &error);
    if (error) {
        g_error_free(error);
        return;
    }
    if(!file){
        return;
    }

    BROWSE_FOR_OUTPUT_DIR_PARAMETERS* params = user_data;

    GtkEntry* output_entry = params->output_entry;
    GtkEntryBuffer* buffer = gtk_entry_get_buffer(output_entry);
    char* folder_name = g_file_get_path(file);
    gtk_entry_buffer_set_text(buffer, folder_name, strlen(folder_name));
}

void browse_for_dir(GtkButton* button, gpointer user_data) {

    (void) button;

    BROWSE_FOR_DIR_PARAMETERS* parameters = (BROWSE_FOR_DIR_PARAMETERS*) user_data;
    gtk_file_dialog_select_folder(parameters->dialog,
                         parameters->window,
                         NULL,                
                         on_file_open_ready,
                         parameters);         
}

void browse_for_output_dir(GtkButton* button, gpointer user_data){
    (void) button;

    BROWSE_FOR_OUTPUT_DIR_PARAMETERS* parameters = (BROWSE_FOR_OUTPUT_DIR_PARAMETERS*) user_data;
    gtk_file_dialog_save(parameters->dialog,
                         parameters->window,
                         NULL,                
                         on_output_dir_selected,
                         parameters);   
}

