#define _POSIX_C_SOURCE 200809L

#include <string.h>
#include <stdio.h>
#include <time.h>
#include "navigate.h"
#include "fileExplorer.h"
#include "utils.h"
#include "types.h"
#include "huffmanSerial.h"
#include "huffmanConcurrent.h"
#include "huffmanParallel.h"

int current_page = 1;

#define ALGO_COUNT 3
static const char* ALGO_SUFFIX[ALGO_COUNT] = { "serial", "concurrent", "parallel" };

typedef CompressionStats   (*CompressFn)(const char*, const char*, const char*);
typedef DecompressionStats (*DecompressFn)(const char*, const char*);

static CompressFn COMPRESS_FUNCS[ALGO_COUNT] = {
    huffman_compression_serial,
    huffman_compression_concurrent,
    huffman_compression_parallel
};

static DecompressFn DECOMPRESS_FUNCS[ALGO_COUNT] = {
    huffman_decompression_serial,
    huffman_decompression_concurrent,
    huffman_decompression_parallel
};

static char g_archive_path[ALGO_COUNT][MAX_PATH];
static char g_output_dir[MAX_PATH];
static char g_output_base_name[MAX_FILENAME];
static gboolean g_have_compressed = FALSE;

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

static void on_back_clicked(GtkButton* button, gpointer user_data){
    GtkWidget* layout_holder = (GtkWidget*) user_data;
    NAV_PARAMETERS params = {
        layout_holder,
        GTK_WIDGET(button),
        current_page - 1
    };
    navigate(GTK_WIDGET(button), &params);
}

// ------------- small helpers -------------

static double now_seconds(void){
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec / 1e9;
}

static char* get_first_input_directory(GHashTable* collection){
    if(!collection || g_hash_table_size(collection) == 0) return NULL;

    GHashTableIter iter;
    gpointer key, value;
    g_hash_table_iter_init(&iter, collection);
    if(g_hash_table_iter_next(&iter, &key, &value)){
        return (char*) key; // owned by the hash table, do not free
    }
    return NULL;
}

static char* resolve_absolute_dir(const char* dir){
    if(dir && g_path_is_absolute(dir)){
        return g_strdup(dir);
    }
    char* cwd = g_get_current_dir();
    if(dir && strlen(dir) > 0){
        char* full = g_build_filename(cwd, dir, NULL);
        g_free(cwd);
        return full;
    }
    return cwd; // no dir given: the archives land in the current directory
}

static void set_percent(GtkLabel* label, double value){
    if(!label) return;
    char buf[64];
    snprintf(buf, sizeof(buf), "%.2f%%", value);
    gtk_label_set_text(label, buf);
}

static void set_seconds(GtkLabel* label, double seconds){
    if(!label) return;
    char buf[64];
    snprintf(buf, sizeof(buf), "%.4f s", seconds);
    gtk_label_set_text(label, buf);
}

static void set_bytes(GtkLabel* label, unsigned long bytes){
    if(!label) return;
    char buf[64];
    if(bytes >= 1024UL * 1024UL){
        snprintf(buf, sizeof(buf), "%.2f MB", bytes / (1024.0 * 1024.0));
    }else if(bytes >= 1024UL){
        snprintf(buf, sizeof(buf), "%.2f KB", bytes / 1024.0);
    }else{
        snprintf(buf, sizeof(buf), "%lu B", bytes);
    }
    gtk_label_set_text(label, buf);
}

static void set_ratio(GtkLabel* label, double ratio){
    if(!label) return;
    char buf[64];
    snprintf(buf, sizeof(buf), "%.4f", ratio);
    gtk_label_set_text(label, buf);
}

// ------------- statistics widgets -------------

typedef struct {
    GtkLabel* health[ALGO_COUNT];
    GtkLabel* comp_time[ALGO_COUNT];
    GtkLabel* decomp_time[ALGO_COUNT];
    GtkLabel* comp_speedup[ALGO_COUNT];
    GtkLabel* decomp_speedup[ALGO_COUNT];
    GtkLabel* original_size[ALGO_COUNT];
    GtkLabel* compressed_size[ALGO_COUNT];
    GtkLabel* ratio[ALGO_COUNT];
} STATS_LABELS;

static GtkLabel* get_stat_label(GtkBuilder* builder, const char* prefix, int algo_index){
    char id[128];
    snprintf(id, sizeof(id), "%s_%s", prefix, ALGO_SUFFIX[algo_index]);
    GObject* obj = gtk_builder_get_object(builder, id);
    return obj ? GTK_LABEL(obj) : NULL;
}

static STATS_LABELS* load_stats_labels(GtkBuilder* builder){
    STATS_LABELS* labels = g_new0(STATS_LABELS, 1);
    for(int i = 0; i < ALGO_COUNT; i++){
        labels->health[i]          = get_stat_label(builder, "lbl_stat_health", i);
        labels->comp_time[i]       = get_stat_label(builder, "lbl_stat_comp_time", i);
        labels->decomp_time[i]     = get_stat_label(builder, "lbl_stat_decomp_time", i);
        labels->comp_speedup[i]    = get_stat_label(builder, "lbl_stat_comp_speedup", i);
        labels->decomp_speedup[i]  = get_stat_label(builder, "lbl_stat_decomp_speedup", i);
        labels->original_size[i]   = get_stat_label(builder, "lbl_stat_original_size", i);
        labels->compressed_size[i] = get_stat_label(builder, "lbl_stat_compressed_size", i);
        labels->ratio[i]           = get_stat_label(builder, "lbl_stat_ratio", i);
    }
    return labels;
}

// ------------- compress screen -> decompress screen -------------

typedef struct {
    NAV_PARAMETERS nav;
    GHashTable* file_collection;         // selected input directory lives here
    GtkEntry* entry_output_name;         // base name / output path typed by the user
    GtkWidget* bottom_section_decomp;    // section that shows the selected file info
    GtkLabel* lbl_selected_decomp_file;
    GtkEntry* entry_output_name_decomp;
    GtkWindow* window;
    STATS_LABELS* stats;
} COMPRESS_TO_DECOMP_PARAMETERS;

static void on_compress_clicked(GtkButton* button, gpointer user_data){
    COMPRESS_TO_DECOMP_PARAMETERS* parameters = (COMPRESS_TO_DECOMP_PARAMETERS*) user_data;

    char* input_dir = get_first_input_directory(parameters->file_collection);
    if(!input_dir){
        show_warning_dialog(parameters->window, "Select a folder to compress first.");
        return;
    }

    GtkEntryBuffer* buffer = gtk_entry_get_buffer(parameters->entry_output_name);
    const char* raw_output = gtk_entry_buffer_get_text(buffer);
    if(raw_output == NULL || strlen(raw_output) == 0){
        raw_output = OUTPUT_FOLDER_DEFAULT_NAME;
    }

    // the entry can hold either a bare name ("Output") or a full path picked
    // through the "browse output dir" dialog - split it into dir + base name
    char* dirname_owned = g_path_get_dirname(raw_output);
    char* basename_owned = g_path_get_basename(raw_output);
    gboolean has_dir = (strcmp(dirname_owned, ".") != 0);
    const char* output_dir = has_dir ? dirname_owned : NULL;

    char filename[ALGO_COUNT][MAX_FILENAME];
    for(int i = 0; i < ALGO_COUNT; i++){
        snprintf(filename[i], MAX_FILENAME, "%s_%s.jix", basename_owned, ALGO_SUFFIX[i]);
    }

    // run the three compressors, one after another, timing each
    double times[ALGO_COUNT];
    CompressionStats stats[ALGO_COUNT];
    for(int i = 0; i < ALGO_COUNT; i++){
        double t0 = now_seconds();
        stats[i] = COMPRESS_FUNCS[i](input_dir, output_dir, filename[i]);
        times[i] = now_seconds() - t0;
    }

    if(stats[0].files_total == 0){
        show_warning_dialog(parameters->window, "No .txt files were found in the selected folder.");
        g_free(dirname_owned);
        g_free(basename_owned);
        return;
    }

    char* absolute_output_dir = resolve_absolute_dir(output_dir);
    g_strlcpy(g_output_dir, absolute_output_dir, sizeof(g_output_dir));
    g_strlcpy(g_output_base_name, basename_owned, sizeof(g_output_base_name));
    for(int i = 0; i < ALGO_COUNT; i++){
        char* full = g_build_filename(absolute_output_dir, filename[i], NULL);
        g_strlcpy(g_archive_path[i], full, sizeof(g_archive_path[i]));
        g_free(full);
    }
    g_have_compressed = TRUE;

    // fill in the compression-related rows of the stats table
    STATS_LABELS* labels = parameters->stats;
    for(int i = 0; i < ALGO_COUNT; i++){
        double health = stats[i].files_total > 0
            ? 100.0 * stats[i].files_verified / stats[i].files_total
            : 0.0;
        double ratio = stats[i].total_original_bytes > 0
            ? (double) stats[i].total_compressed_bytes / stats[i].total_original_bytes
            : 0.0;
        double speedup = times[0] > 0.0
            ? 100.0 * (times[0] - times[i]) / times[0]
            : 0.0;

        set_percent(labels->health[i], health);
        set_seconds(labels->comp_time[i], times[i]);
        set_percent(labels->comp_speedup[i], speedup);
        set_bytes(labels->original_size[i], stats[i].total_original_bytes);
        set_bytes(labels->compressed_size[i], stats[i].total_compressed_bytes);
        set_ratio(labels->ratio[i], ratio);
    }

    // reuse the label + reveal logic the file-browse/drag-and-drop flows use,
    // but fill the entry with the exact path the archives were saved to
    // (not a name derived from the archive filename)
    char displayed_name[MAX_FILENAME];
    snprintf(displayed_name, sizeof(displayed_name), "%s.jix", basename_owned);

    gtk_label_set_text(parameters->lbl_selected_decomp_file, displayed_name);

    GtkEntryBuffer* decomp_buffer = gtk_entry_get_buffer(parameters->entry_output_name_decomp);
    gtk_entry_buffer_set_text(decomp_buffer, g_output_dir, strlen(g_output_dir));

    gtk_widget_set_visible(parameters->bottom_section_decomp, TRUE);

    g_free(absolute_output_dir);
    g_free(dirname_owned);
    g_free(basename_owned);

    navigate(GTK_WIDGET(button), &parameters->nav);
}

// ------------- decompress screen -> statistics screen -------------

typedef struct {
    NAV_PARAMETERS nav;
    GtkWindow* window;
    STATS_LABELS* stats;
} DECOMPRESS_PARAMETERS;

static void on_decompress_clicked(GtkButton* button, gpointer user_data){
    DECOMPRESS_PARAMETERS* parameters = (DECOMPRESS_PARAMETERS*) user_data;

    if(!g_have_compressed){
        show_warning_dialog(parameters->window, "Compress a folder first so there is an archive to decompress.");
        return;
    }

    double times[ALGO_COUNT];
    DecompressionStats stats[ALGO_COUNT];
    for(int i = 0; i < ALGO_COUNT; i++){
        char out_subdir[MAX_PATH + MAX_FILENAME + 50];
        snprintf(out_subdir, sizeof(out_subdir), "%s/%s_%s_out", g_output_dir, g_output_base_name, ALGO_SUFFIX[i]);

        double t0 = now_seconds();
        stats[i] = DECOMPRESS_FUNCS[i](g_archive_path[i], out_subdir);
        times[i] = now_seconds() - t0;
    }

    // fill in the decompression-related rows of the stats table
    STATS_LABELS* labels = parameters->stats;
    for(int i = 0; i < ALGO_COUNT; i++){
        double speedup = times[0] > 0.0
            ? 100.0 * (times[0] - times[i]) / times[0]
            : 0.0;

        set_seconds(labels->decomp_time[i], times[i]);
        set_percent(labels->decomp_speedup[i], speedup);
    }

    (void) stats; // files_total/successful/failed are available per algorithm
                  // if a "files ok/failed" row is added to the table later

    navigate(GTK_WIDGET(button), &parameters->nav);
}

// ------------- setup -------------

void set_up_navigation(GtkBuilder* builder, GHashTable* file_collection){

    GObject* layout_holder = gtk_builder_get_object(builder, "layout_holder");
    GObject* window_obj = gtk_builder_get_object(builder, "window");

    STATS_LABELS* stats_labels = load_stats_labels(builder);

    // ---- back button ----

    GObject* btn_back = gtk_builder_get_object(builder, "btn_back");
    gtk_widget_set_visible(GTK_WIDGET(btn_back), FALSE); // hidden on page 1 at startup
    g_signal_connect(btn_back, "clicked", G_CALLBACK(on_back_clicked), layout_holder);

    // ---- compress (btn_compile) -> decompress screen ----

    GObject* btn_compile = gtk_builder_get_object(builder, "btn_compile");
    GObject* entry_output_name = gtk_builder_get_object(builder, "entry_output_name");
    GObject* bottom_section_decomp = gtk_builder_get_object(builder, "bottom_section_decomp");
    GObject* lbl_selected_decomp_file = gtk_builder_get_object(builder, "lbl_selected_decomp_file");
    GObject* entry_output_name_decomp = gtk_builder_get_object(builder, "entry_output_name_decomp");

    COMPRESS_TO_DECOMP_PARAMETERS* compress_parameters = g_new0(COMPRESS_TO_DECOMP_PARAMETERS, 1);
    compress_parameters->nav.layout_holder = GTK_WIDGET(layout_holder);
    compress_parameters->nav.btn_back = GTK_WIDGET(btn_back);
    compress_parameters->nav.page = PAGE_DECOMPRESS;
    compress_parameters->file_collection = file_collection;
    compress_parameters->entry_output_name = GTK_ENTRY(entry_output_name);
    compress_parameters->bottom_section_decomp = GTK_WIDGET(bottom_section_decomp);
    compress_parameters->lbl_selected_decomp_file = GTK_LABEL(lbl_selected_decomp_file);
    compress_parameters->entry_output_name_decomp = GTK_ENTRY(entry_output_name_decomp);
    compress_parameters->window = GTK_WINDOW(window_obj);
    compress_parameters->stats = stats_labels;

    g_signal_connect_data(
        btn_compile, "clicked",
        G_CALLBACK(on_compress_clicked),
        compress_parameters,
        free_callback_data, 0
    );

    // ---- decompress (btn_decompress) -> statistics screen ----

    GObject* btn_decompress = gtk_builder_get_object(builder, "btn_decompress");

    DECOMPRESS_PARAMETERS* decompress_parameters = g_new0(DECOMPRESS_PARAMETERS, 1);
    decompress_parameters->nav.layout_holder = GTK_WIDGET(layout_holder);
    decompress_parameters->nav.btn_back = GTK_WIDGET(btn_back);
    decompress_parameters->nav.page = PAGE_STATS;
    decompress_parameters->window = GTK_WINDOW(window_obj);
    decompress_parameters->stats = stats_labels;

    g_signal_connect_data(
        btn_decompress, "clicked",
        G_CALLBACK(on_decompress_clicked),
        decompress_parameters,
        free_callback_data, 0
    );
}