/**
 * @file rle.c
 * @brief Implementación del algoritmo Run-Length Encoding
 */

#include "rle.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/**
 * @brief Escribe una secuencia al buffer de salida
 * @details Formato simple: [count][value] donde count es 1-255
 */
static int write_run(uint8_t **buffer, size_t *pos, size_t *capacity, 
                     uint8_t value, size_t count) {
    /* Necesitamos 2 bytes por run */
    while (*pos + 2 > *capacity) {
        *capacity *= 2;
        uint8_t *new_buffer = realloc(*buffer, *capacity);
        if (!new_buffer) return RLE_ERROR_MEMORY;
        *buffer = new_buffer;
    }
    
    /* Formato simple: [count][value] */
    (*buffer)[(*pos)++] = (uint8_t)count;
    (*buffer)[(*pos)++] = value;
    
    return RLE_SUCCESS;
}

int rle_compress(const uint8_t *input, size_t input_size, rle_compressed_t **output) {
    if (!input || input_size == 0 || !output) {
        return RLE_ERROR_INPUT;
    }
    
    /* Asignar buffer inicial (peor caso: 2x tamaño para datos no repetitivos) */
    size_t capacity = input_size * 2 + 256;
    uint8_t *buffer = malloc(capacity);
    if (!buffer) {
        return RLE_ERROR_MEMORY;
    }
    
    size_t pos = 0;
    size_t i = 0;
    
    while (i < input_size) {
        uint8_t current = input[i];
        size_t run_length = 1;
        
        /* Contar longitud de la secuencia */
        while (i + run_length < input_size && 
               input[i + run_length] == current && 
               run_length < RLE_MAX_RUN_LENGTH) {
            run_length++;
        }
        
        /* Escribir run */
        int result = write_run(&buffer, &pos, &capacity, current, run_length);
        if (result != RLE_SUCCESS) {
            free(buffer);
            return result;
        }
        
        i += run_length;
    }
    
    /* Crear estructura de salida */
    rle_compressed_t *result = malloc(sizeof(rle_compressed_t));
    if (!result) {
        free(buffer);
        return RLE_ERROR_MEMORY;
    }
    
    /* Ajustar tamaño al mínimo necesario */
    result->data = realloc(buffer, pos);
    if (!result->data && pos > 0) {
        result->data = buffer;
    }
    
    result->size = pos;
    result->original_size = input_size;
    
    *output = result;
    return RLE_SUCCESS;
}

int rle_decompress(const rle_compressed_t *compressed, uint8_t **output, size_t *output_size) {
    if (!compressed || !output || !output_size) {
        return RLE_ERROR_INPUT;
    }
    
    /* Asignar buffer de salida */
    uint8_t *buffer = malloc(compressed->original_size);
    if (!buffer) {
        return RLE_ERROR_MEMORY;
    }
    
    size_t in_pos = 0;
    size_t out_pos = 0;
    
    while (in_pos < compressed->size && out_pos < compressed->original_size) {
        /* Verificar que tenemos al menos 2 bytes */
        if (in_pos + 1 >= compressed->size) {
            free(buffer);
            return RLE_ERROR_CORRUPT;
        }
        
        uint8_t count = compressed->data[in_pos++];
        uint8_t value = compressed->data[in_pos++];
        
        /* Verificar que no excedamos el buffer de salida */
        if (out_pos + count > compressed->original_size) {
            free(buffer);
            return RLE_ERROR_CORRUPT;
        }
        
        /* Escribir secuencia */
        memset(buffer + out_pos, value, count);
        out_pos += count;
    }
    
    /* Verificar que descomprimimos exactamente el tamaño esperado */
    if (out_pos != compressed->original_size) {
        free(buffer);
        return RLE_ERROR_CORRUPT;
    }
    
    *output = buffer;
    *output_size = compressed->original_size;
    return RLE_SUCCESS;
}

void rle_free_compressed(rle_compressed_t *compressed) {
    if (!compressed) return;
    free(compressed->data);
    free(compressed);
}

int rle_serialize(const rle_compressed_t *compressed, uint8_t **output, size_t *output_size) {
    if (!compressed || !output || !output_size) {
        return RLE_ERROR_INPUT;
    }
    
    /* Formato: [original_size:8][compressed_size:8][data] */
    size_t total_size = 16 + compressed->size;
    uint8_t *buffer = malloc(total_size);
    if (!buffer) {
        return RLE_ERROR_MEMORY;
    }
    
    size_t pos = 0;
    
    /* Tamaño original (8 bytes) */
    memcpy(buffer + pos, &compressed->original_size, 8);
    pos += 8;
    
    /* Tamaño comprimido (8 bytes) */
    memcpy(buffer + pos, &compressed->size, 8);
    pos += 8;
    
    /* Datos comprimidos */
    memcpy(buffer + pos, compressed->data, compressed->size);
    
    *output = buffer;
    *output_size = total_size;
    return RLE_SUCCESS;
}

int rle_deserialize(const uint8_t *input, size_t input_size, rle_compressed_t **compressed) {
    if (!input || !compressed) {
        return RLE_ERROR_INPUT;
    }
    
    if (input_size < 16) {
        return RLE_ERROR_CORRUPT;
    }
    
    rle_compressed_t *result = malloc(sizeof(rle_compressed_t));
    if (!result) {
        return RLE_ERROR_MEMORY;
    }
    
    size_t pos = 0;
    
    /* Leer tamaño original */
    memcpy(&result->original_size, input + pos, 8);
    pos += 8;
    
    /* Leer tamaño comprimido */
    memcpy(&result->size, input + pos, 8);
    pos += 8;
    
    /* Verificar tamaño */
    if (input_size != pos + result->size) {
        free(result);
        return RLE_ERROR_CORRUPT;
    }
    
    /* Leer datos comprimidos */
    result->data = malloc(result->size);
    if (!result->data) {
        free(result);
        return RLE_ERROR_MEMORY;
    }
    
    memcpy(result->data, input + pos, result->size);
    
    *compressed = result;
    return RLE_SUCCESS;
}