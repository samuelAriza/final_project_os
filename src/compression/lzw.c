/**
 * @file lzw.c
 * @brief Implementación del algoritmo LZW
 */

 #include "lzw.h"
 #include "../common.h"
 #include <stdlib.h>
 #include <string.h>
 #include <stdio.h>
 
 /* Entrada del diccionario */
 typedef struct {
     uint16_t prefix;      /* Código del prefijo (-1 si es símbolo simple) */
     uint8_t character;    /* Carácter añadido */
     bool used;            /* Si la entrada está en uso */
 } dict_entry_t;
 
 /**
  * @brief Busca secuencia en el diccionario
  */
 static int find_in_dict(dict_entry_t *dict, size_t dict_size, 
                        uint16_t prefix, uint8_t character) {
     for (size_t i = 0; i < dict_size; i++) {
         if (dict[i].used && 
             dict[i].prefix == prefix && 
             dict[i].character == character) {
             return i;
         }
     }
     return -1;
 }
 
 /**
  * @brief Comprime datos usando LZW
  */
 int lzw_compress(const uint8_t *input, size_t input_size, lzw_compressed_t **output) {
     if (!input || input_size == 0 || !output) {
         return LZW_ERROR_INPUT;
     }
     
     /* Inicializar diccionario */
     dict_entry_t *dict = calloc(LZW_MAX_DICT_SIZE, sizeof(dict_entry_t));
     if (!dict) {
         return LZW_ERROR_MEMORY;
     }
     
     /* Añadir símbolos ASCII iniciales (0-255) */
     for (int i = 0; i < LZW_INIT_DICT_SIZE; i++) {
         dict[i].prefix = 0xFFFF;  /* Sin prefijo */
         dict[i].character = i;
         dict[i].used = true;
     }
     size_t dict_size = LZW_INIT_DICT_SIZE + 1; /* +1 para CLEAR_CODE */
     
     /* Asignar buffer de códigos (peor caso: 1 código por byte) */
     size_t codes_capacity = input_size + 256;
     uint16_t *codes = malloc(codes_capacity * sizeof(uint16_t));
     if (!codes) {
         free(dict);
         return LZW_ERROR_MEMORY;
     }
     size_t code_count = 0;
     
     /* Algoritmo LZW */
     uint16_t w = input[0];  /* Primer símbolo */
     
     for (size_t i = 1; i < input_size; i++) {
         uint8_t k = input[i];
         
         /* Buscar w+k en el diccionario */
         int code = find_in_dict(dict, dict_size, w, k);
         
         if (code != -1) {
             /* w+k existe, actualizar w */
             w = code;
         } else {
             /* w+k no existe */
             /* Emitir código de w */
             if (code_count >= codes_capacity) {
                 codes_capacity *= 2;
                 uint16_t *new_codes = realloc(codes, codes_capacity * sizeof(uint16_t));
                 if (!new_codes) {
                     free(codes);
                     free(dict);
                     return LZW_ERROR_MEMORY;
                 }
                 codes = new_codes;
             }
             codes[code_count++] = w;
             
             /* Añadir w+k al diccionario si hay espacio */
             if (dict_size < LZW_MAX_DICT_SIZE) {
                 dict[dict_size].prefix = w;
                 dict[dict_size].character = k;
                 dict[dict_size].used = true;
                 dict_size++;
             }
             
             /* w = k */
             w = k;
         }
     }
     
     /* Emitir último código */
     codes[code_count++] = w;
     
     /* Crear estructura de salida */
     lzw_compressed_t *result = malloc(sizeof(lzw_compressed_t));
     if (!result) {
         free(codes);
         free(dict);
         return LZW_ERROR_MEMORY;
     }
     
     /* Ajustar tamaño del array de códigos */
     uint16_t *temp_codes = realloc(codes, code_count * sizeof(uint16_t));
     if (!temp_codes && code_count > 0) {
         result->codes = codes;
     } else {
         result->codes = temp_codes;
     }
     result->code_count = code_count;
     result->original_size = input_size;
     
     free(dict);
     *output = result;
     
     return LZW_SUCCESS;
 }
 
 /**
  * @brief Descomprime datos LZW
  */
 int lzw_decompress(const lzw_compressed_t *compressed, uint8_t **output, size_t *output_size) {
     if (!compressed || !output || !output_size) {
         return LZW_ERROR_INPUT;
     }
     
     if (compressed->code_count == 0) {
         *output = NULL;
         *output_size = 0;
         return LZW_SUCCESS;
     }
     
     /* Inicializar diccionario de descompresión */
     /* Cada entrada guarda la secuencia completa */
     uint8_t **dict = malloc(LZW_MAX_DICT_SIZE * sizeof(uint8_t *));
     size_t *dict_lengths = calloc(LZW_MAX_DICT_SIZE, sizeof(size_t));
     if (!dict || !dict_lengths) {
         free(dict);
         free(dict_lengths);
         return LZW_ERROR_MEMORY;
     }
     
     /* Inicializar con símbolos ASCII */
     for (int i = 0; i < LZW_INIT_DICT_SIZE; i++) {
         dict[i] = malloc(1);
         if (!dict[i]) {
             for (int j = 0; j < i; j++) free(dict[j]);
             free(dict);
             free(dict_lengths);
             return LZW_ERROR_MEMORY;
         }
         dict[i][0] = i;
         dict_lengths[i] = 1;
     }
     size_t dict_size = LZW_INIT_DICT_SIZE + 1;
     
     /* Asignar buffer de salida */
     uint8_t *result = malloc(compressed->original_size);
     if (!result) {
         for (int i = 0; i < LZW_INIT_DICT_SIZE; i++) free(dict[i]);
         free(dict);
         free(dict_lengths);
         return LZW_ERROR_MEMORY;
     }
     size_t out_pos = 0;
     
     /* Primer código */
     uint16_t old_code = compressed->codes[0];
     if (old_code >= LZW_INIT_DICT_SIZE) {
         free(result);
         for (int i = 0; i < LZW_INIT_DICT_SIZE; i++) free(dict[i]);
         free(dict);
         free(dict_lengths);
         return LZW_ERROR_CORRUPT;
     }
     
     result[out_pos++] = old_code;
     
     /* Procesar códigos restantes */
     for (size_t i = 1; i < compressed->code_count; i++) {
         uint16_t code = compressed->codes[i];
         
         uint8_t *sequence;
         size_t seq_len;
         
         if (code < dict_size) {
             /* Código existe en diccionario */
             sequence = dict[code];
             seq_len = dict_lengths[code];
         } else if (code == dict_size) {
             /* Caso especial: código no existe aún */
             seq_len = dict_lengths[old_code] + 1;
             sequence = malloc(seq_len);
             if (!sequence) {
                 free(result);
                 for (int j = 0; j < LZW_INIT_DICT_SIZE; j++) free(dict[j]);
                 for (size_t j = LZW_INIT_DICT_SIZE + 1; j < dict_size; j++) 
                     if (dict[j]) free(dict[j]);
                 free(dict);
                 free(dict_lengths);
                 return LZW_ERROR_MEMORY;
             }
             memcpy(sequence, dict[old_code], dict_lengths[old_code]);
             sequence[seq_len - 1] = dict[old_code][0];
         } else {
             /* Código inválido */
             free(result);
             for (int j = 0; j < LZW_INIT_DICT_SIZE; j++) free(dict[j]);
             for (size_t j = LZW_INIT_DICT_SIZE + 1; j < dict_size; j++) 
                 if (dict[j]) free(dict[j]);
             free(dict);
             free(dict_lengths);
             return LZW_ERROR_CORRUPT;
         }
         
         /* Escribir secuencia */
         if (out_pos + seq_len > compressed->original_size) {
             if (code == dict_size) free(sequence);
             free(result);
             for (int j = 0; j < LZW_INIT_DICT_SIZE; j++) free(dict[j]);
             for (size_t j = LZW_INIT_DICT_SIZE + 1; j < dict_size; j++) 
                 if (dict[j]) free(dict[j]);
             free(dict);
             free(dict_lengths);
             return LZW_ERROR_CORRUPT;
         }
         memcpy(result + out_pos, sequence, seq_len);
         out_pos += seq_len;
         
         /* Añadir al diccionario */
         if (dict_size < LZW_MAX_DICT_SIZE) {
             size_t new_len = dict_lengths[old_code] + 1;
             dict[dict_size] = malloc(new_len);
             if (dict[dict_size]) {
                 memcpy(dict[dict_size], dict[old_code], dict_lengths[old_code]);
                 dict[dict_size][new_len - 1] = sequence[0];
                 dict_lengths[dict_size] = new_len;
                 dict_size++;
             }
         }
         
         if (code == dict_size - 1) {
             free(sequence);
         }
         
         old_code = code;
     }
     
     /* Limpiar diccionario */
     for (int i = 0; i < LZW_INIT_DICT_SIZE; i++) free(dict[i]);
     for (size_t i = LZW_INIT_DICT_SIZE + 1; i < dict_size; i++) {
         if (dict[i]) free(dict[i]);
     }
     free(dict);
     free(dict_lengths);
     
     *output = result;
     *output_size = compressed->original_size;
     
     return LZW_SUCCESS;
 }
 
 /**
  * @brief Libera estructura de datos comprimidos
  */
 void lzw_free_compressed(lzw_compressed_t *compressed) {
     if (!compressed) return;
     free(compressed->codes);
     free(compressed);
 }
 
 /**
  * @brief Serializa datos comprimidos
  */
 int lzw_serialize(const lzw_compressed_t *compressed, uint8_t **output, size_t *output_size) {
     if (!compressed || !output || !output_size) {
         return LZW_ERROR_INPUT;
     }
     
     /* Formato: [original_size:8][code_count:8][codes:code_count*2] */
     size_t total_size = 16 + (compressed->code_count * 2);
     uint8_t *buffer = malloc(total_size);
     if (!buffer) {
         return LZW_ERROR_MEMORY;
     }
     
     size_t pos = 0;
     
     /* Tamaño original (8 bytes) */
     memcpy(buffer + pos, &compressed->original_size, 8);
     pos += 8;
     
     /* Número de códigos (8 bytes) */
     memcpy(buffer + pos, &compressed->code_count, 8);
     pos += 8;
     
     /* Códigos (2 bytes cada uno, little-endian) */
     for (size_t i = 0; i < compressed->code_count; i++) {
         buffer[pos++] = compressed->codes[i] & 0xFF;
         buffer[pos++] = (compressed->codes[i] >> 8) & 0xFF;
     }
     
     *output = buffer;
     *output_size = total_size;
     return LZW_SUCCESS;
 }
 
 /**
  * @brief Deserializa datos binarios
  */
 int lzw_deserialize(const uint8_t *input, size_t input_size, lzw_compressed_t **compressed) {
     if (!input || !compressed) {
         return LZW_ERROR_INPUT;
     }
     
     if (input_size < 16) {
         return LZW_ERROR_CORRUPT;
     }
     
     lzw_compressed_t *result = malloc(sizeof(lzw_compressed_t));
     if (!result) {
         return LZW_ERROR_MEMORY;
     }
     
     size_t pos = 0;
     
     /* Leer tamaño original */
     memcpy(&result->original_size, input + pos, 8);
     pos += 8;
     
     /* Leer número de códigos */
     memcpy(&result->code_count, input + pos, 8);
     pos += 8;
     
     /* Verificar tamaño */
     if (input_size != pos + (result->code_count * 2)) {
         free(result);
         return LZW_ERROR_CORRUPT;
     }
     
     /* Leer códigos */
     result->codes = malloc(result->code_count * sizeof(uint16_t));
     if (!result->codes) {
         free(result);
         return LZW_ERROR_MEMORY;
     }
     
     for (size_t i = 0; i < result->code_count; i++) {
         result->codes[i] = input[pos] | (input[pos + 1] << 8);
         pos += 2;
     }
     
     *compressed = result;
     return LZW_SUCCESS;
 }