#include "dragAndDrop.h"
#include "fileExplorer.h"
#include "outputNameEntry.h"
#include "utils.h"
#include "huffmanAlgorithms.h"

GtkBuilder *builder;

static GHashTable* file_collection = NULL;
GObject* window = NULL;
static GtkFileDialog* file_dialog = NULL;

extern GtkLabel* lbl_output_folder_name;

static void set_up_widgets(GtkBuilder* builder){

    // ------------- file explorer -------------

    //set file explorer invisible 
    GObject* box_file_explorer_container = gtk_builder_get_object(builder, "box_file_explorer_container");
    gtk_widget_set_visible(GTK_WIDGET(box_file_explorer_container), FALSE);

    // ------------- drag and drop area -------------

    file_collection = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);
    GtkWidget *dnd_box = GTK_WIDGET(gtk_builder_get_object(builder, "box_dnd_container"));
    set_drop_in_box(dnd_box, file_collection, GTK_WIDGET(box_file_explorer_container));

    // ------------- browse button -------------
    
    file_dialog = gtk_file_dialog_new();
    char* result_file = NULL;
    GObject* btn_browse = gtk_builder_get_object(builder, "btn_open_file_explorer");
    OPEN_FILE_DIALOG_PARAMETERS* open_file_dialog_parameters = g_new0(OPEN_FILE_DIALOG_PARAMETERS, 1); 
    
    open_file_dialog_parameters->dialog = file_dialog;
    open_file_dialog_parameters->window = GTK_WINDOW(window);
    open_file_dialog_parameters->selected_file = result_file;
    open_file_dialog_parameters->collection = file_collection;
    open_file_dialog_parameters->file_explorer = GTK_WIDGET(box_file_explorer_container);

    g_signal_connect_data(
        btn_browse, "clicked", 
        G_CALLBACK(open_file_dialog), 
        open_file_dialog_parameters, 
        free_callback_data, 0
    );

    // ------------- entry for output folder name -------------

    GObject* entry_output_name = gtk_builder_get_object(builder, "entry_output_name");
    g_signal_connect(entry_output_name, "changed", G_CALLBACK(on_update_output_entry), &lbl_output_folder_name);

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

    window = gtk_builder_get_object(builder, "window");
    if (!window) {
        g_error("Failed to get window from builder");
        return;
    }
    gtk_window_maximize(GTK_WINDOW(window));

    set_up_widgets(builder);
    
    gtk_window_set_application(GTK_WINDOW(window), app);
    gtk_widget_set_visible(GTK_WIDGET(window), TRUE);

    g_object_unref(builder);

}

// DEBUG MAIN FUNCTION

int main_debug(int argc, char *argv[]) {
    if (argc < 4) {
        fprintf(stderr, "Usage: %s -C <directory> <output.jix>\n", argv[0]);
        fprintf(stderr, "       %s -D <archive.jix> <output_directory>\n\n", argv[0]);
        return 1;
    }

    char mode = argv[1][1];

    if (mode == 'C') {
        if (argc != 4) {
            fprintf(stderr, "Compression mode: -C <directory> <output.jix>\n");
            return 1;
        }
        printf("=== HUFFMAN COMPRESSION ===\n\n");
        int result = huffman_compression(argv[2], argv[3]);
        return result ? 0 : 1;

    } else if (mode == 'D') {
        if (argc != 4) {
            fprintf(stderr, "Decompression mode: -D <archive.jix> <output_directory>\n");
            return 1;
        }
        printf("=== HUFFMAN DECOMPRESSION ===\n\n");
        int result = huffman_decompression(argv[2], argv[3]);
        return result ? 0 : 1;

    } else {
        fprintf(stderr, "Invalid mode. Use -C for compression or -D for decompression\n");
        return 1;
    }
}

int main(int argc, char *argv[]) {
    return main_debug(argc, argv);

    /*GtkApplication *app = gtk_application_new("org.gtk.example", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);

    int status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);

    return status;*/
}