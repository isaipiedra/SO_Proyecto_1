#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include "huffmanIO.h"
#include "huffmanCore.h"
#include "huffmanParallel.h"

typedef struct {
    pid_t pid;
    int read_fd;
} CompressWorker;

CompressionStats huffman_compression_parallel(const char *directory_path, const char *output_directory, const char *output_filename) {
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

    CompressWorker *workers = calloc(content->file_count, sizeof(CompressWorker));

    for (int i = 0; i < content->file_count; i++) {
        int p[2];
        if (pipe(p) != 0) {
            perror("pipe");
            workers[i].pid = -1;
            workers[i].read_fd = -1;
            continue;
        }

        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
            close(p[0]);
            close(p[1]);
            workers[i].pid = -1;
            workers[i].read_fd = -1;
            continue;
        }

        if (pid == 0) {
            close(p[0]);

            CompressedFile *cf = compress_file_to_block(&content->files[i]);
            if (cf) {
                FILE *pipe_out = fdopen(p[1], "wb");
                if (pipe_out) {
                    write_compressed_file(pipe_out, cf);
                    fclose(pipe_out);
                } else {
                    close(p[1]);
                }
                free_compressed_file(cf);
            } else {
                close(p[1]);
            }

            _exit(0);
        }

        close(p[1]);
        workers[i].pid = pid;
        workers[i].read_fd = p[0];
    }

    for (int i = 0; i < content->file_count; i++) {
        if (workers[i].pid < 0) {
            stats.files_total--;
            continue;
        }

        FILE *pipe_in = fdopen(workers[i].read_fd, "rb");
        if (!pipe_in) {
            close(workers[i].read_fd);
            waitpid(workers[i].pid, NULL, 0);
            stats.files_total--;
            continue;
        }

        CompressedFile *cf = NULL;
        if (read_compressed_file(pipe_in, &cf)) {
            write_compressed_file(output_file, cf);
            stats.total_original_bytes += cf->original_size;
            stats.total_compressed_bytes += cf->compressed_size;
            stats.files_verified++;
            free_compressed_file(cf);
        } else {
            fprintf(stderr, "Error: failed to read block from child %d\n", i);
            stats.files_total--;
        }

        fclose(pipe_in);
        waitpid(workers[i].pid, NULL, 0);
    }

    free(workers);
    fclose(output_file);
    free_directory_content(content);
    return stats;
}

typedef struct {
    pid_t pid;
    int write_fd;
} DecompressWorker;

DecompressionStats huffman_decompression_parallel(const char *jix_filename, const char *output_directory) {
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

    DecompressWorker *workers = calloc(blocks_read, sizeof(DecompressWorker));

    for (int i = 0; i < blocks_read; i++) {
        int p[2];
        if (pipe(p) != 0) {
            perror("pipe");
            workers[i].pid = -1;
            workers[i].write_fd = -1;
            continue;
        }

        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
            close(p[0]);
            close(p[1]);
            workers[i].pid = -1;
            workers[i].write_fd = -1;
            continue;
        }

        if (pid == 0) {
            close(p[1]);

            FILE *pipe_in = fdopen(p[0], "rb");
            if (!pipe_in) _exit(1);

            CompressedFile *cf = NULL;
            if (!read_compressed_file(pipe_in, &cf)) {
                fclose(pipe_in);
                _exit(2);
            }
            fclose(pipe_in);

            unsigned long tree_offset = 0;
            HuffmanNode *tree = deserialize_huffman_tree(cf->tree_buffer, &tree_offset);

            unsigned char *decompressed = NULL;
            unsigned long decompressed_size = decompress_data(
                cf->compressed_data, cf->compressed_size, &decompressed,
                tree, cf->bit_count, cf->original_size);

            int ok = 0;
            if (decompressed_size == cf->original_size &&
                verify_md5(decompressed, decompressed_size, cf->md5)) {
                char output_path[MAX_PATH];
                snprintf(output_path, MAX_PATH, "%s/%s", output_directory, cf->filename);
                FILE *out = fopen(output_path, "wb");
                if (out) {
                    fwrite(decompressed, 1, decompressed_size, out);
                    fclose(out);
                    ok = 1;
                }
            }

            free(decompressed);
            free_huffman_tree(tree);
            free_compressed_file(cf);

            _exit(ok ? 0 : 3);
        }

        close(p[0]);
        workers[i].pid = pid;
        workers[i].write_fd = p[1];
    }

    for (int i = 0; i < blocks_read; i++) {
        if (workers[i].pid < 0) {
            stats.files_failed++;
            continue;
        }

        FILE *pipe_out = fdopen(workers[i].write_fd, "wb");
        if (pipe_out) {
            write_compressed_file(pipe_out, blocks[i]);
            fclose(pipe_out);
        } else {
            close(workers[i].write_fd);
        }
    }

    for (int i = 0; i < blocks_read; i++) {
        if (workers[i].pid < 0) continue;

        int status = 0;
        waitpid(workers[i].pid, &status, 0);

        stats.total_original_bytes += blocks[i]->original_size;

        if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
            stats.files_successful++;
            stats.total_decompressed_bytes += blocks[i]->original_size;
        } else {
            fprintf(stderr, "Error: child %d failed for %s\n",
                    i, blocks[i]->filename);
            stats.files_failed++;
        }
    }

    for (int i = 0; i < blocks_read; i++) {
        free_compressed_file(blocks[i]);
    }
    free(blocks);
    free(workers);

    return stats;
}