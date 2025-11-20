/**
 * @file lzw.h
 * @brief Implementación del algoritmo de compresión LZW (Lempel-Ziv-Welch)
 * @author Universidad EAFIT - Sistemas Operativos
 * 
 * @details
 * LZW es un algoritmo de compresión sin pérdida diseñado por Abraham Lempel,
 * Jacob Ziv y Terry Welch. Características:
 * - Construye diccionario dinámicamente durante compresión
 * - Códigos de 9-12 bits variables
 * - Usado en GIF, TIFF, PDF, compress de Unix
 * - Excelente para texto y datos con patrones repetitivos
 * 
 * Diferencias con LZ77:
 * - LZ77 usa ventana deslizante
 * - LZW usa diccionario dinámico
 * - LZW mejor para archivos con vocabulario limitado
 */

 #ifndef LZW_H
 #define LZW_H
 
 #include "../common.h"
 #include <stdint.h>
 #include <stddef.h>
 
 /* Constantes LZW */
 #define LZW_MAX_DICT_SIZE 4096    /* 2^12 entradas máximo */
 #define LZW_INIT_DICT_SIZE 256    /* Símbolos ASCII iniciales */
 #define LZW_CLEAR_CODE 256        /* Código especial para limpiar diccionario */
 #define LZW_MAX_CODE_BITS 12      /* Bits por código */
 
 /* Códigos de error */
 #define LZW_SUCCESS 0
 #define LZW_ERROR_INPUT -1
 #define LZW_ERROR_MEMORY -2
 #define LZW_ERROR_CORRUPT -3
 
 /* Estructura de datos comprimidos LZW */
 typedef struct {
     uint16_t *codes;           /* Array de códigos comprimidos */
     size_t code_count;         /* Número de códigos */
     size_t original_size;      /* Tamaño original en bytes */
 } lzw_compressed_t;
 
 /**
  * @brief Comprime datos usando LZW
  * @param input Datos de entrada
  * @param input_size Tamaño de entrada en bytes
  * @param output Estructura de salida con datos comprimidos
  * @return LZW_SUCCESS o código de error
  */
 int lzw_compress(const uint8_t *input, size_t input_size, lzw_compressed_t **output);
 
 /**
  * @brief Descomprime datos LZW
  * @param compressed Estructura con datos comprimidos
  * @param output Buffer de salida para datos descomprimidos
  * @param output_size Tamaño de salida en bytes
  * @return LZW_SUCCESS o código de error
  */
 int lzw_decompress(const lzw_compressed_t *compressed, uint8_t **output, size_t *output_size);
 
 /**
  * @brief Libera estructura de datos comprimidos
  * @param compressed Estructura a liberar
  */
 void lzw_free_compressed(lzw_compressed_t *compressed);
 
 /**
  * @brief Serializa datos comprimidos a formato binario
  * @param compressed Estructura de datos comprimidos
  * @param output Buffer de salida para datos serializados
  * @param output_size Tamaño de salida en bytes
  * @return LZW_SUCCESS o código de error
  */
 int lzw_serialize(const lzw_compressed_t *compressed, uint8_t **output, size_t *output_size);
 
 /**
  * @brief Deserializa datos binarios a estructura comprimida
  * @param input Datos serializados
  * @param input_size Tamaño de entrada en bytes
  * @param compressed Estructura de salida
  * @return LZW_SUCCESS o código de error
  */
 int lzw_deserialize(const uint8_t *input, size_t input_size, lzw_compressed_t **compressed);
 
 #endif /* LZW_H */