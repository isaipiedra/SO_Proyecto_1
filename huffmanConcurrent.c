#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/stat.h>
#include "huffmanIO.h"
#include "huffmanCore.h"
#include "huffmanConcurrent.h"

typedef struct {
    const FileData *file;          
    FILE *output_file;               
    pthread_mutex_t *write_mutex; 
    CompressionStats *stats;
} CompressThreadArgs;

static void* compress_worker(void *arg) {
    CompressThreadArgs *a = (CompressThreadArgs *)arg;

    CompressedFile *cf = compress_file_to_block(a->file);
    if (!cf) return NULL;

    pthread_mutex_lock(a->write_mutex);
    write_compressed_file(a->output_file, cf);
    a->stats->total_original_bytes += cf->original_size;
    a->stats->total_compressed_bytes += cf->compressed_size;
    a->stats->files_verified++;
    pthread_mutex_unlock(a->write_mutex);

    free_compressed_file(cf);
    return NULL;
}

CompressionStats huffman_compression_concurrent(const char *directory_path, const char *output_directory, const char *output_filename) {
    CompressionStats stats = {0, 0, 0, 0};

    DirectoryContent *content = load_directory(directory_path);
    if (!content || content->file_count == 0) {
        if (content) free_directory_content(content);
        return stats;
    }

    stats.files_total = content->file_count;

    char output_path[MAX_PATH];
    if (!build_output_path(output_path, sizeof(output_path), output_directory, output_filename)) {
        fprintf(stderr, "Error: output path too long\n");
        free_directory_content(content);
        return stats;
    }

    FILE *output_file = fopen(output_path, "wb");
    if (!output_file) {
        fprintf(stderr, "Error: Cannot create output file %s\n", output_path);
        free_directory_content(content);
        return stats;
    }

    fwrite("JIX1", 1, 4, output_file);
    int file_count = content->file_count;
    fwrite(&file_count, sizeof(int), 1, output_file);

    pthread_mutex_t write_mutex = PTHREAD_MUTEX_INITIALIZER;

    int n = content->file_count;
    pthread_t *threads = calloc(n, sizeof(pthread_t));
    CompressThreadArgs *args = calloc(n, sizeof(CompressThreadArgs));

    for (int i = 0; i < n; i++) {
        args[i].file = &content->files[i];
        args[i].output_file = output_file;
        args[i].write_mutex = &write_mutex;
        args[i].stats = &stats;
        if (pthread_create(&threads[i], NULL, compress_worker, &args[i]) != 0) {
            fprintf(stderr, "Error: pthread_create failed for file %d\n", i);
            threads[i] = 0;
        }
    }

    for (int i = 0; i < n; i++) {
        if (threads[i] != 0) {
            pthread_join(threads[i], NULL);
        } else {
            stats.files_total--;
        }
    }

    pthread_mutex_destroy(&write_mutex);

    free(args);
    free(threads);
    fclose(output_file);
    free_directory_content(content);
    return stats;
}

typedef struct {
    const CompressedFile *cf;
    const char *output_directory;
    int success;
} DecompressThreadArgs;

static void* decompress_worker(void *arg) {
    DecompressThreadArgs *a = (DecompressThreadArgs *)arg;
    a->success = 0;

    unsigned long tree_offset = 0;
    HuffmanNode *tree = deserialize_huffman_tree(a->cf->tree_buffer, &tree_offset);

    unsigned char *decompressed = NULL;
    unsigned long decompressed_size = decompress_data(
        a->cf->compressed_data, a->cf->compressed_size, &decompressed,
        tree, a->cf->bit_count, a->cf->original_size);

    if (decompressed_size != a->cf->original_size) {
        fprintf(stderr, "Error: size mismatch for %s\n", a->cf->filename);
        goto cleanup;
    }

    if (!verify_md5(decompressed, decompressed_size, a->cf->md5)) {
        fprintf(stderr, "Error: MD5 verification failed for %s\n", a->cf->filename);
        goto cleanup;
    }

    {
        char output_path[MAX_PATH];
        snprintf(output_path, MAX_PATH, "%s/%s", a->output_directory, a->cf->filename);
        FILE *out = fopen(output_path, "wb");
        if (!out) {
            fprintf(stderr, "Error: cannot write %s\n", output_path);
            goto cleanup;
        }
        fwrite(decompressed, 1, decompressed_size, out);
        fclose(out);
        a->success = 1;
    }

cleanup:
    free(decompressed);
    free_huffman_tree(tree);
    return NULL;
}

DecompressionStats huffman_decompression_concurrent(const char *jix_filename, const char *output_directory) {
    DecompressionStats stats = {0, 0, 0, 0, 0};

    FILE *input_file = fopen(jix_filename, "rb");
    if (!input_file) {
        fprintf(stderr, "Error: Cannot open archive %s\n", jix_filename);
        return stats;
    }

    char magic[4];
    if (fread(magic, 1, 4, input_file) != 4 || strncmp(magic, "JIX1", 4) != 0) {
        fprintf(stderr, "Error: Invalid archive format\n");
        fclose(input_file);
        return stats;
    }

    mkdir(output_directory, 0755);

    int file_count = 0;
    if (fread(&file_count, sizeof(int), 1, input_file) != 1) {
        fclose(input_file);
        return stats;
    }
    stats.files_total = file_count;

    CompressedFile **blocks = calloc(file_count, sizeof(CompressedFile *));
    int blocks_read = 0;
    for (int i = 0; i < file_count; i++) {
        if (!read_compressed_file(input_file, &blocks[i])) {
            fprintf(stderr, "Error: could not read block %d\n", i);
            break;
        }
        blocks_read++;
    }
    fclose(input_file);

    pthread_t *threads = calloc(blocks_read, sizeof(pthread_t));
    DecompressThreadArgs *args = calloc(blocks_read, sizeof(DecompressThreadArgs));

    for (int i = 0; i < blocks_read; i++) {
        args[i].cf = blocks[i];
        args[i].output_directory = output_directory;
        args[i].success = 0;
        if (pthread_create(&threads[i], NULL, decompress_worker, &args[i]) != 0) {
            fprintf(stderr, "Error: pthread_create failed for block %d\n", i);
            threads[i] = 0;
        }
    }

    for (int i = 0; i < blocks_read; i++) {
        if (threads[i] != 0) {
            pthread_join(threads[i], NULL);
        }

        stats.total_original_bytes += blocks[i]->original_size;

        if (threads[i] != 0 && args[i].success) {
            stats.files_successful++;
            stats.total_decompressed_bytes += blocks[i]->original_size;
        } else {
            stats.files_failed++;
        }
    }

    for (int i = 0; i < blocks_read; i++) {
        free_compressed_file(blocks[i]);
    }
    free(blocks);
    free(args);
    free(threads);

    return stats;
}