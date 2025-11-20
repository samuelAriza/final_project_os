/**
 * @file rc4.c
 * @brief Implementación del algoritmo RC4
 */

 #include "rc4.h"
 #include "../common.h"
 #include <stdlib.h>
 #include <string.h>
 #include <stdio.h>
 
 /**
  * @brief Macro para rotación circular hacia la izquierda
  */
 #define ROTL32(x, n) (((x) << (n)) | ((x) >> (32 - (n))))
 
 /**
  * @brief Inicializa el contexto RC4 con una clave
  */
 int rc4_init(rc4_ctx_t *ctx, const uint8_t *key, size_t key_len) {
     if (!ctx || !key || key_len == 0) {
         return RC4_ERROR_INPUT;
     }
     
     /* Inicializar array de permutación S */
     for (int i = 0; i < RC4_STATE_SIZE; i++) {
         ctx->S[i] = i;
     }
     
     /* Key Scheduling Algorithm (KSA) */
     uint8_t j = 0;
     for (int i = 0; i < RC4_STATE_SIZE; i++) {
         j = (j + ctx->S[i] + key[i % key_len]) % RC4_STATE_SIZE;
         
         /* Swap S[i] y S[j] */
         uint8_t temp = ctx->S[i];
         ctx->S[i] = ctx->S[j];
         ctx->S[j] = temp;
     }
     
     /* Inicializar índices */
     ctx->i = 0;
     ctx->j = 0;
     
     return RC4_SUCCESS;
 }
 
 /**
  * @brief Cifra/descifra datos usando RC4
  */
 int rc4_crypt(rc4_ctx_t *ctx, const uint8_t *input, uint8_t *output, size_t length) {
     if (!ctx || !input || !output) {
         return RC4_ERROR_INPUT;
     }
     
     /* Pseudo-Random Generation Algorithm (PRGA) */
     for (size_t n = 0; n < length; n++) {
         /* Incrementar i */
         ctx->i = (ctx->i + 1) % RC4_STATE_SIZE;
         
         /* Incrementar j */
         ctx->j = (ctx->j + ctx->S[ctx->i]) % RC4_STATE_SIZE;
         
         /* Swap S[i] y S[j] */
         uint8_t temp = ctx->S[ctx->i];
         ctx->S[ctx->i] = ctx->S[ctx->j];
         ctx->S[ctx->j] = temp;
         
         /* Generar byte de keystream */
         uint8_t K = ctx->S[(ctx->S[ctx->i] + ctx->S[ctx->j]) % RC4_STATE_SIZE];
         
         /* XOR con datos */
         output[n] = input[n] ^ K;
     }
     
     return RC4_SUCCESS;
 }
 
 /**
  * @brief Hash simple para derivación de clave (compatible con otros algoritmos)
  */
 static void simple_hash(const uint8_t *input, size_t input_len, uint8_t output[32]) {
     uint32_t state[8];
     
     /* Inicializar con valores del SHA-256 */
     state[0] = 0x6a09e667;
     state[1] = 0xbb67ae85;
     state[2] = 0x3c6ef372;
     state[3] = 0xa54ff53a;
     state[4] = 0x510e527f;
     state[5] = 0x9b05688c;
     state[6] = 0x1f83d9ab;
     state[7] = 0x5be0cd19;
     
     /* Procesar entrada */
     for (size_t i = 0; i < input_len; i++) {
         int idx = i % 8;
         state[idx] ^= input[i];
         state[idx] = ROTL32(state[idx], 7);
         state[(idx + 1) % 8] += state[idx];
     }
     
     /* Rondas adicionales */
     for (int round = 0; round < 1000; round++) {
         for (int i = 0; i < 8; i++) {
             state[i] += state[(i + 1) % 8];
             state[i] = ROTL32(state[i], 11);
         }
     }
     
     /* Escribir salida en little-endian */
     for (int i = 0; i < 8; i++) {
         output[i * 4 + 0] = (uint8_t)(state[i]);
         output[i * 4 + 1] = (uint8_t)(state[i] >> 8);
         output[i * 4 + 2] = (uint8_t)(state[i] >> 16);
         output[i * 4 + 3] = (uint8_t)(state[i] >> 24);
     }
 }
 
 /**
  * @brief Deriva clave de 16 bytes desde contraseña
  */
 int rc4_derive_key(const char *password, size_t password_len, uint8_t key[RC4_KEY_SIZE]) {
     if (!password || password_len == 0 || !key) {
         return RC4_ERROR_INPUT;
     }
     
     uint8_t hash[32];
     simple_hash((const uint8_t *)password, password_len, hash);
     
     /* Tomar primeros 16 bytes */
     memcpy(key, hash, RC4_KEY_SIZE);
     
     return RC4_SUCCESS;
 }
 
 /* ==================== Interfaz de alto nivel compatible con GSEA ==================== */
 
 /**
  * @brief Encripta datos con RC4 (interfaz GSEA)
  */
 int rc4_encrypt(const file_buffer_t *input, file_buffer_t *output,
                 const uint8_t *key, size_t key_len) {
     if (!input || !output || !key || key_len == 0) {
         LOG_ERROR("Invalid parameters for RC4 encryption");
         return GSEA_ERROR_ARGS;
     }
     
     if (!input->data || input->size == 0) {
         LOG_ERROR("Invalid input data for RC4 encryption");
         return GSEA_ERROR_ARGS;
     }
     
     LOG_INFO("Starting RC4 encryption (%zu bytes)", input->size);
     
     /* Derivar clave de 16 bytes */
     uint8_t derived_key[RC4_KEY_SIZE];
     if (rc4_derive_key((const char *)key, key_len, derived_key) != RC4_SUCCESS) {
         LOG_ERROR("RC4 key derivation failed");
         return GSEA_ERROR_ENCRYPTION;
     }
     
     /* Formato de salida: [tamaño_original:8][datos_encriptados] */
     size_t total_size = 8 + input->size;
     output->data = malloc(total_size);
     if (!output->data) {
         LOG_ERROR("Memory allocation failed");
         return GSEA_ERROR_MEMORY;
     }
     
     output->size = total_size;
     output->capacity = total_size;
     
     /* Escribir tamaño original (8 bytes, little-endian) */
     uint64_t orig_size = input->size;
     for (int i = 0; i < 8; i++) {
         output->data[i] = (uint8_t)(orig_size >> (i * 8));
     }
     
     /* Inicializar RC4 */
     rc4_ctx_t ctx;
     if (rc4_init(&ctx, derived_key, RC4_KEY_SIZE) != RC4_SUCCESS) {
         free(output->data);
         output->data = NULL;
         LOG_ERROR("RC4 initialization failed");
         return GSEA_ERROR_ENCRYPTION;
     }
     
     /* Encriptar datos */
     if (rc4_crypt(&ctx, input->data, output->data + 8, input->size) != RC4_SUCCESS) {
         free(output->data);
         output->data = NULL;
         LOG_ERROR("RC4 encryption failed");
         return GSEA_ERROR_ENCRYPTION;
     }
     
     /* Limpiar clave de memoria */
     memset(derived_key, 0, RC4_KEY_SIZE);
     memset(&ctx, 0, sizeof(ctx));
     
     LOG_INFO("RC4 encryption complete: %zu -> %zu bytes", input->size, output->size);
     
     return GSEA_SUCCESS;
 }
 
 /**
  * @brief Desencripta datos con RC4 (interfaz GSEA)
  */
 int rc4_decrypt(const file_buffer_t *input, file_buffer_t *output,
                 const uint8_t *key, size_t key_len) {
     if (!input || !output || !key || key_len == 0) {
         LOG_ERROR("Invalid parameters for RC4 decryption");
         return GSEA_ERROR_ARGS;
     }
     
     /* Verificar tamaño mínimo */
     if (!input->data || input->size < 8) {
         LOG_ERROR("Invalid input size for RC4 decryption");
         return GSEA_ERROR_ENCRYPTION;
     }
     
     LOG_INFO("Starting RC4 decryption");
     
     /* Derivar clave de 16 bytes */
     uint8_t derived_key[RC4_KEY_SIZE];
     if (rc4_derive_key((const char *)key, key_len, derived_key) != RC4_SUCCESS) {
         LOG_ERROR("RC4 key derivation failed");
         return GSEA_ERROR_ENCRYPTION;
     }
     
     /* Leer tamaño original */
     uint64_t orig_size = 0;
     for (int i = 0; i < 8; i++) {
         orig_size |= ((uint64_t)input->data[i]) << (i * 8);
     }
     
     /* Verificar consistencia */
     if (8 + orig_size != input->size) {
         LOG_ERROR("Corrupted RC4 encrypted data");
         return GSEA_ERROR_ENCRYPTION;
     }
     
     /* Asignar buffer de salida */
     output->data = malloc(orig_size);
     if (!output->data) {
         LOG_ERROR("Memory allocation failed");
         return GSEA_ERROR_MEMORY;
     }
     
     output->size = orig_size;
     output->capacity = orig_size;
     
     /* Inicializar RC4 */
     rc4_ctx_t ctx;
     if (rc4_init(&ctx, derived_key, RC4_KEY_SIZE) != RC4_SUCCESS) {
         free(output->data);
         output->data = NULL;
         LOG_ERROR("RC4 initialization failed");
         return GSEA_ERROR_ENCRYPTION;
     }
     
     /* Desencriptar datos (RC4 es simétrico) */
     if (rc4_crypt(&ctx, input->data + 8, output->data, orig_size) != RC4_SUCCESS) {
         free(output->data);
         output->data = NULL;
         LOG_ERROR("RC4 decryption failed");
         return GSEA_ERROR_ENCRYPTION;
     }
     
     /* Limpiar clave de memoria */
     memset(derived_key, 0, RC4_KEY_SIZE);
     memset(&ctx, 0, sizeof(ctx));
     
     LOG_INFO("RC4 decryption complete: %zu -> %zu bytes", input->size, output->size);
     
     return GSEA_SUCCESS;
 }