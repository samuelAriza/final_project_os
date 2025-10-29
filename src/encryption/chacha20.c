/**
 * @file chacha20.c
 * @brief Implementación del algoritmo ChaCha20
 */

#include "chacha20.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* Constantes de ChaCha20 ("expand 32-byte k") */
static const uint32_t CHACHA20_CONSTANTS[4] = {
    0x61707865, 0x3320646e, 0x79622d32, 0x6b206574
};

/* Macro para rotación circular hacia la izquierda */
#define ROTL32(x, n) (((x) << (n)) | ((x) >> (32 - (n))))

/* Función quarter round de ChaCha20 */
#define QUARTERROUND(a, b, c, d) \
    do { \
        a += b; d ^= a; d = ROTL32(d, 16); \
        c += d; b ^= c; b = ROTL32(b, 12); \
        a += b; d ^= a; d = ROTL32(d, 8); \
        c += d; b ^= c; b = ROTL32(b, 7); \
    } while (0)

/**
 * @brief Lee un entero de 32 bits en little-endian
 */
static uint32_t load32_le(const uint8_t *p) {
    return ((uint32_t)p[0]) | 
           ((uint32_t)p[1] << 8) | 
           ((uint32_t)p[2] << 16) | 
           ((uint32_t)p[3] << 24);
}

/**
 * @brief Escribe un entero de 32 bits en little-endian
 */
static void store32_le(uint8_t *p, uint32_t v) {
    p[0] = (uint8_t)(v);
    p[1] = (uint8_t)(v >> 8);
    p[2] = (uint8_t)(v >> 16);
    p[3] = (uint8_t)(v >> 24);
}

/**
 * @brief Ejecuta el bloque de ChaCha20
 */
static void chacha20_block(const uint32_t input[16], uint8_t output[64]) {
    uint32_t x[16];
    
    /* Copiar estado inicial */
    memcpy(x, input, sizeof(x));
    
    /* 20 rondas (10 columnas + 10 diagonales) */
    for (int i = 0; i < 10; i++) {
        /* Rondas de columna */
        QUARTERROUND(x[0], x[4], x[8],  x[12]);
        QUARTERROUND(x[1], x[5], x[9],  x[13]);
        QUARTERROUND(x[2], x[6], x[10], x[14]);
        QUARTERROUND(x[3], x[7], x[11], x[15]);
        
        /* Rondas de diagonal */
        QUARTERROUND(x[0], x[5], x[10], x[15]);
        QUARTERROUND(x[1], x[6], x[11], x[12]);
        QUARTERROUND(x[2], x[7], x[8],  x[13]);
        QUARTERROUND(x[3], x[4], x[9],  x[14]);
    }
    
    /* Sumar estado original */
    for (int i = 0; i < 16; i++) {
        x[i] += input[i];
    }
    
    /* Serializar en little-endian */
    for (int i = 0; i < 16; i++) {
        store32_le(output + (i * 4), x[i]);
    }
}

int chacha20_init(chacha20_ctx_t *ctx, const uint8_t key[CHACHA20_KEY_SIZE], 
                  const uint8_t nonce[CHACHA20_NONCE_SIZE], uint32_t counter) {
    if (!ctx || !key || !nonce) {
        return CHACHA20_ERROR_INPUT;
    }
    
    /* Inicializar estado */
    /* Constantes */
    ctx->state[0] = CHACHA20_CONSTANTS[0];
    ctx->state[1] = CHACHA20_CONSTANTS[1];
    ctx->state[2] = CHACHA20_CONSTANTS[2];
    ctx->state[3] = CHACHA20_CONSTANTS[3];
    
    /* Clave (256 bits = 8 words de 32 bits) */
    ctx->state[4]  = load32_le(key + 0);
    ctx->state[5]  = load32_le(key + 4);
    ctx->state[6]  = load32_le(key + 8);
    ctx->state[7]  = load32_le(key + 12);
    ctx->state[8]  = load32_le(key + 16);
    ctx->state[9]  = load32_le(key + 20);
    ctx->state[10] = load32_le(key + 24);
    ctx->state[11] = load32_le(key + 28);
    
    /* Contador (32 bits) */
    ctx->state[12] = counter;
    
    /* Nonce (96 bits = 3 words de 32 bits) */
    ctx->state[13] = load32_le(nonce + 0);
    ctx->state[14] = load32_le(nonce + 4);
    ctx->state[15] = load32_le(nonce + 8);
    
    /* Inicializar posición del keystream */
    ctx->keystream_pos = CHACHA20_BLOCK_SIZE;
    ctx->counter = counter;
    
    return CHACHA20_SUCCESS;
}

int chacha20_crypt(chacha20_ctx_t *ctx, const uint8_t *input, uint8_t *output, size_t length) {
    if (!ctx || !input || !output) {
        return CHACHA20_ERROR_INPUT;
    }
    
    for (size_t i = 0; i < length; i++) {
        /* Generar nuevo bloque de keystream si es necesario */
        if (ctx->keystream_pos >= CHACHA20_BLOCK_SIZE) {
            chacha20_block(ctx->state, ctx->keystream);
            ctx->keystream_pos = 0;
            
            /* Incrementar contador */
            ctx->state[12]++;
            if (ctx->state[12] == 0) {
                /* Overflow del contador - en aplicaciones reales se debería 
                   usar un nonce diferente antes de que esto ocurra */
                return CHACHA20_ERROR_INPUT;
            }
        }
        
        /* XOR byte de entrada con keystream */
        output[i] = input[i] ^ ctx->keystream[ctx->keystream_pos];
        ctx->keystream_pos++;
    }
    
    return CHACHA20_SUCCESS;
}

/**
 * @brief Hash simple para derivación de clave (NO usar en producción)
 * @details Para producción usar PBKDF2, Argon2, o scrypt
 */
static void simple_hash(const uint8_t *input, size_t input_len, 
                       uint8_t output[32]) {
    /* Algoritmo simple de hash para demostración */
    /* En producción usar PBKDF2 o similar */
    uint32_t state[8];
    
    /* Inicializar con valores arbitrarios */
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
    
    /* Rondas adicionales de mezcla */
    for (int round = 0; round < 1000; round++) {
        for (int i = 0; i < 8; i++) {
            state[i] += state[(i + 1) % 8];
            state[i] = ROTL32(state[i], 11);
        }
    }
    
    /* Escribir salida */
    for (int i = 0; i < 8; i++) {
        store32_le(output + (i * 4), state[i]);
    }
}

int chacha20_derive_key(const char *password, size_t password_len, 
                        uint8_t key[CHACHA20_KEY_SIZE]) {
    if (!password || password_len == 0 || !key) {
        return CHACHA20_ERROR_INPUT;
    }
    
    /* Derivar clave desde contraseña */
    simple_hash((const uint8_t *)password, password_len, key);
    
    return CHACHA20_SUCCESS;
}

int chacha20_generate_nonce(const uint8_t *salt, size_t salt_len, 
                            uint8_t nonce[CHACHA20_NONCE_SIZE]) {
    if (!salt || salt_len == 0 || !nonce) {
        return CHACHA20_ERROR_INPUT;
    }
    
    /* Generar nonce desde salt */
    uint8_t hash[32];
    simple_hash(salt, salt_len, hash);
    
    /* Tomar primeros 12 bytes */
    memcpy(nonce, hash, CHACHA20_NONCE_SIZE);
    
    return CHACHA20_SUCCESS;
}

/* ==================== Interfaz de alto nivel compatible con GSEA ==================== */

int chacha20_encrypt(const file_buffer_t *input, file_buffer_t *output,
                     const uint8_t *key, size_t key_len) {
    if (!input || !output || !key || key_len == 0) {
        LOG_ERROR("Invalid parameters for ChaCha20 encryption");
        return GSEA_ERROR_ARGS;
    }
    
    LOG_INFO("Starting ChaCha20 encryption (%zu bytes)", input->size);
    
    /* Derivar clave de 32 bytes */
    uint8_t derived_key[CHACHA20_KEY_SIZE];
    if (chacha20_derive_key((const char *)key, key_len, derived_key) != CHACHA20_SUCCESS) {
        LOG_ERROR("Key derivation failed");
        return GSEA_ERROR_ENCRYPTION;
    }
    
    /* Generar nonce desde la clave (determinístico para permitir desencriptación) */
    uint8_t nonce[CHACHA20_NONCE_SIZE];
    if (chacha20_generate_nonce(key, key_len, nonce) != CHACHA20_SUCCESS) {
        LOG_ERROR("Nonce generation failed");
        return GSEA_ERROR_ENCRYPTION;
    }
    
    /* Formato de salida: [nonce:12][tamaño_original:8][datos_encriptados] */
    size_t total_size = CHACHA20_NONCE_SIZE + 8 + input->size;
    output->data = malloc(total_size);
    if (!output->data) {
        LOG_ERROR("Memory allocation failed");
        return GSEA_ERROR_MEMORY;
    }
    
    output->capacity = total_size;
    output->size = total_size;
    
    /* Escribir nonce */
    memcpy(output->data, nonce, CHACHA20_NONCE_SIZE);
    
    /* Escribir tamaño original (8 bytes, little-endian) */
    size_t pos = CHACHA20_NONCE_SIZE;
    uint64_t orig_size = input->size;
    for (int i = 0; i < 8; i++) {
        output->data[pos++] = (uint8_t)(orig_size >> (i * 8));
    }
    
    /* Inicializar ChaCha20 */
    chacha20_ctx_t ctx;
    if (chacha20_init(&ctx, derived_key, nonce, 1) != CHACHA20_SUCCESS) {
        free(output->data);
        output->data = NULL;
        LOG_ERROR("ChaCha20 initialization failed");
        return GSEA_ERROR_ENCRYPTION;
    }
    
    /* Encriptar datos */
    if (chacha20_crypt(&ctx, input->data, output->data + pos, input->size) != CHACHA20_SUCCESS) {
        free(output->data);
        output->data = NULL;
        LOG_ERROR("ChaCha20 encryption failed");
        return GSEA_ERROR_ENCRYPTION;
    }
    
    /* Limpiar clave de memoria */
    memset(derived_key, 0, sizeof(derived_key));
    memset(&ctx, 0, sizeof(ctx));
    
    LOG_INFO("ChaCha20 encryption complete: %zu -> %zu bytes", 
             input->size, output->size);
    
    return GSEA_SUCCESS;
}

int chacha20_decrypt(const file_buffer_t *input, file_buffer_t *output,
                     const uint8_t *key, size_t key_len) {
    if (!input || !output || !key || key_len == 0) {
        LOG_ERROR("Invalid parameters for ChaCha20 decryption");
        return GSEA_ERROR_ARGS;
    }
    
    /* Verificar tamaño mínimo */
    if (input->size < CHACHA20_NONCE_SIZE + 8) {
        LOG_ERROR("Invalid encrypted data size");
        return GSEA_ERROR_ENCRYPTION;
    }
    
    LOG_INFO("Starting ChaCha20 decryption");
    
    /* Derivar clave de 32 bytes */
    uint8_t derived_key[CHACHA20_KEY_SIZE];
    if (chacha20_derive_key((const char *)key, key_len, derived_key) != CHACHA20_SUCCESS) {
        LOG_ERROR("Key derivation failed");
        return GSEA_ERROR_ENCRYPTION;
    }
    
    /* Leer nonce */
    uint8_t nonce[CHACHA20_NONCE_SIZE];
    memcpy(nonce, input->data, CHACHA20_NONCE_SIZE);
    
    /* Leer tamaño original */
    size_t pos = CHACHA20_NONCE_SIZE;
    uint64_t orig_size = 0;
    for (int i = 0; i < 8; i++) {
        orig_size |= ((uint64_t)input->data[pos++]) << (i * 8);
    }
    
    /* Verificar consistencia */
    if (pos + orig_size != input->size) {
        LOG_ERROR("Corrupted encrypted data");
        return GSEA_ERROR_ENCRYPTION;
    }
    
    /* Asignar buffer de salida */
    output->data = malloc(orig_size);
    if (!output->data) {
        LOG_ERROR("Memory allocation failed");
        return GSEA_ERROR_MEMORY;
    }
    
    output->capacity = orig_size;
    output->size = orig_size;
    
    /* Inicializar ChaCha20 */
    chacha20_ctx_t ctx;
    if (chacha20_init(&ctx, derived_key, nonce, 1) != CHACHA20_SUCCESS) {
        free(output->data);
        output->data = NULL;
        LOG_ERROR("ChaCha20 initialization failed");
        return GSEA_ERROR_ENCRYPTION;
    }
    
    /* Desencriptar datos (ChaCha20 es simétrico) */
    if (chacha20_crypt(&ctx, input->data + pos, output->data, orig_size) != CHACHA20_SUCCESS) {
        free(output->data);
        output->data = NULL;
        LOG_ERROR("ChaCha20 decryption failed");
        return GSEA_ERROR_ENCRYPTION;
    }
    
    /* Limpiar clave de memoria */
    memset(derived_key, 0, sizeof(derived_key));
    memset(&ctx, 0, sizeof(ctx));
    
    LOG_INFO("ChaCha20 decryption complete: %zu -> %zu bytes",
             input->size, output->size);
    
    return GSEA_SUCCESS;
}