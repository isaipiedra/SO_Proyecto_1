#include "huffmanAlgorithms.h"
#include <time.h>
#include <sys/stat.h>
#include <openssl/evp.h>

typedef struct {
    HuffmanNode *node;
    int priority;
} PriorityQueueElement;

// UTILITY FUNCTIONS

char* get_filename_from_path(const char *path) {
    char *filename = strrchr(path, '/');
    return filename ? filename + 1 : (char *)path;
}

void swap_queue_elements(PriorityQueueElement *a, PriorityQueueElement *b) {
    PriorityQueueElement temp = *a;
    *a = *b;
    *b = temp;
}

int is_text_file(const char *filename) {
    const char *ext = strrchr(filename, '.');
    return ext && strcmp(ext, ".txt") == 0;
}

void heapify_down(PriorityQueueElement *queue, int size, int index) {
    int smallest = index;
    int left = 2 * index + 1;
    int right = 2 * index + 2;

    if (left < size && queue[left].priority < queue[smallest].priority)
        smallest = left;
    if (right < size && queue[right].priority < queue[smallest].priority)
        smallest = right;

    if (smallest != index) {
        swap_queue_elements(&queue[index], &queue[smallest]);
        heapify_down(queue, size, smallest);
    }
}

void heapify_up(PriorityQueueElement *queue, int index) {
    if (index == 0) return;
    int parent = (index - 1) / 2;
    if (queue[index].priority < queue[parent].priority) {
        swap_queue_elements(&queue[index], &queue[parent]);
        heapify_up(queue, parent);
    }
}

// COMPRESSION FUNCTIONS IMPLEMENTATION

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

void calculate_md5(const unsigned char *data, unsigned long size, unsigned char *digest) {
    EVP_MD_CTX *ctx = EVP_MD_CTX_new();
    unsigned int digest_len = 0;

    EVP_DigestInit_ex(ctx, EVP_md5(), NULL);
    EVP_DigestUpdate(ctx, data, size);
    EVP_DigestFinal_ex(ctx, digest, &digest_len);

    EVP_MD_CTX_free(ctx);
}

unsigned long* calculate_frequencies(const unsigned char *data, unsigned long size) {
    unsigned long *frequencies = calloc(HUFFMAN_ALPHABET, sizeof(unsigned long));
    for (unsigned long i = 0; i < size; i++) {
        frequencies[data[i]]++;
    }
    return frequencies;
}

HuffmanNode* build_huffman_tree(unsigned long *frequencies) {
    PriorityQueueElement *queue = malloc(sizeof(PriorityQueueElement) * HUFFMAN_ALPHABET);
    int queue_size = 0;

    for (int i = 0; i < HUFFMAN_ALPHABET; i++) {
        if (frequencies[i] > 0) {
            HuffmanNode *node = malloc(sizeof(HuffmanNode));
            node->byte = i;
            node->frequency = frequencies[i];
            node->left = NULL;
            node->right = NULL;

            queue[queue_size].node = node;
            queue[queue_size].priority = frequencies[i];
            queue_size++;

            int pos = queue_size - 1;
            heapify_up(queue, pos);
        }
    }

    if (queue_size == 0) {
        free(queue);
        return NULL;
    }

    while (queue_size > 1) {
        // Extraer el mínimo
        HuffmanNode *left = queue[0].node;
        queue[0] = queue[queue_size - 1];
        queue_size--;
        heapify_down(queue, queue_size, 0);

        // Extraer el segundo mínimo
        HuffmanNode *right = queue[0].node;
        queue[0] = queue[queue_size - 1];
        queue_size--;
        heapify_down(queue, queue_size, 0);

        // Crear el padre
        HuffmanNode *parent = malloc(sizeof(HuffmanNode));
        parent->byte = 0;
        parent->frequency = left->frequency + right->frequency;
        parent->left = left;
        parent->right = right;

        // Insertar el padre
        queue[queue_size].node = parent;
        queue[queue_size].priority = parent->frequency;
        queue_size++;
        heapify_up(queue, queue_size - 1);
    }

    HuffmanNode *root = queue[0].node;
    free(queue);
    return root;
}

void generate_huffman_codes(HuffmanNode *node, HuffmanCode *codes, char *current_code, int depth) {
    if (!node) return;

    if (!node->left && !node->right) {
        codes[node->byte].byte = node->byte;
        codes[node->byte].code = malloc(depth + 1);
        if (!codes[node->byte].code) return;
        memcpy(codes[node->byte].code, current_code, depth);
        codes[node->byte].code[depth] = '\0';
        codes[node->byte].code_length = depth;
        return;
    }

    current_code[depth] = '0';
    generate_huffman_codes(node->left, codes, current_code, depth + 1);

    current_code[depth] = '1';
    generate_huffman_codes(node->right, codes, current_code, depth + 1);
}

HuffmanCode* create_huffman_table(HuffmanNode *tree) {
    if (!tree) return NULL;

    HuffmanCode *codes = malloc(sizeof(HuffmanCode) * HUFFMAN_ALPHABET);
    memset(codes, 0, sizeof(HuffmanCode) * HUFFMAN_ALPHABET);

    // Caso especial: árbol con un único nodo hoja
    if (!tree->left && !tree->right) {
        codes[tree->byte].byte = tree->byte;
        codes[tree->byte].code = malloc(2);
        codes[tree->byte].code[0] = '0';
        codes[tree->byte].code[1] = '\0';
        codes[tree->byte].code_length = 1;
        return codes;
    }

    char *current_code = malloc(HUFFMAN_ALPHABET + 1);
    generate_huffman_codes(tree, codes, current_code, 0);
    free(current_code);

    return codes;
}

unsigned long compress_data(const unsigned char *data, unsigned long size,
                            unsigned char **compressed, const HuffmanCode *codes) {
    int max_code_len = 0;
    for (int i = 0; i < HUFFMAN_ALPHABET; i++) {
        if (codes[i].code && codes[i].code_length > max_code_len) {
            max_code_len = codes[i].code_length;
        }
    }

    if (max_code_len == 0 || size == 0) {
        *compressed = malloc(1);
        return 0;
    }

    unsigned long max_bits = size * (unsigned long)max_code_len;
    unsigned long bit_buffer_size = (max_bits + 7) / 8;
    unsigned char *bit_buffer = calloc(bit_buffer_size, sizeof(unsigned char));

    unsigned long bit_position = 0;

    for (unsigned long i = 0; i < size; i++) {
        unsigned char byte = data[i];
        const char *code = codes[byte].code;
        int code_len = codes[byte].code_length;

        if (!code || code_len == 0) continue;

        for (int j = 0; j < code_len; j++) {
            unsigned long bit_index = bit_position / 8;
            int bit_offset = 7 - (bit_position % 8);

            if (code[j] == '1') {
                bit_buffer[bit_index] |= (1 << bit_offset);
            }
            bit_position++;
        }
    }

    unsigned long compressed_size = (bit_position + 7) / 8;
    *compressed = malloc(compressed_size > 0 ? compressed_size : 1);
    memcpy(*compressed, bit_buffer, compressed_size);
    free(bit_buffer);
    return compressed_size;
}

void serialize_huffman_tree(HuffmanNode *node, unsigned char *buffer, unsigned long *offset) {
    if (!node) {
        buffer[(*offset)++] = 0;
        return;
    }
    if (!node->left && !node->right) {
        buffer[(*offset)++] = 1;
        buffer[(*offset)++] = node->byte;
        return;
    }
    buffer[(*offset)++] = 2;
    serialize_huffman_tree(node->left, buffer, offset);
    serialize_huffman_tree(node->right, buffer, offset);
}

void free_huffman_tree(HuffmanNode *node) {
    if (!node) return;
    free_huffman_tree(node->left);
    free_huffman_tree(node->right);
    free(node);
}

void free_huffman_codes(HuffmanCode *codes) {
    if (!codes) return;
    for (int i = 0; i < HUFFMAN_ALPHABET; i++) {
        if (codes[i].code) {
            free(codes[i].code);
        }
    }
    free(codes);
}

int huffman_compression(const char *directory_path, const char *output_filename) {
    printf("Starting compression from directory: %s\n", directory_path);

    DirectoryContent *content = load_directory(directory_path);
    if (!content || content->file_count == 0) {
        fprintf(stderr, "Error: No .txt files found in directory\n");
        return 0;
    }

    printf("Found %d files to compress\n", content->file_count);

    FILE *output_file = fopen(output_filename, "wb");
    if (!output_file) {
        fprintf(stderr, "Error: Cannot create output file %s\n", output_filename);
        free_directory_content(content);
        return 0;
    }

    // Write header
    fwrite("JIX1", 1, 4, output_file);
    int file_count = content->file_count;
    fwrite(&file_count, sizeof(int), 1, output_file);

    unsigned long total_original = 0;
    unsigned long total_compressed = 0;

    for (int i = 0; i < content->file_count; i++) {
        FileData *file = &content->files[i];
        unsigned char md5_digest[MD5_DIGEST_LENGTH];
        calculate_md5(file->data, file->size, md5_digest);

        unsigned long *frequencies = calculate_frequencies(file->data, file->size);
        HuffmanNode *tree = build_huffman_tree(frequencies);

        if (!tree) {
            fprintf(stderr, "Warning: Could not build tree for %s\n", file->filename);
            free(frequencies);
            continue;
        }

        HuffmanCode *codes = create_huffman_table(tree);
        unsigned char *compressed_data = NULL;
        unsigned long compressed_size = compress_data(file->data, file->size, &compressed_data, codes);

        // Write filename
        int filename_len = strlen(file->filename);
        fwrite(&filename_len, sizeof(int), 1, output_file);
        fwrite(file->filename, 1, filename_len, output_file);

        // Write MD5
        fwrite(md5_digest, 1, MD5_DIGEST_LENGTH, output_file);

        // Write sizes
        fwrite(&file->size, sizeof(unsigned long), 1, output_file);
        fwrite(&compressed_size, sizeof(unsigned long), 1, output_file);

        // Serialize and write tree
        unsigned char *tree_buffer = malloc(MAX_HUFFMAN_TREE_SIZE);
        unsigned long tree_offset = 0;
        serialize_huffman_tree(tree, tree_buffer, &tree_offset);
        fwrite(&tree_offset, sizeof(unsigned long), 1, output_file);
        fwrite(tree_buffer, 1, tree_offset, output_file);

        // Write compressed data
        fwrite(compressed_data, 1, compressed_size, output_file);

        // Write ending bit count for proper decompression
        unsigned long bit_count = 0;
        for (unsigned long j = 0; j < file->size; j++)
            bit_count += codes[file->data[j]].code_length;
        fwrite(&bit_count, sizeof(unsigned long), 1, output_file);

        total_original += file->size;
        total_compressed += compressed_size;

        printf("  %s: %lu -> %lu bytes (%.2f%%)\n",
               file->filename, file->size, compressed_size,
               (100.0 * compressed_size) / file->size);

        free(frequencies);
        free_huffman_tree(tree);
        free_huffman_codes(codes);
        free(compressed_data);
        free(tree_buffer);
    }

    fclose(output_file);
    free_directory_content(content);

    printf("\nCompression summary:\n");
    printf("Total original size: %lu bytes\n", total_original);
    printf("Total compressed size: %lu bytes\n", total_compressed);
    printf("Compression ratio: %.2f%%\n", (100.0 * total_compressed) / total_original);
    printf("Space saved: %lu bytes\n\n", total_original - total_compressed);

    return 1;
}

// DECOMPRESSION FUNCTIONS IMPLEMENTATION

HuffmanNode* deserialize_huffman_tree(const unsigned char *buffer, unsigned long *offset) {
    unsigned char tag = buffer[(*offset)++];
    if (tag == 0) return NULL;

    HuffmanNode *node = malloc(sizeof(HuffmanNode));
    node->left = node->right = NULL;
    node->frequency = 0;

    if (tag == 1) {
        node->byte = buffer[(*offset)++];
        return node;
    }
    
    node->left = deserialize_huffman_tree(buffer, offset);
    node->right = deserialize_huffman_tree(buffer, offset);
    return node;
}

unsigned long decompress_data(const unsigned char *compressed_data, unsigned long compressed_size,
                             unsigned char **decompressed, const HuffmanNode *tree,
                             unsigned long bit_count, unsigned long original_size) {
    unsigned char *output = malloc(original_size > 0 ? original_size : 1);
    unsigned long output_position = 0;
    unsigned long bit_position = 0;

    if (!tree) {
        *decompressed = output;
        return 0;
    }

    if (!tree->left && !tree->right) {
        for (unsigned long i = 0; i < original_size; i++) {
            output[i] = tree->byte;
        }
        *decompressed = output;
        return original_size;
    }

    const HuffmanNode *current = tree;

    while (output_position < original_size && bit_position < bit_count) {
        unsigned long byte_index = bit_position / 8;
        if (byte_index >= compressed_size) break;

        int bit_offset = 7 - (bit_position % 8);
        unsigned char bit = (compressed_data[byte_index] >> bit_offset) & 1;

        current = (bit == 0) ? current->left : current->right;
        bit_position++;

        if (!current) {
            break;
        }

        if (!current->left && !current->right) {
            output[output_position++] = current->byte;
            current = tree;
        }
    }

    *decompressed = output;
    return output_position;
}

int verify_md5(const unsigned char *data, unsigned long size, const unsigned char *expected_digest) {
    unsigned char calculated_digest[MD5_DIGEST_LENGTH];
    calculate_md5(data, size, calculated_digest);
    return memcmp(calculated_digest, expected_digest, MD5_DIGEST_LENGTH) == 0;
}

int huffman_decompression(const char *jix_filename, const char *output_directory) {
    printf("Starting decompression from: %s\n", jix_filename);

    FILE *input_file = fopen(jix_filename, "rb");
    if (!input_file) {
        fprintf(stderr, "Error: Cannot open archive %s\n", jix_filename);
        return 0;
    }

    char magic[4];
    fread(magic, 1, 4, input_file);
    if (strncmp(magic, "JIX1", 4) != 0) {
        fprintf(stderr, "Error: Invalid archive format\n");
        fclose(input_file);
        return 0;
    }

    mkdir(output_directory, 0755);

    int file_count;
    fread(&file_count, sizeof(int), 1, input_file);

    int successful = 0;
    int failed = 0;

    for (int i = 0; i < file_count; i++) {
        // Read filename
        int filename_len;
        fread(&filename_len, sizeof(int), 1, input_file);
        char *filename = malloc(filename_len + 1);
        fread(filename, 1, filename_len, input_file);
        filename[filename_len] = '\0';

        // Read MD5
        unsigned char stored_md5[MD5_DIGEST_LENGTH];
        fread(stored_md5, 1, MD5_DIGEST_LENGTH, input_file);

        // Read sizes
        unsigned long original_size;
        unsigned long compressed_size;
        fread(&original_size, sizeof(unsigned long), 1, input_file);
        fread(&compressed_size, sizeof(unsigned long), 1, input_file);

        // Read and deserialize tree
        unsigned long tree_size;
        fread(&tree_size, sizeof(unsigned long), 1, input_file);
        unsigned char *tree_buffer = malloc(tree_size);
        fread(tree_buffer, 1, tree_size, input_file);

        unsigned long tree_offset = 0;
        HuffmanNode *tree = deserialize_huffman_tree(tree_buffer, &tree_offset);

        // Read compressed data
        unsigned char *compressed_data = malloc(compressed_size);
        fread(compressed_data, 1, compressed_size, input_file);

        // Read bit count
        unsigned long bit_count;
        fread(&bit_count, sizeof(unsigned long), 1, input_file);

        // Decompress
        unsigned char *decompressed = NULL;
        unsigned long decompressed_size = decompress_data(compressed_data, compressed_size, &decompressed, tree, bit_count, original_size);

        if (decompressed_size != original_size) {
            fprintf(stderr, "Error: size mismatch for %s (expected %lu, got %lu)\n",
                    filename, original_size, decompressed_size);
            free(filename);
            free(tree_buffer);
            free(compressed_data);
            free(decompressed);
            free_huffman_tree(tree);
            failed++;
            continue;
        }

        // Verify MD5
        if (!verify_md5(decompressed, decompressed_size, stored_md5)) {
            fprintf(stderr, "Error: MD5 verification failed for %s\n", filename);
            free(filename);
            free(tree_buffer);
            free(compressed_data);
            free(decompressed);
            free_huffman_tree(tree);
            failed++;
            continue;
        }

        // Write file
        char output_path[MAX_PATH];
        snprintf(output_path, MAX_PATH, "%s/%s", output_directory, filename);
        FILE *output_file = fopen(output_path, "wb");
        if (output_file) {
            fwrite(decompressed, 1, decompressed_size, output_file);
            fclose(output_file);
            printf("  %s: %lu bytes restored (MD5 verified)\n", filename, decompressed_size);
            successful++;
        } else {
            fprintf(stderr, "Error: Cannot write file %s\n", output_path);
            failed++;
        }

        free(filename);
        free(tree_buffer);
        free(compressed_data);
        free(decompressed);
        free_huffman_tree(tree);
    }

    fclose(input_file);

    printf("\nDecompression summary:\n");
    printf("Successfully extracted: %d files\n", successful);
    printf("Failed: %d files\n\n", failed);

    return failed == 0 ? 1 : 0;
}