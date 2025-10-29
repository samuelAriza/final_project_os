/**
 * @file rle.h
 * @brief Implementación del algoritmo Run-Length Encoding (RLE)
 * @details Compresión sin pérdida para datos con secuencias repetitivas
 */

#ifndef RLE_H
#define RLE_H

#include <stdint.h>
#include <stddef.h>

/* Códigos de retorno */
#define RLE_SUCCESS 0
#define RLE_ERROR_MEMORY -1
#define RLE_ERROR_INPUT -2
#define RLE_ERROR_CORRUPT -3

/* Constantes */
#define RLE_MAX_RUN_LENGTH 255
#define RLE_ESCAPE_BYTE 0xFF

/**
 * @brief Estructura para datos comprimidos con RLE
 */
typedef struct {
    uint8_t *data;          /* Datos comprimidos */
    size_t size;            /* Tamaño comprimido */
    size_t original_size;   /* Tamaño original */
} rle_compressed_t;

/**
 * @brief Comprime datos usando Run-Length Encoding
 * @param input Datos de entrada
 * @param input_size Tamaño de entrada
 * @param output Estructura con datos comprimidos (debe liberarse con rle_free_compressed)
 * @return RLE_SUCCESS en éxito, código de error en caso contrario
 */
int rle_compress(const uint8_t *input, size_t input_size, rle_compressed_t **output);

/**
 * @brief Descomprime datos comprimidos con RLE
 * @param compressed Datos comprimidos
 * @param output Buffer de salida (debe liberarse por el llamador)
 * @param output_size Tamaño del buffer de salida
 * @return RLE_SUCCESS en éxito, código de error en caso contrario
 */
int rle_decompress(const rle_compressed_t *compressed, uint8_t **output, size_t *output_size);

/**
 * @brief Libera memoria de estructura comprimida
 * @param compressed Estructura a liberar
 */
void rle_free_compressed(rle_compressed_t *compressed);

/**
 * @brief Serializa datos comprimidos para escritura en archivo
 * @param compressed Datos comprimidos
 * @param output Buffer de salida
 * @param output_size Tamaño del buffer serializado
 * @return RLE_SUCCESS en éxito, código de error en caso contrario
 */
int rle_serialize(const rle_compressed_t *compressed, uint8_t **output, size_t *output_size);

/**
 * @brief Deserializa datos desde archivo
 * @param input Datos serializados
 * @param input_size Tamaño de entrada
 * @param compressed Estructura de salida
 * @return RLE_SUCCESS en éxito, código de error en caso contrario
 */
int rle_deserialize(const uint8_t *input, size_t input_size, rle_compressed_t **compressed);

#endif /* RLE_H */