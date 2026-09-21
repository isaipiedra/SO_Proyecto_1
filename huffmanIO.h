#ifndef HUFFMAN_IO_H
#define HUFFMAN_IO_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include "types.h"

DirectoryContent* load_directory(const char *directory_path);

void free_directory_content(DirectoryContent *content);

CompressedFile* compress_file_to_block(const FileData *file);
void free_compressed_file(CompressedFile *cf);
int write_compressed_file(FILE *out, const CompressedFile *cf);
int read_compressed_file(FILE *in, CompressedFile **out);

#endif