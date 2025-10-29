/**
 * @file salsa20.c
 * @brief Implementación del algoritmo Salsa20
 */

#include "salsa20.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/* Constantes de Salsa20 ("expand 32-byte k") */
static const uint32_t SALSA20_CONSTANTS[4] = {
    0x61707865, 0x3320646e, 0x79622d32, 0x6b206574
};

/* Macro para rotación circular hacia la izquierda */
#define ROTL32(x, n) (((x) << (n)) | ((x) >> (32 - (n))))

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
 * @brief Función quarter round de Salsa20
 */
static void salsa20_quarter_round(uint32_t *y0, uint32_t *y1, uint32_t *y2, uint32_t *y3) {
    *y1 ^= ROTL32(*y0 + *y3, 7);
    *y2 ^= ROTL32(*y1 + *y0, 9);
    *y3 ^= ROTL32(*y2 + *y1, 13);
    *y0 ^= ROTL32(*y3 + *y2, 18);
}

/**
 * @brief Función row round de Salsa20
 */
static void salsa20_row_round(uint32_t y[16]) {
    salsa20_quarter_round(&y[0], &y[1], &y[2], &y[3]);
    salsa20_quarter_round(&y[5], &y[6], &y[7], &y[4]);
    salsa20_quarter_round(&y[10], &y[11], &y[8], &y[9]);
    salsa20_quarter_round(&y[15], &y[12], &y[13], &y[14]);
}

/**
 * @brief Función column round de Salsa20
 */
static void salsa20_column_round(uint32_t x[16]) {
    salsa20_quarter_round(&x[0], &x[4], &x[8], &x[12]);
    salsa20_quarter_round(&x[5], &x[9], &x[13], &x[1]);
    salsa20_quarter_round(&x[10], &x[14], &x[2], &x[6]);
    salsa20_quarter_round(&x[15], &x[3], &x[7], &x[11]);
}

/**
 * @brief Función double round de Salsa20
 */
static void salsa20_double_round(uint32_t x[16]) {
    salsa20_column_round(x);
    salsa20_row_round(x);
}

/**
 * @brief Ejecuta el bloque de Salsa20 (20 rondas)
 */
static void salsa20_block(const uint32_t input[16], uint8_t output[64]) {
    uint32_t x[16];
    
    /* Copiar estado inicial */
    memcpy(x, input, sizeof(x));
    
    /* 20 rondas = 10 double rounds */
    for (int i = 0; i < 10; i++) {
        salsa20_double_round(x);
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

int salsa20_init(salsa20_ctx_t *ctx, const uint8_t key[SALSA20_KEY_SIZE], 
                 const uint8_t nonce[SALSA20_NONCE_SIZE], uint64_t counter) {
    if (!ctx || !key || !nonce) {
        return SALSA20_ERROR_INPUT;
    }
    
    /* Inicializar estado según especificación de Salsa20 */
    /* Layout del estado:
     * 0: constant   1: key       2: key       3: key
     * 4: key        5: constant  6: nonce     7: nonce
     * 8: counter    9: counter   10: constant 11: key
     * 12: key       13: key      14: key      15: constant
     */
    
    ctx->state[0] = SALSA20_CONSTANTS[0];
    ctx->state[5] = SALSA20_CONSTANTS[1];
    ctx->state[10] = SALSA20_CONSTANTS[2];
    ctx->state[15] = SALSA20_CONSTANTS[3];
    
    /* Clave (256 bits = 8 words) */
    ctx->state[1]  = load32_le(key + 0);
    ctx->state[2]  = load32_le(key + 4);
    ctx->state[3]  = load32_le(key + 8);
    ctx->state[4]  = load32_le(key + 12);
    ctx->state[11] = load32_le(key + 16);
    ctx->state[12] = load32_le(key + 20);
    ctx->state[13] = load32_le(key + 24);
    ctx->state[14] = load32_le(key + 28);
    
    /* Nonce (64 bits = 2 words) */
    ctx->state[6] = load32_le(nonce + 0);
    ctx->state[7] = load32_le(nonce + 4);
    
    /* Contador (64 bits = 2 words) */
    ctx->state[8] = (uint32_t)(counter & 0xFFFFFFFF);
    ctx->state[9] = (uint32_t)(counter >> 32);
    
    /* Inicializar posición del keystream */
    ctx->keystream_pos = SALSA20_BLOCK_SIZE;
    ctx->counter = counter;
    
    return SALSA20_SUCCESS;
}

int salsa20_crypt(salsa20_ctx_t *ctx, const uint8_t *input, uint8_t *output, size_t length) {
    if (!ctx || !input || !output) {
        return SALSA20_ERROR_INPUT;
    }
    
    for (size_t i = 0; i < length; i++) {
        /* Generar nuevo bloque de keystream si es necesario */
        if (ctx->keystream_pos >= SALSA20_BLOCK_SIZE) {
            salsa20_block(ctx->state, ctx->keystream);
            ctx->keystream_pos = 0;
            
            /* Incrementar contador de 64 bits */
            ctx->counter++;
            ctx->state[8] = (uint32_t)(ctx->counter & 0xFFFFFFFF);
            ctx->state[9] = (uint32_t)(ctx->counter >> 32);
        }
        
        /* XOR byte de entrada con keystream */
        output[i] = input[i] ^ ctx->keystream[ctx->keystream_pos];
        ctx->keystream_pos++;
    }
    
    return SALSA20_SUCCESS;
}

/**
 * @brief Hash simple para derivación de clave (NO usar en producción)
 */
static void simple_hash(const uint8_t *input, size_t input_len, 
                       uint8_t output[32]) {
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
    
    /* Escribir salida */
    for (int i = 0; i < 8; i++) {
        store32_le(output + (i * 4), state[i]);
    }
}

int salsa20_derive_key(const char *password, size_t password_len, 
                       uint8_t key[SALSA20_KEY_SIZE]) {
    if (!password || password_len == 0 || !key) {
        return SALSA20_ERROR_INPUT;
    }
    
    simple_hash((const uint8_t *)password, password_len, key);
    
    return SALSA20_SUCCESS;
}

int salsa20_generate_nonce(const uint8_t *salt, size_t salt_len, 
                           uint8_t nonce[SALSA20_NONCE_SIZE]) {
    if (!salt || salt_len == 0 || !nonce) {
        return SALSA20_ERROR_INPUT;
    }
    
    uint8_t hash[32];
    simple_hash(salt, salt_len, hash);
    
    /* Tomar primeros 8 bytes */
    memcpy(nonce, hash, SALSA20_NONCE_SIZE);
    
    return SALSA20_SUCCESS;
}

/* ==================== Interfaz de alto nivel compatible con GSEA ==================== */

int salsa20_encrypt(const file_buffer_t *input, file_buffer_t *output,
                    const uint8_t *key, size_t key_len) {
    if (!input || !output || !key || key_len == 0) {
        return GSEA_ERROR_ARGS;
    }

    if (!input->data || input->size == 0) {
        return GSEA_ERROR_ARGS;
    }

    /* Derivar clave de 256 bits desde la clave proporcionada */
    uint8_t derived_key[SALSA20_KEY_SIZE];
    if (salsa20_derive_key((const char *)key, key_len, derived_key) != SALSA20_SUCCESS) {
        return GSEA_ERROR_ENCRYPTION;
    }

    /* Generar nonce determinístico desde la clave (para poder desencriptar) */
    uint8_t nonce[SALSA20_NONCE_SIZE];
    if (salsa20_generate_nonce(key, key_len, nonce) != SALSA20_SUCCESS) {
        return GSEA_ERROR_ENCRYPTION;
    }

    /* Calcular tamaño de salida: nonce (8) + tamaño_original (8) + datos_encriptados */
    size_t output_size = SALSA20_NONCE_SIZE + sizeof(uint64_t) + input->size;
    
    output->data = malloc(output_size);
    if (!output->data) {
        return GSEA_ERROR_MEMORY;
    }
    output->size = output_size;

    /* Escribir nonce al inicio */
    memcpy(output->data, nonce, SALSA20_NONCE_SIZE);
    
    /* Escribir tamaño original */
    uint64_t orig_size = input->size;
    for (int i = 0; i < 8; i++) {
        output->data[SALSA20_NONCE_SIZE + i] = (uint8_t)(orig_size >> (i * 8));
    }

    /* Inicializar contexto de Salsa20 */
    salsa20_ctx_t ctx;
    if (salsa20_init(&ctx, derived_key, nonce, 0) != SALSA20_SUCCESS) {
        free(output->data);
        output->data = NULL;
        output->size = 0;
        return GSEA_ERROR_ENCRYPTION;
    }

    /* Encriptar datos */
    uint8_t *encrypted_start = output->data + SALSA20_NONCE_SIZE + sizeof(uint64_t);
    if (salsa20_crypt(&ctx, input->data, encrypted_start, input->size) != SALSA20_SUCCESS) {
        free(output->data);
        output->data = NULL;
        output->size = 0;
        return GSEA_ERROR_ENCRYPTION;
    }

    /* Limpiar datos sensibles */
    memset(derived_key, 0, SALSA20_KEY_SIZE);
    memset(&ctx, 0, sizeof(ctx));

    return GSEA_SUCCESS;
}

int salsa20_decrypt(const file_buffer_t *input, file_buffer_t *output,
                    const uint8_t *key, size_t key_len) {
    if (!input || !output || !key || key_len == 0) {
        return GSEA_ERROR_ARGS;
    }

    /* Verificar tamaño mínimo: nonce + tamaño_original */
    size_t min_size = SALSA20_NONCE_SIZE + sizeof(uint64_t);
    if (!input->data || input->size < min_size) {
        return GSEA_ERROR_ARGS;
    }

    /* Derivar clave de 256 bits desde la clave proporcionada */
    uint8_t derived_key[SALSA20_KEY_SIZE];
    if (salsa20_derive_key((const char *)key, key_len, derived_key) != SALSA20_SUCCESS) {
        return GSEA_ERROR_ENCRYPTION;
    }

    /* Extraer nonce */
    uint8_t nonce[SALSA20_NONCE_SIZE];
    memcpy(nonce, input->data, SALSA20_NONCE_SIZE);

    /* Extraer tamaño original */
    uint64_t orig_size = 0;
    for (int i = 0; i < 8; i++) {
        orig_size |= ((uint64_t)input->data[SALSA20_NONCE_SIZE + i]) << (i * 8);
    }

    /* Verificar que el tamaño sea coherente */
    size_t encrypted_size = input->size - min_size;
    if (orig_size != encrypted_size) {
        return GSEA_ERROR_ENCRYPTION;
    }

    /* Reservar memoria para salida */
    output->data = malloc(orig_size);
    if (!output->data) {
        return GSEA_ERROR_MEMORY;
    }
    output->size = orig_size;

    /* Inicializar contexto de Salsa20 */
    salsa20_ctx_t ctx;
    if (salsa20_init(&ctx, derived_key, nonce, 0) != SALSA20_SUCCESS) {
        free(output->data);
        output->data = NULL;
        output->size = 0;
        return GSEA_ERROR_ENCRYPTION;
    }

    /* Desencriptar datos */
    const uint8_t *encrypted_start = input->data + min_size;
    if (salsa20_crypt(&ctx, encrypted_start, output->data, orig_size) != SALSA20_SUCCESS) {
        free(output->data);
        output->data = NULL;
        output->size = 0;
        return GSEA_ERROR_ENCRYPTION;
    }

    /* Limpiar datos sensibles */
    memset(derived_key, 0, SALSA20_KEY_SIZE);
    memset(&ctx, 0, sizeof(ctx));

    return GSEA_SUCCESS;
}