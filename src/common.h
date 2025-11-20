/**
 * @file common.h
 * @brief Definiciones comunes y estructuras globales del proyecto GSEA
 * @author Universidad EAFIT - Sistemas Operativos
 */

#ifndef COMMON_H
#define COMMON_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdio.h>  /* Necesario para fprintf */

/* ==============================
 * Códigos de error del sistema
 * ============================== */
#define GSEA_SUCCESS 0
#define GSEA_ERROR_ARGS -1
#define GSEA_ERROR_FILE -2
#define GSEA_ERROR_MEMORY -3
#define GSEA_ERROR_COMPRESSION -4
#define GSEA_ERROR_ENCRYPTION -5
#define GSEA_ERROR_THREAD -6

/* ==============================
 * Constantes del sistema
 * ============================== */
#define MAX_PATH_LENGTH 4096
#define MAX_KEY_LENGTH 256
#define BUFFER_SIZE 8192
#define MAX_THREADS 16

/* ==============================
 * Enumeraciones de operaciones
 * ============================== */
typedef enum {
    OP_NONE = 0,
    OP_COMPRESS = 1 << 0,
    OP_DECOMPRESS = 1 << 1,
    OP_ENCRYPT = 1 << 2,
    OP_DECRYPT = 1 << 3
} operation_t;

/* ==============================
 * Algoritmos disponibles
 * ============================== */
typedef enum {
    COMP_LZ77,
    COMP_HUFFMAN,
    COMP_RLE,
    COMP_LZW        
} compression_algorithm_t;

typedef enum {
    ENC_AES128,
    ENC_CHACHA20,
    ENC_SALSA20,
    ENC_DES,
    ENC_VIGENERE,
    ENC_RC4         
} encryption_algorithm_t;

/* ==============================
 * Estructura de configuración global
 * ============================== */
typedef struct {
    operation_t operations;
    compression_algorithm_t comp_alg;
    encryption_algorithm_t enc_alg;
    char input_path[MAX_PATH_LENGTH];
    char output_path[MAX_PATH_LENGTH];
    char key[MAX_KEY_LENGTH];
    size_t key_len;
    int num_threads;
    bool verbose;
} gsea_config_t;

/* ==============================
 * Estructura para datos de archivo
 * ============================== */
typedef struct {
    uint8_t *data;
    size_t size;
    size_t capacity;
} file_buffer_t;

/* ==============================
 * Estructura para tarea de procesamiento
 * ============================== */
typedef struct {
    char input_path[MAX_PATH_LENGTH];
    char output_path[MAX_PATH_LENGTH];
    gsea_config_t *config;
} process_task_t;

/* ==============================
 * Macros de utilidad
 * ============================== */
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define ALIGN_UP(x, align) (((x) + (align) - 1) & ~((align) - 1))

/* ==============================
 * Macros de logging (100% C99)
 * ============================== */
#ifdef DEBUG
#define LOG_DEBUG(...) \
    do { fprintf(stderr, "[DEBUG] %s:%d: ", __FILE__, __LINE__); fprintf(stderr, __VA_ARGS__); fprintf(stderr, "\n"); } while (0)
#else
#define LOG_DEBUG(...) ((void)0)
#endif

#define LOG_INFO(...)  \
    do { fprintf(stdout, "[INFO] "); fprintf(stdout, __VA_ARGS__); fprintf(stdout, "\n"); } while (0)

#define LOG_ERROR(...) \
    do { fprintf(stderr, "[ERROR] "); fprintf(stderr, __VA_ARGS__); fprintf(stderr, "\n"); } while (0)

#endif /* COMMON_H */
