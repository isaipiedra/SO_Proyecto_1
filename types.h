#ifndef TYPES_H
#define TYPES_H

#include <stdint.h>

// CONSTANTS

#define MAX_FILES 1000
#define MAX_FILENAME 256
#define MAX_PATH 512
#define HUFFMAN_ALPHABET 256
#define MD5_DIGEST_LENGTH 16
#define MAX_HUFFMAN_TREE_SIZE 1024

// HUFFMAN TYPES

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

// FILE / DIRECTORY TYPES

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

// STATISTICS TYPES

typedef struct {
    int files_total;
    int files_verified;
    unsigned long total_original_bytes;
    unsigned long total_compressed_bytes;
} CompressionStats;

typedef struct {
    int files_total;
    int files_successful;
    int files_failed;
    unsigned long total_original_bytes;
    unsigned long total_decompressed_bytes;
} DecompressionStats;

typedef struct {
    HuffmanNode *node;
    int priority;
} PriorityQueueElement;

typedef struct {
    char filename[MAX_FILENAME];
    int filename_len;
    unsigned char md5[MD5_DIGEST_LENGTH];
    unsigned long original_size;
    unsigned long compressed_size;
    unsigned char *tree_buffer;
    unsigned long tree_size;
    unsigned char *compressed_data;
    unsigned long bit_count;
} CompressedFile;

#endif