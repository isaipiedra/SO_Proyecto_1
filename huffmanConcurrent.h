#ifndef HUFFMAN_CONCURRENT_H
#define HUFFMAN_CONCURRENT_H

#include "types.h"

CompressionStats huffman_compression_concurrent(const char *directory_path, const char *output_directory, const char *output_filename);

DecompressionStats huffman_decompression_concurrent(const char *jix_filename, const char *output_directory);

#endif