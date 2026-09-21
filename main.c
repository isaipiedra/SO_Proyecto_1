#define _POSIX_C_SOURCE 200809L

#include "dragAndDrop.h"
#include "fileExplorer.h"
#include "outputNameEntry.h"
#include "utils.h"
#include "types.h"
#include "huffmanSerial.h"
#include "huffmanParallel.h"

GtkBuilder *builder;

static GHashTable* file_collection = NULL;
GObject* window = NULL;
static GtkFileDialog* file_dialog = NULL;

extern GtkLabel* lbl_output_folder_name;

static void set_up_widgets(GtkBuilder* builder){

    GdkCursor* pointer_cursor = gdk_cursor_new_from_name("pointer", NULL);

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

// MEDICIÓN DE TIEMPO

static double now_seconds(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec / 1e9;
}

// IMPRESIÓN DE ESTADÍSTICAS

static void print_compression_stats(const CompressionStats *s, const char *label,
                                    const char *archive, double elapsed) {
    double health = s->files_total > 0
        ? 100.0 * s->files_verified / s->files_total
        : 0.0;
    double ratio = s->total_original_bytes > 0
        ? (double)s->total_compressed_bytes / s->total_original_bytes
        : 0.0;
    double saved = 100.0 * (1.0 - ratio);

    printf("--- %s ---\n", label);
    printf("  Archive:                     %s\n", archive);
    printf("  Elapsed time:                %.6f s\n", elapsed);
    printf("  Files processed:             %d\n", s->files_total);
    printf("  Files verified (round-trip): %d / %d (%.2f%%)\n",
           s->files_verified, s->files_total, health);
    printf("  Total original size:         %lu bytes\n", s->total_original_bytes);
    printf("  Total compressed size:       %lu bytes\n", s->total_compressed_bytes);
    printf("  Compression ratio:           %.4f\n", ratio);
    printf("  Space saved:                 %.2f%%\n", saved);
    printf("\n");
}

static void print_decompression_stats(const DecompressionStats *s, const char *label,
                                      const char *archive, double elapsed) {
    double success = s->files_total > 0
        ? 100.0 * s->files_successful / s->files_total
        : 0.0;

    printf("--- %s ---\n", label);
    printf("  Archive:                   %s\n", archive);
    printf("  Elapsed time:              %.6f s\n", elapsed);
    printf("  Files in archive:          %d\n", s->files_total);
    printf("  Successfully extracted:    %d (%.2f%%)\n", s->files_successful, success);
    printf("  Failed:                    %d\n", s->files_failed);
    printf("  Total original size:       %lu bytes\n", s->total_original_bytes);
    printf("  Total decompressed size:   %lu bytes\n", s->total_decompressed_bytes);
    printf("\n");
}

// HELPERS DE NOMBRES

// Construye "<base>_<suffix>.jix" a partir de un nombre base (sin extensión).
static void build_archive_name(char *out, size_t out_size,
                               const char *base, const char *suffix) {
    snprintf(out, out_size, "%s_%s.jix", base, suffix);
}

// Construye "<base>_<suffix>" (sin extensión) para directorios de salida.
static void build_dir_name(char *out, size_t out_size,
                           const char *base, const char *suffix) {
    snprintf(out, out_size, "%s_%s", base, suffix);
}

// MAIN DE COMPARACIÓN

int main_debug(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <directory> <archive_base_name>\n", argv[0]);
        fprintf(stderr, "Example: %s /home/user/Test salida\n", argv[0]);
        fprintf(stderr, "  Produces: salida_serial.jix, salida_paralela.jix\n");
        fprintf(stderr, "  Also runs decompression to: <base>_serial_out/, <base>_paralela_out/\n");
        return 1;
    }

    const char *input_dir = argv[1];
    const char *base = argv[2];

    char serial_archive[MAX_PATH];
    char parallel_archive[MAX_PATH];
    char serial_outdir[MAX_PATH];
    char parallel_outdir[MAX_PATH];

    build_archive_name(serial_archive, sizeof(serial_archive), base, "serial");
    build_archive_name(parallel_archive, sizeof(parallel_archive), base, "paralela");
    build_dir_name(serial_outdir, sizeof(serial_outdir), base, "serial_out");
    build_dir_name(parallel_outdir, sizeof(parallel_outdir), base, "paralela_out");

    printf("=====================================================\n");
    printf("  HUFFMAN BENCHMARK: SERIAL vs PARALLEL\n");
    printf("=====================================================\n");
    printf("Input directory:   %s\n", input_dir);
    printf("Serial archive:    %s\n", serial_archive);
    printf("Parallel archive:  %s\n", parallel_archive);
    printf("=====================================================\n\n");

    // ---------- COMPRESIÓN SERIAL ----------

    printf(">>> Running SERIAL compression...\n\n");
    double t0 = now_seconds();
    CompressionStats cs = huffman_compression_serial(input_dir, serial_archive);
    double t1 = now_seconds();
    double serial_comp_time = t1 - t0;
    print_compression_stats(&cs, "COMPRESSION (SERIAL)", serial_archive, serial_comp_time);

    // ---------- COMPRESIÓN PARALELA ----------

    printf(">>> Running PARALLEL compression...\n\n");
    t0 = now_seconds();
    CompressionStats cp = huffman_compression_parallel(input_dir, parallel_archive);
    t1 = now_seconds();
    double parallel_comp_time = t1 - t0;
    print_compression_stats(&cp, "COMPRESSION (PARALLEL)", parallel_archive, parallel_comp_time);

    // ---------- DESCOMPRESIÓN SERIAL ----------

    printf(">>> Running SERIAL decompression...\n\n");
    t0 = now_seconds();
    DecompressionStats ds = huffman_decompression_serial(serial_archive, serial_outdir);
    t1 = now_seconds();
    double serial_decomp_time = t1 - t0;
    print_decompression_stats(&ds, "DECOMPRESSION (SERIAL)", serial_archive, serial_decomp_time);

    // ---------- DESCOMPRESIÓN PARALELA ----------

    printf(">>> Running PARALLEL decompression...\n\n");
    t0 = now_seconds();
    DecompressionStats dp = huffman_decompression_parallel(parallel_archive, parallel_outdir);
    t1 = now_seconds();
    double parallel_decomp_time = t1 - t0;
    print_decompression_stats(&dp, "DECOMPRESSION (PARALLEL)", parallel_archive, parallel_decomp_time);

    // ---------- RESUMEN ----------

    printf("=====================================================\n");
    printf("  SUMMARY\n");
    printf("=====================================================\n");
    printf("Compression times:\n");
    printf("  Serial:   %.6f s\n", serial_comp_time);
    printf("  Parallel: %.6f s\n", parallel_comp_time);
    if (parallel_comp_time > 0.0) {
        printf("  Speedup:  %.2fx\n", serial_comp_time / parallel_comp_time);
    }
    printf("\n");

    printf("Decompression times:\n");
    printf("  Serial:   %.6f s\n", serial_decomp_time);
    printf("  Parallel: %.6f s\n", parallel_decomp_time);
    if (parallel_decomp_time > 0.0) {
        printf("  Speedup:  %.2fx\n", serial_decomp_time / parallel_decomp_time);
    }
    printf("\n");

    printf("Health check:\n");
    printf("  Serial compression:   %d / %d verified\n", cs.files_verified, cs.files_total);
    printf("  Parallel compression: %d / %d verified\n", cp.files_verified, cp.files_total);
    printf("  Serial decompression:   %d ok, %d failed\n", ds.files_successful, ds.files_failed);
    printf("  Parallel decompression: %d ok, %d failed\n", dp.files_successful, dp.files_failed);
    printf("\n");

    printf("To verify archives are byte-identical, run:\n");
    printf("  cmp %s %s && echo IDENTICAL\n", serial_archive, parallel_archive);
    printf("\nTo verify restored files match the originals, run:\n");
    printf("  diff -r %s %s && echo MATCH\n", input_dir, serial_outdir);
    printf("  diff -r %s %s && echo MATCH\n", input_dir, parallel_outdir);
    printf("=====================================================\n");

    // Éxito global: ambos archivos verificados y ambas descompresiones exitosas
    int ok = (cs.files_verified == cs.files_total && cs.files_total > 0) &&
             (cp.files_verified == cp.files_total && cp.files_total > 0) &&
             (ds.files_failed == 0 && ds.files_total > 0) &&
             (dp.files_failed == 0 && dp.files_total > 0);
    return ok ? 0 : 1;
}


int main(int argc, char *argv[]) {

    main_debug(argc, argv);

    /*GtkApplication *app = gtk_application_new("org.gtk.example", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);

    int status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);

    return status;*/
}