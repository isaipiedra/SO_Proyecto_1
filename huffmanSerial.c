#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "huffmanIO.h"
#include "huffmanCore.h"
#include "huffmanSerial.h"

CompressionStats huffman_compression_serial(const char *directory_path, const char *output_directory, const char *output_filename) {
    CompressionStats stats = {0, 0, 0, 0};

    DirectoryContent *content = load_directory(directory_path);
    if (!content || content->file_count == 0) {
        fprintf(stderr, "Error: No .txt files found in directory\n");
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

    for (int i = 0; i < content->file_count; i++) {
        CompressedFile *cf = compress_file_to_block(&content->files[i]);
        if (!cf) {
            fprintf(stderr, "Error: could not compress %s\n", content->files[i].filename);
            continue;
        }

        write_compressed_file(output_file, cf);

        stats.total_original_bytes += cf->original_size;
        stats.total_compressed_bytes += cf->compressed_size;

        free_compressed_file(cf);
    }

    fclose(output_file);
    free_directory_content(content);
    return stats;
}

DecompressionStats huffman_decompression_serial(const char *jix_filename, const char *output_directory) {
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

    for (int i = 0; i < file_count; i++) {
        CompressedFile *cf = NULL;
        if (!read_compressed_file(input_file, &cf)) {
            fprintf(stderr, "Error: could not read block %d\n", i);
            stats.files_failed++;
            break;
        }

        stats.total_original_bytes += cf->original_size;

        unsigned long tree_offset = 0;
        HuffmanNode *tree = deserialize_huffman_tree(cf->tree_buffer, &tree_offset);

        unsigned char *decompressed = NULL;
        unsigned long decompressed_size = decompress_data(
            cf->compressed_data, cf->compressed_size, &decompressed,
            tree, cf->bit_count, cf->original_size);

        if (decompressed_size != cf->original_size) {
            fprintf(stderr, "Error: size mismatch for %s\n", cf->filename);
            stats.files_failed++;
        } else if (!verify_md5(decompressed, decompressed_size, cf->md5)) {
            fprintf(stderr, "Error: MD5 verification failed for %s\n", cf->filename);
            stats.files_failed++;
        } else {
            char output_path[MAX_PATH];
            snprintf(output_path, MAX_PATH, "%s/%s", output_directory, cf->filename);
            FILE *out = fopen(output_path, "wb");
            if (!out) {
                fprintf(stderr, "Error: cannot write %s\n", output_path);
                stats.files_failed++;
            } else {
                fwrite(decompressed, 1, decompressed_size, out);
                fclose(out);
                stats.files_successful++;
                stats.total_decompressed_bytes += decompressed_size;
            }
        }

        free(decompressed);
        free_huffman_tree(tree);
        free_compressed_file(cf);
    }

    fclose(input_file);
    return stats;
}