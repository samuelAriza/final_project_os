/**
 * @file rc4.h
 * @brief Implementación del algoritmo de encriptación RC4 (Rivest Cipher 4)
 * @author Universidad EAFIT - Sistemas Operativos
 * 
 * @details
 * RC4 es un cifrado de flujo (stream cipher) diseñado por Ron Rivest en 1987.
 * Características:
 * - Clave variable (1-256 bytes, usamos 16 para compatibilidad)
 * - Genera keystream mediante permutación de estado
 * - Extremadamente rápido y simple
 * - Encriptación/desencriptación usan la misma función (XOR con keystream)
 * 
 * NOTA: RC4 no es seguro para uso en producción moderna, pero es excelente
 * para propósitos educativos y demostración de cifrados de flujo.
 */

 #ifndef RC4_H
 #define RC4_H
 
 #include "../common.h"
 #include <stdint.h>
 #include <stddef.h>
 
 /* Constantes RC4 */
 #define RC4_KEY_SIZE 16
 #define RC4_STATE_SIZE 256
 
 /* Códigos de error */
 #define RC4_SUCCESS 0
 #define RC4_ERROR_INPUT -1
 
 /* Contexto de RC4 */
 typedef struct {
     uint8_t S[RC4_STATE_SIZE];  /* Estado interno (permutación) */
     uint8_t i;                   /* Índice i */
     uint8_t j;                   /* Índice j */
 } rc4_ctx_t;
 
 /**
  * @brief Inicializa el contexto RC4 con una clave
  * @param ctx Contexto RC4 a inicializar
  * @param key Clave de encriptación (cualquier longitud)
  * @param key_len Longitud de la clave en bytes
  * @return RC4_SUCCESS o RC4_ERROR_INPUT
  */
 int rc4_init(rc4_ctx_t *ctx, const uint8_t *key, size_t key_len);
 
 /**
  * @brief Cifra/descifra datos usando RC4 (misma operación para ambos)
  * @param ctx Contexto RC4 inicializado
  * @param input Datos de entrada
  * @param output Buffer de salida (puede ser igual que input)
  * @param length Longitud de los datos
  * @return RC4_SUCCESS o RC4_ERROR_INPUT
  */
 int rc4_crypt(rc4_ctx_t *ctx, const uint8_t *input, uint8_t *output, size_t length);
 
 /**
  * @brief Deriva clave de 16 bytes desde contraseña
  * @param password Contraseña de texto
  * @param password_len Longitud de la contraseña
  * @param key Buffer de salida para clave (16 bytes)
  * @return RC4_SUCCESS o RC4_ERROR_INPUT
  */
 int rc4_derive_key(const char *password, size_t password_len, uint8_t key[RC4_KEY_SIZE]);
 
 /* ==================== Interfaz de alto nivel compatible con GSEA ==================== */
 
 /**
  * @brief Encripta datos con RC4 (interfaz GSEA)
  * @param input Buffer de entrada con datos originales
  * @param output Buffer de salida para datos encriptados
  * @param key Clave de encriptación
  * @param key_len Longitud de la clave
  * @return GSEA_SUCCESS o código de error GSEA
  */
 int rc4_encrypt(const file_buffer_t *input, file_buffer_t *output,
                 const uint8_t *key, size_t key_len);
 
 /**
  * @brief Desencripta datos con RC4 (interfaz GSEA)
  * @param input Buffer de entrada con datos encriptados
  * @param output Buffer de salida para datos originales
  * @param key Clave de desencriptación
  * @param key_len Longitud de la clave
  * @return GSEA_SUCCESS o código de error GSEA
  */
 int rc4_decrypt(const file_buffer_t *input, file_buffer_t *output,
                 const uint8_t *key, size_t key_len);
 
 #endif /* RC4_H */