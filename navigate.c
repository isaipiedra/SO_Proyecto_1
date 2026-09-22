#include <string.h>
#include <stdio.h>
#include "navigate.h"
#include "fileExplorer.h"
#include "utils.h"
#include "types.h"

int current_page = 1;

// ------------- generic page navigation -------------

void navigate(GtkWidget* button, gpointer user_data){
    (void) button;
    NAV_PARAMETERS* parameters = (NAV_PARAMETERS*) user_data;

    int child = parameters->page;
    current_page = child;

    GtkWidget* layout_holder = parameters->layout_holder;
    GtkWidget* btn_back = parameters->btn_back;
    GtkWidget* layout = gtk_widget_get_first_child(layout_holder);
    int i = 1;
    if(child == 1){
        gtk_widget_set_visible(btn_back, FALSE);
    }else{
        gtk_widget_set_visible(btn_back, TRUE);
    }
    while ((layout = gtk_widget_get_next_sibling(layout))){
        if(i == child){
            gtk_widget_set_visible(layout, TRUE);
        }else{
            gtk_widget_set_visible(layout, FALSE);
        }
        i++;
    }
}

typedef struct {
    NAV_PARAMETERS nav;
    GtkEntry* entry_output_name;         // base name used for compression
    GtkWidget* bottom_section_decomp;    // section that shows the selected file info
    GtkLabel* lbl_selected_decomp_file;
    GtkEntry* entry_output_name_decomp;
} COMPRESS_TO_DECOMP_PARAMETERS;

static void on_compress_clicked(GtkButton* button, gpointer user_data){
    COMPRESS_TO_DECOMP_PARAMETERS* parameters = (COMPRESS_TO_DECOMP_PARAMETERS*) user_data;

    GtkEntryBuffer* buffer = gtk_entry_get_buffer(parameters->entry_output_name);
    const char* base_name = gtk_entry_buffer_get_text(buffer);

    if(base_name == NULL || strlen(base_name) == 0){
        base_name = OUTPUT_FOLDER_DEFAULT_NAME;
    }

    char compressed_file_name[MAX_FILENAME];
    if(has_extension(base_name, ".jix")){
        snprintf(compressed_file_name, sizeof(compressed_file_name), "%s", base_name);
    }else{
        snprintf(compressed_file_name, sizeof(compressed_file_name), "%s.jix", base_name);
    }

    show_export_section(
        parameters->bottom_section_decomp,
        parameters->lbl_selected_decomp_file,
        compressed_file_name,
        parameters->entry_output_name_decomp
    );

    navigate(GTK_WIDGET(button), &parameters->nav);
}

static void on_back_clicked(GtkButton* button, gpointer user_data){
    GtkWidget* layout_holder = (GtkWidget*) user_data;  
    NAV_PARAMETERS params = {
        layout_holder,
        GTK_WIDGET(button),
        current_page-1
    };
    navigate(GTK_WIDGET(button), &params);
}

void set_up_navigation(GtkBuilder* builder){

    GObject* layout_holder = gtk_builder_get_object(builder, "layout_holder");

    // ---- compress to decompress screen ----

    GObject* btn_back = gtk_builder_get_object(builder, "btn_back");
    g_signal_connect(btn_back, "clicked", G_CALLBACK(on_back_clicked), layout_holder);

    // ---- compress to decompress screen ----

    GObject* btn_compile = gtk_builder_get_object(builder, "btn_compile");
    GObject* entry_output_name = gtk_builder_get_object(builder, "entry_output_name");
    GObject* bottom_section_decomp = gtk_builder_get_object(builder, "bottom_section_decomp");
    GObject* lbl_selected_decomp_file = gtk_builder_get_object(builder, "lbl_selected_decomp_file");
    GObject* entry_output_name_decomp = gtk_builder_get_object(builder, "entry_output_name_decomp");

    COMPRESS_TO_DECOMP_PARAMETERS* compress_nav_parameters = g_new0(COMPRESS_TO_DECOMP_PARAMETERS, 1);
    compress_nav_parameters->nav.layout_holder = GTK_WIDGET(layout_holder);
    compress_nav_parameters->nav.btn_back = GTK_WIDGET(btn_back);
    compress_nav_parameters->nav.page = PAGE_DECOMPRESS;
    compress_nav_parameters->entry_output_name = GTK_ENTRY(entry_output_name);
    compress_nav_parameters->bottom_section_decomp = GTK_WIDGET(bottom_section_decomp);
    compress_nav_parameters->lbl_selected_decomp_file = GTK_LABEL(lbl_selected_decomp_file);
    compress_nav_parameters->entry_output_name_decomp = GTK_ENTRY(entry_output_name_decomp);

    g_signal_connect_data(
        btn_compile, "clicked",
        G_CALLBACK(on_compress_clicked),
        compress_nav_parameters,
        free_callback_data, 0
    );

    // ---- compress to decompress screen ----

    GObject* btn_decompress = gtk_builder_get_object(builder, "btn_decompress");

    NAV_PARAMETERS* stats_nav_parameters = g_new0(NAV_PARAMETERS, 1);
    stats_nav_parameters->layout_holder = GTK_WIDGET(layout_holder);
    stats_nav_parameters->btn_back = GTK_WIDGET(btn_back);
    stats_nav_parameters->page = PAGE_STATS;

    g_signal_connect_data(
        btn_decompress, "clicked",
        G_CALLBACK(navigate),
        stats_nav_parameters,
        free_callback_data, 0
    );
}
