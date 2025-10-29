/**
 * @file huffman.c
 * @brief Implementación del algoritmo de compresión Huffman
 */

#include "huffman.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* Estructura de nodo del árbol de Huffman */
typedef struct huffman_node {
    uint8_t symbol;
    uint32_t frequency;
    struct huffman_node *left;
    struct huffman_node *right;
} huffman_node_t;

/* Estructura para códigos Huffman */
typedef struct {
    uint8_t bits[HUFFMAN_MAX_CODE_LENGTH];
    int length;
} huffman_code_t;

/* Cola de prioridad mínima para construcción del árbol */
typedef struct {
    huffman_node_t **nodes;
    int size;
    int capacity;
} min_heap_t;

/* Funciones auxiliares de heap */
static min_heap_t *create_min_heap(int capacity) {
    min_heap_t *heap = malloc(sizeof(min_heap_t));
    if (!heap) return NULL;
    
    heap->nodes = malloc(sizeof(huffman_node_t*) * capacity);
    if (!heap->nodes) {
        free(heap);
        return NULL;
    }
    
    heap->size = 0;
    heap->capacity = capacity;
    return heap;
}

static void swap_nodes(huffman_node_t **a, huffman_node_t **b) {
    huffman_node_t *temp = *a;
    *a = *b;
    *b = temp;
}

static void min_heapify(min_heap_t *heap, int idx) {
    int smallest = idx;
    int left = 2 * idx + 1;
    int right = 2 * idx + 2;
    
    if (left < heap->size && heap->nodes[left]->frequency < heap->nodes[smallest]->frequency)
        smallest = left;
    
    if (right < heap->size && heap->nodes[right]->frequency < heap->nodes[smallest]->frequency)
        smallest = right;
    
    if (smallest != idx) {
        swap_nodes(&heap->nodes[smallest], &heap->nodes[idx]);
        min_heapify(heap, smallest);
    }
}

static huffman_node_t *extract_min(min_heap_t *heap) {
    if (heap->size == 0) return NULL;
    
    huffman_node_t *min = heap->nodes[0];
    heap->nodes[0] = heap->nodes[heap->size - 1];
    heap->size--;
    min_heapify(heap, 0);
    
    return min;
}

static void insert_min_heap(min_heap_t *heap, huffman_node_t *node) {
    if (heap->size == heap->capacity) return;
    
    int i = heap->size;
    heap->size++;
    
    while (i > 0 && node->frequency < heap->nodes[(i - 1) / 2]->frequency) {
        heap->nodes[i] = heap->nodes[(i - 1) / 2];
        i = (i - 1) / 2;
    }
    
    heap->nodes[i] = node;
}

static huffman_node_t *create_node(uint8_t symbol, uint32_t frequency) {
    huffman_node_t *node = malloc(sizeof(huffman_node_t));
    if (!node) return NULL;
    
    node->symbol = symbol;
    node->frequency = frequency;
    node->left = NULL;
    node->right = NULL;
    
    return node;
}

static void free_tree(huffman_node_t *root) {
    if (!root) return;
    free_tree(root->left);
    free_tree(root->right);
    free(root);
}

/* Construye el árbol de Huffman */
static huffman_node_t *build_huffman_tree(const uint32_t freq[HUFFMAN_SYMBOLS]) {
    min_heap_t *heap = create_min_heap(HUFFMAN_SYMBOLS);
    if (!heap) return NULL;
    
    /* Insertar nodos hoja para cada símbolo con frecuencia > 0 */
    for (int i = 0; i < HUFFMAN_SYMBOLS; i++) {
        if (freq[i] > 0) {
            huffman_node_t *node = create_node(i, freq[i]);
            if (!node) {
                for (int j = 0; j < heap->size; j++)
                    free_tree(heap->nodes[j]);
                free(heap->nodes);
                free(heap);
                return NULL;
            }
            insert_min_heap(heap, node);
        }
    }
    
    /* Construir árbol combinando nodos */
    while (heap->size > 1) {
        huffman_node_t *left = extract_min(heap);
        huffman_node_t *right = extract_min(heap);
        
        huffman_node_t *parent = create_node(0, left->frequency + right->frequency);
        if (!parent) {
            free_tree(left);
            free_tree(right);
            for (int j = 0; j < heap->size; j++)
                free_tree(heap->nodes[j]);
            free(heap->nodes);
            free(heap);
            return NULL;
        }
        
        parent->left = left;
        parent->right = right;
        insert_min_heap(heap, parent);
    }
    
    huffman_node_t *root = extract_min(heap);
    free(heap->nodes);
    free(heap);
    
    return root;
}

/* Genera códigos Huffman desde el árbol */
static void generate_codes(huffman_node_t *root, huffman_code_t codes[HUFFMAN_SYMBOLS], 
                          uint8_t code[], int depth) {
    if (!root) return;
    
    /* Nodo hoja */
    if (!root->left && !root->right) {
        codes[root->symbol].length = depth;
        memcpy(codes[root->symbol].bits, code, depth);
        return;
    }
    
    /* Recorrer izquierda (bit 0) */
    if (root->left) {
        code[depth] = 0;
        generate_codes(root->left, codes, code, depth + 1);
    }
    
    /* Recorrer derecha (bit 1) */
    if (root->right) {
        code[depth] = 1;
        generate_codes(root->right, codes, code, depth + 1);
    }
}

int huffman_compress(const uint8_t *input, size_t input_size, huffman_compressed_t **output) {
    if (!input || input_size == 0 || !output) {
        return HUFFMAN_ERROR_INPUT;
    }
    
    /* Calcular frecuencias */
    uint32_t freq[HUFFMAN_SYMBOLS] = {0};
    for (size_t i = 0; i < input_size; i++) {
        freq[input[i]]++;
    }
    
    /* Construir árbol de Huffman */
    huffman_node_t *root = build_huffman_tree(freq);
    if (!root) {
        return HUFFMAN_ERROR_MEMORY;
    }
    
    /* Caso especial: solo un símbolo único */
    if (!root->left && !root->right) {
        huffman_compressed_t *result = malloc(sizeof(huffman_compressed_t));
        if (!result) {
            free_tree(root);
            return HUFFMAN_ERROR_MEMORY;
        }
        
        result->data = malloc(1);
        if (!result->data) {
            free(result);
            free_tree(root);
            return HUFFMAN_ERROR_MEMORY;
        }
        
        result->data[0] = root->symbol;
        result->size = 1;
        result->original_size = input_size;
        memcpy(result->freq_table, freq, sizeof(freq));
        
        free_tree(root);
        *output = result;
        return HUFFMAN_SUCCESS;
    }
    
    /* Generar códigos */
    huffman_code_t codes[HUFFMAN_SYMBOLS] = {0};
    uint8_t code_buffer[HUFFMAN_MAX_CODE_LENGTH];
    generate_codes(root, codes, code_buffer, 0);
    free_tree(root);
    
    /* Calcular tamaño de salida en bits */
    size_t total_bits = 0;
    for (size_t i = 0; i < input_size; i++) {
        total_bits += codes[input[i]].length;
    }
    
    size_t output_bytes = (total_bits + 7) / 8;
    
    /* Crear estructura de salida */
    huffman_compressed_t *result = malloc(sizeof(huffman_compressed_t));
    if (!result) {
        return HUFFMAN_ERROR_MEMORY;
    }
    
    result->data = calloc(output_bytes, 1);
    if (!result->data) {
        free(result);
        return HUFFMAN_ERROR_MEMORY;
    }
    
    result->size = output_bytes;
    result->original_size = input_size;
    memcpy(result->freq_table, freq, sizeof(freq));
    
    /* Escribir datos comprimidos bit a bit */
    size_t bit_pos = 0;
    for (size_t i = 0; i < input_size; i++) {
        huffman_code_t *code = &codes[input[i]];
        for (int j = 0; j < code->length; j++) {
            if (code->bits[j]) {
                result->data[bit_pos / 8] |= (1 << (7 - (bit_pos % 8)));
            }
            bit_pos++;
        }
    }
    
    *output = result;
    return HUFFMAN_SUCCESS;
}

int huffman_decompress(const huffman_compressed_t *compressed, uint8_t **output, size_t *output_size) {
    if (!compressed || !output || !output_size) {
        return HUFFMAN_ERROR_INPUT;
    }
    
    /* Reconstruir árbol desde frecuencias */
    huffman_node_t *root = build_huffman_tree(compressed->freq_table);
    if (!root) {
        return HUFFMAN_ERROR_MEMORY;
    }
    
    /* Asignar buffer de salida */
    uint8_t *result = malloc(compressed->original_size);
    if (!result) {
        free_tree(root);
        return HUFFMAN_ERROR_MEMORY;
    }
    
    /* Caso especial: un solo símbolo */
    if (!root->left && !root->right) {
        memset(result, root->symbol, compressed->original_size);
        free_tree(root);
        *output = result;
        *output_size = compressed->original_size;
        return HUFFMAN_SUCCESS;
    }
    
    /* Decodificar bit a bit */
    huffman_node_t *current = root;
    size_t output_pos = 0;
    
    for (size_t byte_pos = 0; byte_pos < compressed->size && output_pos < compressed->original_size; byte_pos++) {
        for (int bit = 7; bit >= 0 && output_pos < compressed->original_size; bit--) {
            int bit_value = (compressed->data[byte_pos] >> bit) & 1;
            
            current = bit_value ? current->right : current->left;
            
            if (!current) {
                free(result);
                free_tree(root);
                return HUFFMAN_ERROR_CORRUPT;
            }
            
            /* Nodo hoja encontrado */
            if (!current->left && !current->right) {
                result[output_pos++] = current->symbol;
                current = root;
            }
        }
    }
    
    free_tree(root);
    
    if (output_pos != compressed->original_size) {
        free(result);
        return HUFFMAN_ERROR_CORRUPT;
    }
    
    *output = result;
    *output_size = compressed->original_size;
    return HUFFMAN_SUCCESS;
}

void huffman_free_compressed(huffman_compressed_t *compressed) {
    if (!compressed) return;
    free(compressed->data);
    free(compressed);
}

int huffman_serialize(const huffman_compressed_t *compressed, uint8_t **output, size_t *output_size) {
    if (!compressed || !output || !output_size) {
        return HUFFMAN_ERROR_INPUT;
    }
    
    /* Formato: [original_size:8][compressed_size:8][freq_table:256*4][data] */
    size_t total_size = 16 + (HUFFMAN_SYMBOLS * 4) + compressed->size;
    uint8_t *buffer = malloc(total_size);
    if (!buffer) {
        return HUFFMAN_ERROR_MEMORY;
    }
    
    size_t pos = 0;
    
    /* Tamaño original (8 bytes) */
    memcpy(buffer + pos, &compressed->original_size, 8);
    pos += 8;
    
    /* Tamaño comprimido (8 bytes) */
    memcpy(buffer + pos, &compressed->size, 8);
    pos += 8;
    
    /* Tabla de frecuencias (256 * 4 bytes) */
    memcpy(buffer + pos, compressed->freq_table, HUFFMAN_SYMBOLS * 4);
    pos += HUFFMAN_SYMBOLS * 4;
    
    /* Datos comprimidos */
    memcpy(buffer + pos, compressed->data, compressed->size);
    
    *output = buffer;
    *output_size = total_size;
    return HUFFMAN_SUCCESS;
}

int huffman_deserialize(const uint8_t *input, size_t input_size, huffman_compressed_t **compressed) {
    if (!input || !compressed) {
        return HUFFMAN_ERROR_INPUT;
    }
    
    size_t min_size = 16 + (HUFFMAN_SYMBOLS * 4);
    if (input_size < min_size) {
        return HUFFMAN_ERROR_CORRUPT;
    }
    
    huffman_compressed_t *result = malloc(sizeof(huffman_compressed_t));
    if (!result) {
        return HUFFMAN_ERROR_MEMORY;
    }
    
    size_t pos = 0;
    
    /* Leer tamaño original */
    memcpy(&result->original_size, input + pos, 8);
    pos += 8;
    
    /* Leer tamaño comprimido */
    memcpy(&result->size, input + pos, 8);
    pos += 8;
    
    /* Leer tabla de frecuencias */
    memcpy(result->freq_table, input + pos, HUFFMAN_SYMBOLS * 4);
    pos += HUFFMAN_SYMBOLS * 4;
    
    /* Verificar tamaño */
    if (input_size != pos + result->size) {
        free(result);
        return HUFFMAN_ERROR_CORRUPT;
    }
    
    /* Leer datos comprimidos */
    result->data = malloc(result->size);
    if (!result->data) {
        free(result);
        return HUFFMAN_ERROR_MEMORY;
    }
    
    memcpy(result->data, input + pos, result->size);
    
    *compressed = result;
    return HUFFMAN_SUCCESS;
}