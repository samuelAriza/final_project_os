/**
 * @file salsa20.h
 * @brief Implementación del algoritmo de encriptación Salsa20
 * @details Cifrado de flujo diseñado por Daniel J. Bernstein
 */

#ifndef SALSA20_H
#define SALSA20_H

#include <stdint.h>
#include <stddef.h>
#include "../common.h"

/* Códigos de retorno */
#define SALSA20_SUCCESS 0
#define SALSA20_ERROR_MEMORY -1
#define SALSA20_ERROR_INPUT -2

/* Constantes */
#define SALSA20_KEY_SIZE 32      /* 256 bits */
#define SALSA20_NONCE_SIZE 8     /* 64 bits */
#define SALSA20_BLOCK_SIZE 64    /* 512 bits */

/**
 * @brief Contexto de Salsa20
 */
typedef struct {
    uint32_t state[16];
    uint8_t keystream[SALSA20_BLOCK_SIZE];
    size_t keystream_pos;
    uint64_t counter;
} salsa20_ctx_t;

/**
 * @brief Inicializa el contexto de Salsa20
 * @param ctx Contexto a inicializar
 * @param key Clave de 256 bits (32 bytes)
 * @param nonce Nonce de 64 bits (8 bytes)
 * @param counter Contador inicial (normalmente 0)
 * @return SALSA20_SUCCESS en éxito, código de error en caso contrario
 */
int salsa20_init(salsa20_ctx_t *ctx, const uint8_t key[SALSA20_KEY_SIZE], 
                 const uint8_t nonce[SALSA20_NONCE_SIZE], uint64_t counter);

/**
 * @brief Encripta/Desencripta datos con Salsa20
 * @param ctx Contexto de Salsa20
 * @param input Datos de entrada
 * @param output Buffer de salida (mismo tamaño que entrada)
 * @param length Longitud de datos
 * @return SALSA20_SUCCESS en éxito, código de error en caso contrario
 * @note Salsa20 es simétrico: encriptar y desencriptar es la misma operación
 */
int salsa20_crypt(salsa20_ctx_t *ctx, const uint8_t *input, uint8_t *output, size_t length);

/**
 * @brief Genera una clave desde una contraseña usando derivación simple
 * @param password Contraseña
 * @param password_len Longitud de contraseña
 * @param key Buffer de salida para clave (32 bytes)
 * @return SALSA20_SUCCESS en éxito, código de error en caso contrario
 */
int salsa20_derive_key(const char *password, size_t password_len, uint8_t key[SALSA20_KEY_SIZE]);

/**
 * @brief Genera un nonce desde un salt
 * @param salt Salt para generación
 * @param salt_len Longitud del salt
 * @param nonce Buffer de salida para nonce (8 bytes)
 * @return SALSA20_SUCCESS en éxito, código de error en caso contrario
 */
int salsa20_generate_nonce(const uint8_t *salt, size_t salt_len, uint8_t nonce[SALSA20_NONCE_SIZE]);

/* ==================== Interfaz de alto nivel compatible con GSEA ==================== */

/**
 * @brief Encripta datos usando Salsa20 con interfaz file_buffer_t
 * @param input Buffer de entrada con datos sin encriptar
 * @param output Buffer de salida para datos encriptados
 * @param key Clave de encriptación (puede ser de cualquier tamaño, se derivará a 32 bytes)
 * @param key_len Longitud de la clave
 * @return GSEA_SUCCESS si la operación fue exitosa, código de error en caso contrario
 */
int salsa20_encrypt(const file_buffer_t *input, file_buffer_t *output,
                    const uint8_t *key, size_t key_len);

/**
 * @brief Desencripta datos usando Salsa20 con interfaz file_buffer_t
 * @param input Buffer de entrada con datos encriptados
 * @param output Buffer de salida para datos desencriptados
 * @param key Clave de desencriptación (debe ser la misma usada para encriptar)
 * @param key_len Longitud de la clave
 * @return GSEA_SUCCESS si la operación fue exitosa, código de error en caso contrario
 */
int salsa20_decrypt(const file_buffer_t *input, file_buffer_t *output,
                    const uint8_t *key, size_t key_len);

#endif /* SALSA20_H */