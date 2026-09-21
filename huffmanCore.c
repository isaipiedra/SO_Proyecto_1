#include <sys/stat.h>
#include <openssl/evp.h>
#include <string.h>
#include "huffmanIO.h"
#include "huffmanCore.h"

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
        HuffmanNode *left = queue[0].node;
        queue[0] = queue[queue_size - 1];
        queue_size--;
        heapify_down(queue, queue_size, 0);

        HuffmanNode *right = queue[0].node;
        queue[0] = queue[queue_size - 1];
        queue_size--;
        heapify_down(queue, queue_size, 0);

        HuffmanNode *parent = malloc(sizeof(HuffmanNode));
        parent->byte = 0;
        parent->frequency = left->frequency + right->frequency;
        parent->left = left;
        parent->right = right;

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

        if (!current) break;

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