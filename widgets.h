#ifndef WIDGETS_H
#define WIDGETS_H

#include <gtk/gtk.h>

typedef enum {
    NODE_FILE,
    NODE_FOLDER
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

#endif