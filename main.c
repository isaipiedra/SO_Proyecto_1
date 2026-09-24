#define _POSIX_C_SOURCE 200809L

#include "dragAndDrop.h"
#include "fileExplorer.h"
#include "outputNameEntry.h"
#include "utils.h"
#include "types.h"
#include "huffmanSerial.h"
#include "huffmanParallel.h"
#include "huffmanConcurrent.h"
#include "navigate.h"

GtkBuilder *builder;

static GHashTable* file_collection = NULL;
GObject* window = NULL;
static GtkFileDialog* file_dialog = NULL;

extern GtkLabel* lbl_output_folder_name;

GFile* decompress_selected_file = NULL;

int page_index = 1;
static GObject* title = NULL;

static void set_up_widgets(GtkBuilder* builder){

    GdkCursor* pointer_cursor = gdk_cursor_new_from_name("pointer", NULL);

    window = gtk_builder_get_object(builder, "window");

    // ------------- header -------------
    title = gtk_builder_get_object(builder, "lbl_title");


    // ------------- file explorer -------------

    //set file explorer invisible 
    GObject* box_file_explorer_container = gtk_builder_get_object(builder, "box_file_explorer_container");
    gtk_widget_set_visible(GTK_WIDGET(box_file_explorer_container), FALSE);

    // ------------- drag and drop area -------------

    file_collection = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);
    GtkWidget *dnd_box = GTK_WIDGET(gtk_builder_get_object(builder, "box_dnd_container"));
    set_drop_to_compress_in_box(dnd_box, file_collection, GTK_WIDGET(box_file_explorer_container), GTK_WINDOW(window));

    // ------------- browse dir button -------------
    
    file_dialog = gtk_file_dialog_new();
    char* result_file = NULL;
    GObject* btn_browse = gtk_builder_get_object(builder, "btn_open_file_explorer");
    BROWSE_FOR_DIR_PARAMETERS* open_file_dialog_parameters = g_new0(BROWSE_FOR_DIR_PARAMETERS, 1); 
    
    open_file_dialog_parameters->dialog = file_dialog;
    open_file_dialog_parameters->window = GTK_WINDOW(window);
    open_file_dialog_parameters->selected_file = result_file;
    open_file_dialog_parameters->collection = file_collection;
    open_file_dialog_parameters->file_explorer = GTK_WIDGET(box_file_explorer_container);

    g_signal_connect_data(
        btn_browse, "clicked", 
        G_CALLBACK(browse_for_dir), 
        open_file_dialog_parameters, 
        free_callback_data, 0
    );

    gtk_widget_set_cursor(GTK_WIDGET(btn_browse), pointer_cursor);

    // ------------- entry for output folder name -------------

    GObject* entry_output_name = gtk_builder_get_object(builder, "entry_output_name");
    g_signal_connect(entry_output_name, "changed", G_CALLBACK(on_update_output_entry), &lbl_output_folder_name);

    GtkEventController* output_entry_focus_controller = gtk_event_controller_focus_new();
    ON_OUTPUT_ENTRY_FOCUS_LEAVE_PARAMETERS* on_focus_leave_params = g_new0(ON_OUTPUT_ENTRY_FOCUS_LEAVE_PARAMETERS, 1);
    on_focus_leave_params->entry = GTK_ENTRY(entry_output_name);
    on_focus_leave_params->folder_name = &lbl_output_folder_name; 
    on_focus_leave_params->window = GTK_WINDOW(window); 


    g_signal_connect_data(
        output_entry_focus_controller, 
        "leave", 
        G_CALLBACK(on_output_entry_focus_leave), 
        on_focus_leave_params,
        free_callback_data,
        0
    );

    gtk_widget_add_controller(GTK_WIDGET(entry_output_name), output_entry_focus_controller);

    // ------------- compile button -------------

    GObject* btn_compile = gtk_builder_get_object(builder, "btn_compile");
    gtk_widget_set_cursor(GTK_WIDGET(btn_compile), pointer_cursor);

    // ------------- button to select output dir -------------

    GtkFileDialog* output_name_file_dialog = gtk_file_dialog_new();
    char* result_output_dir = NULL;
    GObject* btn_output_browse_dir = gtk_builder_get_object(builder, "btn_output_browse_dir");
    BROWSE_FOR_OUTPUT_DIR_PARAMETERS* browse_for_output_dir_parameters = g_new0(BROWSE_FOR_OUTPUT_DIR_PARAMETERS, 1); 
    
    browse_for_output_dir_parameters->dialog = output_name_file_dialog;
    browse_for_output_dir_parameters->window = GTK_WINDOW(window);
    browse_for_output_dir_parameters->selected_file = result_output_dir;
    browse_for_output_dir_parameters->output_entry = GTK_ENTRY(entry_output_name);

    g_signal_connect_data(
        btn_output_browse_dir, "clicked", 
        G_CALLBACK(browse_for_output_dir), 
        browse_for_output_dir_parameters, 
        free_callback_data, 0
    );

    gtk_widget_set_cursor(GTK_WIDGET(btn_output_browse_dir), pointer_cursor);

    //======================= Decompresser =======================

    set_up_navigation(builder, file_collection);

    // ------------- entry for output folder name -------------

    GObject* entry_output_decomp = gtk_builder_get_object(builder, "entry_output_name_decomp");
    g_signal_connect(entry_output_decomp, "changed", G_CALLBACK(on_update_output_entry), &lbl_output_folder_name);

    GtkEventController* output_entry_decomp_focus_controller = gtk_event_controller_focus_new();
    ON_OUTPUT_ENTRY_DECOMP_FOCUS_LEAVE_PARAMETERS* on_focus_leave_decomp_params = g_new0(ON_OUTPUT_ENTRY_DECOMP_FOCUS_LEAVE_PARAMETERS, 1);
    on_focus_leave_decomp_params->entry = GTK_ENTRY(entry_output_decomp);
    on_focus_leave_decomp_params->window = GTK_WINDOW(window); 

    g_signal_connect_data(
        output_entry_decomp_focus_controller, 
        "leave", 
        G_CALLBACK(on_output_entry_decomp_focus_leave), 
        on_focus_leave_decomp_params,
        free_callback_data,
        0
    );

    gtk_widget_add_controller(GTK_WIDGET(entry_output_decomp), output_entry_decomp_focus_controller);

    // ------------- button to select output dir -------------

    char* result_output_dir_decomp = NULL;
    GObject* btn_output_browse_dir_decomp = gtk_builder_get_object(builder, "btn_output_browse_dir_decomp");
    BROWSE_FOR_OUTPUT_DIR_PARAMETERS* browse_for_output_dir_decomp_parameters = g_new0(BROWSE_FOR_OUTPUT_DIR_PARAMETERS, 1); 
    
    browse_for_output_dir_decomp_parameters->dialog = output_name_file_dialog;
    browse_for_output_dir_decomp_parameters->window = GTK_WINDOW(window);
    browse_for_output_dir_decomp_parameters->selected_file = result_output_dir_decomp;
    browse_for_output_dir_decomp_parameters->output_entry = GTK_ENTRY(entry_output_decomp);

    g_signal_connect_data(
        btn_output_browse_dir_decomp, "clicked", 
        G_CALLBACK(browse_for_output_dir), 
        browse_for_output_dir_decomp_parameters, 
        free_callback_data, 0
    );

    gtk_widget_set_cursor(GTK_WIDGET(btn_output_browse_dir_decomp), pointer_cursor);
}

static void activate(GtkApplication *app) {

    //load the css
    GtkCssProvider *css_provider = gtk_css_provider_new();
    gtk_css_provider_load_from_path(css_provider, "styles.css");
    gtk_style_context_add_provider_for_display(
        gdk_display_get_default(),
        GTK_STYLE_PROVIDER(css_provider),
        GTK_STYLE_PROVIDER_PRIORITY_USER
    );
    g_object_unref(css_provider);

    //create builder from Cambalache
    GtkBuilder *builder = gtk_builder_new();
    gtk_builder_add_from_file(builder, "UI/builder.ui", NULL);

    set_up_widgets(builder);

    if (!window) {
        g_error("Failed to get window from builder");
        return;
    }
    gtk_window_maximize(GTK_WINDOW(window));

    
    gtk_window_set_application(GTK_WINDOW(window), app);
    gtk_widget_set_visible(GTK_WIDGET(window), TRUE);

    g_object_unref(builder);

}

int main(int argc, char *argv[]) {

    GtkApplication *app = gtk_application_new("org.gtk.example", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);

    int status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);

    return status;
}