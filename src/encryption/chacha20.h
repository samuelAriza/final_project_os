/**
 * @file chacha20.h
 * @brief Implementación del algoritmo de encriptación ChaCha20
 * @details Cifrado de flujo (stream cipher) diseñado por Daniel J. Bernstein
 */

#ifndef CHACHA20_H
#define CHACHA20_H

#include <stdint.h>
#include <stddef.h>
#include "../common.h"

/* Códigos de retorno */
#define CHACHA20_SUCCESS 0
#define CHACHA20_ERROR_MEMORY -1
#define CHACHA20_ERROR_INPUT -2

/* Constantes */
#define CHACHA20_KEY_SIZE 32      /* 256 bits */
#define CHACHA20_NONCE_SIZE 12    /* 96 bits */
#define CHACHA20_BLOCK_SIZE 64    /* 512 bits */

/**
 * @brief Contexto de ChaCha20
 */
typedef struct {
    uint32_t state[16];
    uint8_t keystream[CHACHA20_BLOCK_SIZE];
    size_t keystream_pos;
    uint32_t counter;
} chacha20_ctx_t;

/**
 * @brief Inicializa el contexto de ChaCha20
 * @param ctx Contexto a inicializar
 * @param key Clave de 256 bits (32 bytes)
 * @param nonce Nonce de 96 bits (12 bytes)
 * @param counter Contador inicial (normalmente 0 o 1)
 * @return CHACHA20_SUCCESS en éxito, código de error en caso contrario
 */
int chacha20_init(chacha20_ctx_t *ctx, const uint8_t key[CHACHA20_KEY_SIZE], 
                  const uint8_t nonce[CHACHA20_NONCE_SIZE], uint32_t counter);

/**
 * @brief Encripta/Desencripta datos con ChaCha20
 * @param ctx Contexto de ChaCha20
 * @param input Datos de entrada
 * @param output Buffer de salida (mismo tamaño que entrada)
 * @param length Longitud de datos
 * @return CHACHA20_SUCCESS en éxito, código de error en caso contrario
 * @note ChaCha20 es simétrico: encriptar y desencriptar es la misma operación
 */
int chacha20_crypt(chacha20_ctx_t *ctx, const uint8_t *input, uint8_t *output, size_t length);

/**
 * @brief Genera una clave desde una contraseña usando derivación simple
 * @param password Contraseña
 * @param password_len Longitud de contraseña
 * @param key Buffer de salida para clave (32 bytes)
 * @return CHACHA20_SUCCESS en éxito, código de error en caso contrario
 */
int chacha20_derive_key(const char *password, size_t password_len, uint8_t key[CHACHA20_KEY_SIZE]);

/**
 * @brief Genera un nonce desde un salt
 * @param salt Salt para generación
 * @param salt_len Longitud del salt
 * @param nonce Buffer de salida para nonce (12 bytes)
 * @return CHACHA20_SUCCESS en éxito, código de error en caso contrario
 */
int chacha20_generate_nonce(const uint8_t *salt, size_t salt_len, uint8_t nonce[CHACHA20_NONCE_SIZE]);

/* ==================== Interfaz de alto nivel compatible con GSEA ==================== */

/**
 * @brief Encripta datos usando ChaCha20 con interfaz file_buffer_t
 * @param input Buffer de entrada con datos sin encriptar
 * @param output Buffer de salida para datos encriptados
 * @param key Clave de encriptación (puede ser de cualquier tamaño, se derivará a 32 bytes)
 * @param key_len Longitud de la clave
 * @return GSEA_SUCCESS si la operación fue exitosa, código de error en caso contrario
 */
int chacha20_encrypt(const file_buffer_t *input, file_buffer_t *output,
                     const uint8_t *key, size_t key_len);

/**
 * @brief Desencripta datos usando ChaCha20 con interfaz file_buffer_t
 * @param input Buffer de entrada con datos encriptados
 * @param output Buffer de salida para datos desencriptados
 * @param key Clave de desencriptación (debe ser la misma usada para encriptar)
 * @param key_len Longitud de la clave
 * @return GSEA_SUCCESS si la operación fue exitosa, código de error en caso contrario
 */
int chacha20_decrypt(const file_buffer_t *input, file_buffer_t *output,
                     const uint8_t *key, size_t key_len);

#endif /* CHACHA20_H */