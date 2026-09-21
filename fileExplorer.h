#ifndef WIDGETS_H
#define WIDGETS_H

#include <gtk/gtk.h>

typedef enum {
    NODE_FILE,
    NODE_FOLDER,
    NODE_ROOT
} NodeType;

/* a node in the file tree. For folder children is a list of
 file nodes (files or nested folders). For file it's always NULL. */
typedef struct FileNode {
    NodeType type;
    char* name;
    GList* children;
} FileNode;

FileNode* file_node_new_file(const char* name);
FileNode* file_node_new_folder(const char* name, GList* children);
void file_node_free(FileNode* node);

// builders
GtkWidget* create_file_widget(const char* name);
GtkWidget* create_folder_widget(const char* name, GList* children, int depth);

// based on node type 
GtkWidget* build_node_widget(FileNode* node, int depth);

//tree handlers
FileNode *build_tree_from_hashtable(GHashTable *files);
FileNode *build_node_from_path(const char *path, const char *name);

//widget constructor
void build_file_hierarchy_widget(GHashTable* collection, GtkWidget* file_explorer);

//file dialog
typedef struct{
    GtkFileDialog* dialog;
    GtkWindow* window;
    GHashTable* collection;
    GtkWidget* file_explorer;
    char* selected_file;
}BROWSE_FOR_DIR_PARAMETERS;

void browse_for_dir(GtkButton* button, gpointer user_data);

typedef struct{
    GtkFileDialog* dialog;
    GtkWindow* window;
    char* selected_file;
    GtkEntry* output_entry;
}BROWSE_FOR_OUTPUT_DIR_PARAMETERS;
void browse_for_output_dir(GtkButton* button, gpointer user_data);

#endif