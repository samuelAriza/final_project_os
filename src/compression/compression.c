/**
 * @file compression.c
 * @brief Implementación de la interfaz unificada de compresión
 */

#include "compression.h"
#include "lz77.h"
#include "huffman.h"
#include "rle.h"
#include "../common.h"
#include <stdlib.h>
#include <string.h>

/**
 * @brief Comprime datos usando el algoritmo especificado
 */
int compress_data(const file_buffer_t *input, file_buffer_t *output,
                  compression_algorithm_t algorithm) {
    if (!input || !output) {
        LOG_ERROR("Invalid parameters for compress_data");
        return GSEA_ERROR_ARGS;
    }
    
    switch (algorithm) {
        case COMP_LZ77:
            return lz77_compress(input, output);
            
        case COMP_HUFFMAN: {
            /* Huffman usa una estructura intermedia, necesitamos adaptar */
            huffman_compressed_t *compressed = NULL;
            int result = huffman_compress(input->data, input->size, &compressed);
            
            if (result != HUFFMAN_SUCCESS) {
                LOG_ERROR("Huffman compression failed: %d", result);
                return GSEA_ERROR_COMPRESSION;
            }
            
            /* Serializar a file_buffer_t */
            uint8_t *serialized = NULL;
            size_t serialized_size = 0;
            result = huffman_serialize(compressed, &serialized, &serialized_size);
            
            if (result != HUFFMAN_SUCCESS) {
                huffman_free_compressed(compressed);
                LOG_ERROR("Huffman serialization failed: %d", result);
                return GSEA_ERROR_COMPRESSION;
            }
            
            /* Copiar al buffer de salida */
            output->data = serialized;
            output->size = serialized_size;
            output->capacity = serialized_size;
            
            huffman_free_compressed(compressed);
            return GSEA_SUCCESS;
        }
            
        case COMP_RLE: {
            /* RLE usa una estructura intermedia similar a Huffman */
            rle_compressed_t *compressed = NULL;
            int result = rle_compress(input->data, input->size, &compressed);
            
            if (result != RLE_SUCCESS) {
                LOG_ERROR("RLE compression failed: %d", result);
                return GSEA_ERROR_COMPRESSION;
            }
            
            /* Serializar a file_buffer_t */
            uint8_t *serialized = NULL;
            size_t serialized_size = 0;
            result = rle_serialize(compressed, &serialized, &serialized_size);
            
            if (result != RLE_SUCCESS) {
                rle_free_compressed(compressed);
                LOG_ERROR("RLE serialization failed: %d", result);
                return GSEA_ERROR_COMPRESSION;
            }
            
            /* Copiar al buffer de salida */
            output->data = serialized;
            output->size = serialized_size;
            output->capacity = serialized_size;
            
            rle_free_compressed(compressed);
            return GSEA_SUCCESS;
        }
            
        default:
            LOG_ERROR("Unknown compression algorithm: %d", algorithm);
            return GSEA_ERROR_ARGS;
    }
}

/**
 * @brief Descomprime datos usando el algoritmo especificado
 */
int decompress_data(const file_buffer_t *input, file_buffer_t *output,
                    compression_algorithm_t algorithm) {
    if (!input || !output) {
        LOG_ERROR("Invalid parameters for decompress_data");
        return GSEA_ERROR_ARGS;
    }
    
    switch (algorithm) {
        case COMP_LZ77:
            return lz77_decompress(input, output);
            
        case COMP_HUFFMAN: {
            /* Deserializar desde file_buffer_t */
            huffman_compressed_t *compressed = NULL;
            int result = huffman_deserialize(input->data, input->size, &compressed);
            
            if (result != HUFFMAN_SUCCESS) {
                LOG_ERROR("Huffman deserialization failed: %d", result);
                return GSEA_ERROR_COMPRESSION;
            }
            
            /* Descomprimir */
            uint8_t *decompressed = NULL;
            size_t decompressed_size = 0;
            result = huffman_decompress(compressed, &decompressed, &decompressed_size);
            
            if (result != HUFFMAN_SUCCESS) {
                huffman_free_compressed(compressed);
                LOG_ERROR("Huffman decompression failed: %d", result);
                return GSEA_ERROR_COMPRESSION;
            }
            
            /* Copiar al buffer de salida */
            output->data = decompressed;
            output->size = decompressed_size;
            output->capacity = decompressed_size;
            
            huffman_free_compressed(compressed);
            return GSEA_SUCCESS;
        }
            
        case COMP_RLE: {
            /* Deserializar desde file_buffer_t */
            rle_compressed_t *compressed = NULL;
            int result = rle_deserialize(input->data, input->size, &compressed);
            
            if (result != RLE_SUCCESS) {
                LOG_ERROR("RLE deserialization failed: %d", result);
                return GSEA_ERROR_COMPRESSION;
            }
            
            /* Descomprimir */
            uint8_t *decompressed = NULL;
            size_t decompressed_size = 0;
            result = rle_decompress(compressed, &decompressed, &decompressed_size);
            
            if (result != RLE_SUCCESS) {
                rle_free_compressed(compressed);
                LOG_ERROR("RLE decompression failed: %d", result);
                return GSEA_ERROR_COMPRESSION;
            }
            
            /* Copiar al buffer de salida */
            output->data = decompressed;
            output->size = decompressed_size;
            output->capacity = decompressed_size;
            
            rle_free_compressed(compressed);
            return GSEA_SUCCESS;
        }
            
        default:
            LOG_ERROR("Unknown compression algorithm: %d", algorithm);
            return GSEA_ERROR_ARGS;
    }
}
