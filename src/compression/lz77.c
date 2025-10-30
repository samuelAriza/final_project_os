/**
 * @file lz77.c
 * @brief Implementación optimizada del algoritmo de compresión LZ77
 * @author Universidad EAFIT - Sistemas Operativos
 * @date Noviembre 2025
 *
 * @details
 * Algoritmo LZ77 con:
 *  - Ventana deslizante de 4096 bytes
 *  - Búsqueda hacia adelante de 18 bytes
 *  - Longitud mínima de coincidencia: 3 bytes
 *  - Hash table de 65536 entradas (16-bit hash)
 *  - Token: <offset (16), length (8), next_char (8)>
 *  - Header: 8 bytes (tamaño original)
 *
 * Justificación técnica:
 *  - LZ77 es ideal para datos repetitivos (texto, logs, genómica)
 *  - Hash table reduce complejidad de O(n*w) → O(n)
 *  - Velocidad esperada: 100-300 MB/s en CPU moderna
 *  - Compresión: 40-70% en texto, 10-30% en binario
 *
 * Referencias:
 *  - Ziv, Lempel (1977): "A Universal Algorithm for Sequential Data Compression"
 *  - Storer, Szymanski (1982): "Data Compression via Textual Substitution"
 */

#include "lz77.h"
#include "../common.h"
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

/* ==================== CONFIGURACIÓN LZ77 ==================== */
#define WINDOW_SIZE       4096u      /* Ventana de búsqueda */
#define LOOKAHEAD_SIZE    18u        /* Máximo lookahead */
#define MIN_MATCH_LENGTH  3u         /* Mínimo para codificar */
#define HASH_TABLE_SIZE   65536u     /* 2^16 entradas */
#define HASH_MASK         (HASH_TABLE_SIZE - 1)

/* Token LZ77: 4 bytes */
typedef struct {
    uint16_t offset;     /* 0 = literal */
    uint8_t length;
    uint8_t next_char;
} lz77_token_t;

/* ==================== TABLA HASH ESTÁTICA ==================== */
/* Usamos static para que sea única por hilo (segura en thread_pool) */
static uint16_t hash_table[HASH_TABLE_SIZE];

/* Hash de 3 bytes: (a << 16) | (b << 8) | c */
static inline uint32_t compute_hash(const uint8_t *p) {
    return ((uint32_t)p[0] << 16) | ((uint32_t)p[1] << 8) | p[2];
}

/* ==================== FUNCIÓN DE BÚSQUEDA OPTIMIZADA ==================== */
static size_t find_longest_match(const uint8_t *data, size_t pos, size_t data_size,
                                 size_t *best_offset, size_t *best_length) {
    size_t window_start = (pos > WINDOW_SIZE) ? pos - WINDOW_SIZE : 0;
    size_t lookahead_end = (pos + LOOKAHEAD_SIZE < data_size) ? pos + LOOKAHEAD_SIZE : data_size;

    *best_length = 0;
    *best_offset = 0;

    /* Necesitamos al menos 3 bytes para hash */
    if (pos + 2 >= data_size) return 0;

    uint32_t hash = compute_hash(&data[pos]) & HASH_MASK;
    uint32_t candidate = hash_table[hash];

    /* Solo un candidato por hash (última posición) */
    if (candidate >= window_start && candidate < pos) {
        if (data[candidate] == data[pos] &&
            data[candidate + 1] == data[pos + 1] &&
            data[candidate + 2] == data[pos + 2]) {

            size_t match_len = 3;
            while (pos + match_len < lookahead_end &&
                   data[candidate + match_len] == data[pos + match_len]) {
                match_len++;
            }

            *best_length = match_len;
            *best_offset = pos - candidate;
        }
    }

    /* Actualizar hash table (sobrescribe) */
    hash_table[hash] = (uint16_t)pos;

    return *best_length;
}

/* ==================== ESCRITURA DE TOKEN ==================== */
static int write_token(file_buffer_t *output, const lz77_token_t *token) {
    if (output->size + 4 > output->capacity) {
        size_t new_cap = (output->capacity < 8192) ? 8192 : output->capacity * 2;
        uint8_t *new_data = realloc(output->data, new_cap);
        if (!new_data) return GSEA_ERROR_MEMORY;
        output->data = new_data;
        output->capacity = new_cap;
    }

    output->data[output->size++] = (token->offset >> 8) & 0xFF;
    output->data[output->size++] = token->offset & 0xFF;
    output->data[output->size++] = token->length;
    output->data[output->size++] = token->next_char;

    return GSEA_SUCCESS;
}

/* ==================== LECTURA DE TOKEN ==================== */
static int read_token(const file_buffer_t *input, size_t *pos, lz77_token_t *token) {
    if (*pos + 4 > input->size) return GSEA_ERROR_COMPRESSION;

    token->offset  = ((uint16_t)input->data[(*pos)++] << 8);
    token->offset |= input->data[(*pos)++];
    token->length  = input->data[(*pos)++];
    token->next_char = input->data[(*pos)++];

    return GSEA_SUCCESS;
}

/* ==================== COMPRESIÓN LZ77 ==================== */
int lz77_compress(const file_buffer_t *input, file_buffer_t *output) {
    if (!input || !output || !input->data || input->size == 0) {
        LOG_ERROR("Invalid parameters for LZ77 compression");
        return GSEA_ERROR_ARGS;
    }

    LOG_INFO("Starting LZ77 compression (%zu bytes)", input->size);

    /* Inicializar hash table */
    memset(hash_table, 0, sizeof(hash_table));

    /* Buffer de salida */
    output->capacity = input->size + (input->size / 10) + 1024; /* +10% + header */
    output->data = malloc(output->capacity);
    output->size = 0;
    if (!output->data) {
        LOG_ERROR("Memory allocation failed");
        return GSEA_ERROR_MEMORY;
    }

    /* Header: tamaño original (8 bytes, big-endian) */
    uint64_t orig_size = input->size;
    for (int i = 7; i >= 0; i--) {
        output->data[output->size++] = (orig_size >> (i * 8)) & 0xFF;
    }

    size_t pos = 0;
    while (pos < input->size) {
        lz77_token_t token = {0};
        size_t best_offset = 0, best_length = 0;

        find_longest_match(input->data, pos, input->size, &best_offset, &best_length);

        if (best_length >= MIN_MATCH_LENGTH) {
            token.offset = (uint16_t)best_offset;
            token.length = (uint8_t)best_length;
            token.next_char = (pos + best_length < input->size)
                              ? input->data[pos + best_length] : 0;
            pos += best_length + 1;
        } else {
            token.offset = 0;
            token.length = 0;
            token.next_char = input->data[pos];
            pos++;
        }

        if (write_token(output, &token) != GSEA_SUCCESS) {
            free(output->data);
            output->data = NULL;
            return GSEA_ERROR_COMPRESSION;
        }
    }

    double ratio = input->size > 0
        ? (1.0 - (double)output->size / input->size) * 100.0
        : 0.0;

    LOG_INFO("LZ77 compression complete: %zu → %zu bytes (%.2f%% reduction)",
             input->size, output->size, ratio);

    return GSEA_SUCCESS;
}

/* ==================== DESCOMPRESIÓN LZ77 ==================== */
int lz77_decompress(const file_buffer_t *input, file_buffer_t *output) {
    if (!input || !output || !input->data || input->size < 12) {
        LOG_ERROR("Invalid parameters for LZ77 decompression");
        return GSEA_ERROR_ARGS;
    }

    LOG_INFO("Starting LZ77 decompression");

    /* Leer tamaño original */
    uint64_t orig_size = 0;
    for (int i = 0; i < 8; i++) {
        orig_size = (orig_size << 8) | input->data[i];
    }

    if (orig_size == 0) {
        output->size = 0;
        output->data = NULL;
        return GSEA_SUCCESS;
    }

    /* Buffer de salida */
    output->capacity = (size_t)orig_size + 1024;
    output->data = malloc(output->capacity);
    output->size = 0;
    if (!output->data) {
        LOG_ERROR("Memory allocation failed");
        return GSEA_ERROR_MEMORY;
    }

    size_t pos = 8;
    while (pos < input->size && output->size < orig_size) {
        lz77_token_t token;

        if (read_token(input, &pos, &token) != GSEA_SUCCESS) {
            LOG_ERROR("Corrupted compressed data at position %zu", pos);
            free(output->data);
            output->data = NULL;
            return GSEA_ERROR_COMPRESSION;
        }

        /* Expandir buffer si necesario */
        if (output->size + token.length + 1 > output->capacity) {
            size_t new_cap = output->capacity * 2;
            uint8_t *new_data = realloc(output->data, new_cap);
            if (!new_data) {
                free(output->data);
                output->data = NULL;
                return GSEA_ERROR_MEMORY;
            }
            output->data = new_data;
            output->capacity = new_cap;
        }

        /* Copiar referencia */
        if (token.offset > 0 && token.length > 0) {
            if (token.offset > output->size) {
                LOG_ERROR("Invalid reference in LZ77 stream");
                free(output->data);
                output->data = NULL;
                return GSEA_ERROR_COMPRESSION;
            }
            size_t ref_pos = output->size - token.offset;
            for (size_t i = 0; i < token.length; i++) {
                output->data[output->size++] = output->data[ref_pos + i];
            }
        }

        /* Añadir carácter literal */
        if (output->size < orig_size) {
            output->data[output->size++] = token.next_char;
        }
    }

    if (output->size != orig_size) {
        LOG_ERROR("Decompression size mismatch: expected %llu, got %zu",
                  (unsigned long long)orig_size, output->size);
        free(output->data);
        output->data = NULL;
        return GSEA_ERROR_COMPRESSION;
    }

    LOG_INFO("LZ77 decompression complete: %zu → %zu bytes",
             input->size, output->size);

    return GSEA_SUCCESS;
}