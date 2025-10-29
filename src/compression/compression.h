/**
 * @file compression.h
 * @brief Interfaz unificada para algoritmos de compresión
 * @details Proporciona una API común para LZ77, Huffman y otros algoritmos
 */

#ifndef COMPRESSION_H
#define COMPRESSION_H

#include "../common.h"

/**
 * @brief Comprime datos usando el algoritmo especificado
 * @param input Buffer de entrada con datos sin comprimir
 * @param output Buffer de salida para datos comprimidos
 * @param algorithm Algoritmo de compresión a utilizar
 * @return GSEA_SUCCESS si fue exitoso, código de error en caso contrario
 */
int compress_data(const file_buffer_t *input, file_buffer_t *output,
                  compression_algorithm_t algorithm);

/**
 * @brief Descomprime datos usando el algoritmo especificado
 * @param input Buffer de entrada con datos comprimidos
 * @param output Buffer de salida para datos descomprimidos
 * @param algorithm Algoritmo de descompresión a utilizar
 * @return GSEA_SUCCESS si fue exitoso, código de error en caso contrario
 */
int decompress_data(const file_buffer_t *input, file_buffer_t *output,
                    compression_algorithm_t algorithm);

#endif /* COMPRESSION_H */
