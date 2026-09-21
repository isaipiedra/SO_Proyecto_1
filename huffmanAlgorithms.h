#ifndef HUFFMAN_ALGORITHMS_H
#define HUFFMAN_ALGORITHMS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>

#define MAX_FILES 1000
#define MAX_FILENAME 256
#define MAX_PATH 512
#define HUFFMAN_ALPHABET 256
#define MD5_DIGEST_LENGTH 16
#define MAX_HUFFMAN_TREE_SIZE 1024

typedef struct HuffmanNode {
    unsigned char byte;
    unsigned long frequency;
    struct HuffmanNode *left;
    struct HuffmanNode *right;
} HuffmanNode;

typedef struct {
    unsigned char byte;
    char *code;
    int code_length;
} HuffmanCode;

typedef struct {
    char filename[MAX_FILENAME];
    unsigned char md5[MD5_DIGEST_LENGTH];
    unsigned long original_size;
    unsigned long compressed_size;
    int code_count;
    HuffmanCode *codes;
} FileMetadata;

typedef struct {
    char filename[MAX_FILENAME];
    unsigned char *data;
    unsigned long size;
} FileData;

typedef struct {
    FileData *files;
    int file_count;
} DirectoryContent;

typedef struct {
    FileMetadata *metadata;
    int metadata_count;
    unsigned char *compressed_data;
    unsigned long compressed_size;
    unsigned char *tree_buffer;
    unsigned long tree_buffer_size;
} CompressedArchive;

// COMPRESSION FUNCTIONS

DirectoryContent* load_directory(const char *directory_path);

void free_directory_content(DirectoryContent *content);

void calculate_md5(const unsigned char *data, unsigned long size, unsigned char *digest);

unsigned long* calculate_frequencies(const unsigned char *data, unsigned long size);

HuffmanNode* build_huffman_tree(unsigned long *frequencies);

void generate_huffman_codes(HuffmanNode *node, HuffmanCode *codes, char *current_code, int depth);

HuffmanCode* create_huffman_table(HuffmanNode *tree);

unsigned long compress_data(const unsigned char *data, unsigned long size, 
                           unsigned char **compressed, const HuffmanCode *codes);

int huffman_compression(const char *directory_path, const char *output_filename);

// DECOMPRESSION FUNCTIONS

CompressedArchive* load_compressed_archive(const char *jix_filename);

void free_compressed_archive(CompressedArchive *archive);

HuffmanNode* deserialize_huffman_tree(const unsigned char *buffer, unsigned long *offset);

unsigned long decompress_data(const unsigned char *compressed_data, unsigned long compressed_size,
                             unsigned char **decompressed, const HuffmanNode *tree,
                             unsigned long bit_count, unsigned long original_size);

int huffman_decompression(const char *jix_filename, const char *output_directory);

// UTILITY FUNCTIONS

void print_compression_stats(const FileMetadata *metadata, int file_count);

int verify_md5(const unsigned char *data, unsigned long size, const unsigned char *expected_digest);

void serialize_huffman_tree(HuffmanNode *node, unsigned char *buffer, unsigned long *offset);

char* get_filename_from_path(const char *path);

#endif