#ifndef NAVIGATE_H
#define NAVIGATE_H

#include <gtk/gtk.h>

// page numbers, matching the order of the ScrolledWindow children
// inside "layout_holder" (right after box_header)
#define PAGE_COMPRESS   1
#define PAGE_DECOMPRESS 2
#define PAGE_STATS      3

typedef struct {
    GtkWidget* layout_holder;
    GtkWidget* btn_back;
    int page;
} NAV_PARAMETERS;

void navigate(GtkWidget* button, gpointer user_data);

void set_up_navigation(GtkBuilder* builder, GHashTable* file_collection);

#endif