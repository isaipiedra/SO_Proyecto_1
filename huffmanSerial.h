#ifndef HUFFMAN_SERIAL_H
#define HUFFMAN_SERIAL_H

#include "types.h"

CompressionStats huffman_compression_serial(const char *directory_path, const char *output_filename);
DecompressionStats huffman_decompression_serial(const char *jix_filename, const char *output_directory);

#endif