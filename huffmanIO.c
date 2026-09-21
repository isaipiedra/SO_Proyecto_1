#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include "huffmanIO.h"
#include "huffmanCore.h"

int is_text_file(const char *filename) {
    const char *ext = strrchr(filename, '.');
    return ext && strcmp(ext, ".txt") == 0;
}

DirectoryContent* load_directory(const char *directory_path) {
    DirectoryContent *content = malloc(sizeof(DirectoryContent));
    content->files = malloc(sizeof(FileData) * MAX_FILES);
    content->file_count = 0;

    DIR *dir = opendir(directory_path);
    if (!dir) {
        fprintf(stderr, "Error: Cannot open directory %s\n", directory_path);
        return NULL;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) && content->file_count < MAX_FILES) {
        struct stat st;
        char filepath[MAX_PATH];
        snprintf(filepath, MAX_PATH, "%s/%s", directory_path, entry->d_name);

        if (stat(filepath, &st) != 0 || !S_ISREG(st.st_mode))
            continue;

        if (!is_text_file(entry->d_name))
            continue;

        FILE *file = fopen(filepath, "rb");
        if (!file) continue;

        fseek(file, 0, SEEK_END);
        unsigned long size = ftell(file);
        fseek(file, 0, SEEK_SET);

        unsigned char *data = malloc(size);
        fread(data, 1, size, file);
        fclose(file);

        snprintf(content->files[content->file_count].filename, MAX_FILENAME, "%s", entry->d_name);
        content->files[content->file_count].data = data;
        content->files[content->file_count].size = size;
        content->file_count++;
    }

    closedir(dir);
    return content;
}

void free_directory_content(DirectoryContent *content) {
    if (!content) return;
    for (int i = 0; i < content->file_count; i++) {
        free(content->files[i].data);
    }
    free(content->files);
    free(content);
}

int build_output_path(char *out, size_t out_size, const char *output_directory, const char *filename) {
    if (!out || out_size == 0 || !filename) return 0;

    int n;
    if (output_directory && output_directory[0] != '\0') {
        n = snprintf(out, out_size, "%s/%s", output_directory, filename);
    } else {
        n = snprintf(out, out_size, "%s", filename);
    }
    return (n > 0 && (size_t)n < out_size) ? 1 : 0;
}

CompressedFile* compress_file_to_block(const FileData *file) {
    if (!file) return NULL;

    CompressedFile *cf = calloc(1, sizeof(CompressedFile));
    if (!cf) return NULL;

    // Copiar nombre
    cf->filename_len = strlen(file->filename);
    if (cf->filename_len >= MAX_FILENAME) cf->filename_len = MAX_FILENAME - 1;
    memcpy(cf->filename, file->filename, cf->filename_len);
    cf->filename[cf->filename_len] = '\0';
    cf->original_size = file->size;

    // MD5 del original
    calculate_md5(file->data, file->size, cf->md5);

    // Árbol y tabla de códigos
    unsigned long *frequencies = calculate_frequencies(file->data, file->size);
    HuffmanNode *tree = build_huffman_tree(frequencies);
    free(frequencies);

    if (!tree) {
        free(cf);
        return NULL;
    }

    HuffmanCode *codes = create_huffman_table(tree);

    unsigned char *compressed_data = NULL;
    unsigned long compressed_size = compress_data(file->data, file->size,
                                                  &compressed_data, codes);
    cf->compressed_data = compressed_data;
    cf->compressed_size = compressed_size;

    unsigned long bit_count = 0;
    for (unsigned long j = 0; j < file->size; j++)
        bit_count += codes[file->data[j]].code_length;
    cf->bit_count = bit_count;

    unsigned char *tree_buffer = malloc(MAX_HUFFMAN_TREE_SIZE);
    unsigned long tree_offset = 0;
    serialize_huffman_tree(tree, tree_buffer, &tree_offset);
    cf->tree_buffer = tree_buffer;
    cf->tree_size = tree_offset;

    unsigned char *decompressed = NULL;

    free(decompressed);
    free_huffman_tree(tree);
    free_huffman_codes(codes);

    return cf;
}

void free_compressed_file(CompressedFile *cf) {
    if (!cf) return;
    free(cf->tree_buffer);
    free(cf->compressed_data);
    free(cf);
}

int write_compressed_file(FILE *out, const CompressedFile *cf) {
    if (!out || !cf) return 0;

    if (fwrite(&cf->filename_len, sizeof(int), 1, out) != 1) return 0;
    if (fwrite(cf->filename, 1, cf->filename_len, out) != (size_t)cf->filename_len) return 0;
    if (fwrite(cf->md5, 1, MD5_DIGEST_LENGTH, out) != MD5_DIGEST_LENGTH) return 0;
    if (fwrite(&cf->original_size, sizeof(unsigned long), 1, out) != 1) return 0;
    if (fwrite(&cf->compressed_size, sizeof(unsigned long), 1, out) != 1) return 0;
    if (fwrite(&cf->tree_size, sizeof(unsigned long), 1, out) != 1) return 0;
    if (fwrite(cf->tree_buffer, 1, cf->tree_size, out) != (size_t)cf->tree_size) return 0;
    if (cf->compressed_size > 0) {
        if (fwrite(cf->compressed_data, 1, cf->compressed_size, out) != (size_t)cf->compressed_size) return 0;
    }
    if (fwrite(&cf->bit_count, sizeof(unsigned long), 1, out) != 1) return 0;

    return 1;
}

int read_compressed_file(FILE *in, CompressedFile **out) {
    *out = NULL;

    CompressedFile *cf = calloc(1, sizeof(CompressedFile));
    if (!cf) return 0;

    if (fread(&cf->filename_len, sizeof(int), 1, in) != 1) goto fail;
    if (cf->filename_len <= 0 || cf->filename_len >= MAX_FILENAME) goto fail;

    if (fread(cf->filename, 1, cf->filename_len, in) != (size_t)cf->filename_len) goto fail;
    cf->filename[cf->filename_len] = '\0';

    if (fread(cf->md5, 1, MD5_DIGEST_LENGTH, in) != MD5_DIGEST_LENGTH) goto fail;
    if (fread(&cf->original_size, sizeof(unsigned long), 1, in) != 1) goto fail;
    if (fread(&cf->compressed_size, sizeof(unsigned long), 1, in) != 1) goto fail;
    if (fread(&cf->tree_size, sizeof(unsigned long), 1, in) != 1) goto fail;

    cf->tree_buffer = malloc(cf->tree_size > 0 ? cf->tree_size : 1);
    if (!cf->tree_buffer) goto fail;
    if (fread(cf->tree_buffer, 1, cf->tree_size, in) != (size_t)cf->tree_size) goto fail;

    cf->compressed_data = malloc(cf->compressed_size > 0 ? cf->compressed_size : 1);
    if (!cf->compressed_data) goto fail;
    if (cf->compressed_size > 0) {
        if (fread(cf->compressed_data, 1, cf->compressed_size, in) != (size_t)cf->compressed_size) goto fail;
    }

    if (fread(&cf->bit_count, sizeof(unsigned long), 1, in) != 1) goto fail;

    *out = cf;
    return 1;

    fail:
        free_compressed_file(cf);
        return 0;
}