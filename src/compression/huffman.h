/**
 * @file huffman.h
 * @brief Implementación del algoritmo de compresión Huffman
 * @details Compresión sin pérdida basada en frecuencias de símbolos
 */

#ifndef HUFFMAN_H
#define HUFFMAN_H

#include <stdint.h>
#include <stddef.h>

/* Códigos de retorno */
#define HUFFMAN_SUCCESS 0
#define HUFFMAN_ERROR_MEMORY -1
#define HUFFMAN_ERROR_INPUT -2
#define HUFFMAN_ERROR_CORRUPT -3

/* Constantes */
#define HUFFMAN_SYMBOLS 256
#define HUFFMAN_MAX_CODE_LENGTH 256

/**
 * @brief Estructura para almacenar datos comprimidos
 */
typedef struct {
    uint8_t *data;           /* Datos comprimidos */
    size_t size;             /* Tamaño de datos comprimidos */
    size_t original_size;    /* Tamaño original */
    uint32_t freq_table[HUFFMAN_SYMBOLS]; /* Tabla de frecuencias */
} huffman_compressed_t;

/**
 * @brief Comprime datos usando el algoritmo de Huffman
 * @param input Datos de entrada
 * @param input_size Tamaño de entrada
 * @param output Estructura con datos comprimidos (debe liberarse con huffman_free_compressed)
 * @return HUFFMAN_SUCCESS en éxito, código de error en caso contrario
 */
int huffman_compress(const uint8_t *input, size_t input_size, huffman_compressed_t **output);

/**
 * @brief Descomprime datos comprimidos con Huffman
 * @param compressed Datos comprimidos
 * @param output Buffer de salida (debe liberarse por el llamador)
 * @param output_size Tamaño del buffer de salida
 * @return HUFFMAN_SUCCESS en éxito, código de error en caso contrario
 */
int huffman_decompress(const huffman_compressed_t *compressed, uint8_t **output, size_t *output_size);

/**
 * @brief Libera memoria de estructura comprimida
 * @param compressed Estructura a liberar
 */
void huffman_free_compressed(huffman_compressed_t *compressed);

/**
 * @brief Serializa datos comprimidos para escritura en archivo
 * @param compressed Datos comprimidos
 * @param output Buffer de salida
 * @param output_size Tamaño del buffer serializado
 * @return HUFFMAN_SUCCESS en éxito, código de error en caso contrario
 */
int huffman_serialize(const huffman_compressed_t *compressed, uint8_t **output, size_t *output_size);

/**
 * @brief Deserializa datos desde archivo
 * @param input Datos serializados
 * @param input_size Tamaño de entrada
 * @param compressed Estructura de salida
 * @return HUFFMAN_SUCCESS en éxito, código de error en caso contrario
 */
int huffman_deserialize(const uint8_t *input, size_t input_size, huffman_compressed_t **compressed);

#endif /* HUFFMAN_H */