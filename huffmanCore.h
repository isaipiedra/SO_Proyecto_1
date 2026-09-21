#ifndef HUFFMAN_CORE_H
#define HUFFMAN_CORE_H

#include "types.h"

unsigned long* calculate_frequencies(const unsigned char *data, unsigned long size);

HuffmanNode* build_huffman_tree(unsigned long *frequencies);

HuffmanCode* create_huffman_table(HuffmanNode *tree);

unsigned long compress_data(const unsigned char *data, unsigned long size, unsigned char **compressed, const HuffmanCode *codes);

unsigned long decompress_data(const unsigned char *compressed_data, unsigned long compressed_size, unsigned char **decompressed, const HuffmanNode *tree, unsigned long bit_count, unsigned long original_size);

void serialize_huffman_tree(HuffmanNode *node, unsigned char *buffer, unsigned long *offset);

HuffmanNode* deserialize_huffman_tree(const unsigned char *buffer, unsigned long *offset);

void free_huffman_tree(HuffmanNode *node);

void free_huffman_codes(HuffmanCode *codes);

void calculate_md5(const unsigned char *data, unsigned long size, unsigned char *digest);

int verify_md5(const unsigned char *data, unsigned long size, const unsigned char *expected_digest);

#endif