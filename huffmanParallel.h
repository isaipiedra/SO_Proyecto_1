#ifndef HUFFMAN_PARALLEL_H
#define HUFFMAN_PARALLEL_H

#include "types.h"

CompressionStats huffman_compression_parallel(const char *directory_path, const char *output_filename);

DecompressionStats huffman_decompression_parallel(const char *jix_filename, const char *output_directory);

#endif