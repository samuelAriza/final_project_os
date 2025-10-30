/**
 * @file aes.h
 * @brief Interfaz del algoritmo de encriptación AES-128
 */

#ifndef AES_H
#define AES_H

#include "../common.h"

/**
 * @brief Encripta datos usando AES-128 en modo ECB con padding PKCS#7
 * @param input Buffer de entrada con datos sin encriptar
 * @param output Buffer de salida para datos encriptados
 * @param key Clave de encriptación (debe ser de 16 bytes)
 * @param key_len Longitud de la clave (debe ser 16)
 * @return GSEA_SUCCESS si la operación fue exitosa, código de error en caso contrario
 */
int aes_encrypt(const file_buffer_t *input, file_buffer_t *output,
                const uint8_t *key, size_t key_len);

/**
 * @brief Desencripta datos usando AES-128 en modo ECB
 * @param input Buffer de entrada con datos encriptados
 * @param output Buffer de salida para datos desencriptados
 * @param key Clave de desencriptación (debe ser de 16 bytes)
 * @param key_len Longitud de la clave (debe ser 16)
 * @return GSEA_SUCCESS si la operación fue exitosa, código de error en caso contrario
 */
int aes_decrypt(const file_buffer_t *input, file_buffer_t *output,
                const uint8_t *key, size_t key_len);

#endif /* AES_H */