/**
 * @file main.c
 * @brief Punto de entrada principal de GSEA
 * @author Universidad EAFIT - Sistemas Operativos
 */

#include "common.h"
#include "file_manager.h"
#include "compression/compression.h"
#include "encryption/aes.h"
#include "concurrency/thread_pool.h"
#include "utils/arg_parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>

/* Estructura para argumentos de tarea de procesamiento */
typedef struct {
    char input_path[MAX_PATH_LENGTH];
    char output_path[MAX_PATH_LENGTH];
    gsea_config_t *config;
    int *error_count;
    pthread_mutex_t *error_mutex;
} process_args_t;

/**
 * @brief Procesa un archivo individual: compresión/descompresión y/o encriptación/desencriptación
 */
static int process_file_operations(const char *input_path, const char *output_path,
                                    const gsea_config_t *config) {
    int result = GSEA_SUCCESS;
    file_buffer_t input = {0};
    file_buffer_t intermediate = {0};
    file_buffer_t output = {0};
    
    if (config->verbose) {
        LOG_INFO("Processing: %s -> %s", input_path, output_path);
    }
    
    /* Leer archivo de entrada */
    result = read_file(input_path, &input);
    if (result != GSEA_SUCCESS) {
        LOG_ERROR("Failed to read input file: %s", input_path);
        return result;
    }
    
    /* Determinar orden de operaciones */
    /* Para reversibilidad: si comprimimos luego encriptamos (compress->encrypt),
       debemos desencriptar luego descomprimir (decrypt->decompress) */
    bool compress_first = (config->operations & OP_COMPRESS) != 0;
    bool encrypt_first = !compress_first && (config->operations & OP_ENCRYPT);
    bool decrypt_first = (config->operations & OP_DECRYPT) != 0;
    bool decompress_first = !decrypt_first && (config->operations & OP_DECOMPRESS);
    
    file_buffer_t *current_input = &input;
    file_buffer_t *current_output = &intermediate;
    
    /* Primera operación */
    if (compress_first) {
        if (config->verbose) LOG_INFO("  [1/2] Compressing with %s...",
                                      config->comp_alg == COMP_LZ77 ? "LZ77" : 
                                      config->comp_alg == COMP_HUFFMAN ? "Huffman" : "Unknown");
        result = compress_data(current_input, current_output, config->comp_alg);
        if (result != GSEA_SUCCESS) {
            LOG_ERROR("Compression failed");
            goto cleanup;
        }
        current_input = &intermediate;
        current_output = &output;
    } else if (decompress_first) {
        if (config->verbose) LOG_INFO("  [1/2] Decompressing with %s...",
                                      config->comp_alg == COMP_LZ77 ? "LZ77" : 
                                      config->comp_alg == COMP_HUFFMAN ? "Huffman" : "Unknown");
        result = decompress_data(current_input, current_output, config->comp_alg);
        if (result != GSEA_SUCCESS) {
            LOG_ERROR("Decompression failed");
            goto cleanup;
        }
        current_input = &intermediate;
        current_output = &output;
    } else if (encrypt_first) {
        if (config->verbose) LOG_INFO("  [1/1] Encrypting...");
        result = aes_encrypt(current_input, &output, (uint8_t *)config->key, config->key_len);
        if (result != GSEA_SUCCESS) {
            LOG_ERROR("Encryption failed");
            goto cleanup;
        }
        current_input = &output;
    } else if (decrypt_first) {
        if (config->verbose) LOG_INFO("  [1/1] Decrypting...");
        result = aes_decrypt(current_input, &output, (uint8_t *)config->key, config->key_len);
        if (result != GSEA_SUCCESS) {
            LOG_ERROR("Decryption failed");
            goto cleanup;
        }
        current_input = &output;
    }
    
    /* Segunda operación (si existe) */
    if ((compress_first || decrypt_first) && 
        (config->operations & (OP_ENCRYPT | OP_DECOMPRESS))) {
        
        if (config->operations & OP_ENCRYPT) {
            if (config->verbose) LOG_INFO("  [2/2] Encrypting...");
            result = aes_encrypt(current_input, current_output, 
                               (uint8_t *)config->key, config->key_len);
        } else {
            if (config->verbose) LOG_INFO("  [2/2] Decompressing with %s...",
                                          config->comp_alg == COMP_LZ77 ? "LZ77" : 
                                          config->comp_alg == COMP_HUFFMAN ? "Huffman" : "Unknown");
            result = decompress_data(current_input, current_output, config->comp_alg);
        }
        
        if (result != GSEA_SUCCESS) {
            LOG_ERROR("Second operation failed");
            goto cleanup;
        }
        current_input = current_output;
    }
    
    /* Escribir archivo de salida */
    result = write_file(output_path, current_input);
    if (result != GSEA_SUCCESS) {
        LOG_ERROR("Failed to write output file: %s", output_path);
        goto cleanup;
    }
    
    if (config->verbose) {
        LOG_INFO("  Completed: %zu bytes -> %zu bytes", 
                 input.size, current_input->size);
    }
    
cleanup:
    free_buffer(&input);
    free_buffer(&intermediate);
    free_buffer(&output);
    
    return result;
}

/**
 * @brief Función de worker para procesamiento concurrente
 */
static void process_file_worker(void *arg) {
    process_args_t *args = (process_args_t *)arg;
    
    int result = process_file_operations(args->input_path, args->output_path, 
                                         args->config);
    
    if (result != GSEA_SUCCESS) {
        pthread_mutex_lock(args->error_mutex);
        (*args->error_count)++;
        pthread_mutex_unlock(args->error_mutex);
    }
    
    free(args);
}

/**
 * @brief Procesa un directorio de archivos usando el pool de hilos
 */
static int process_directory(const gsea_config_t *config) {
    int result = GSEA_SUCCESS;
    char **files = NULL;
    int file_count = 0;
    thread_pool_t *pool = NULL;
    pthread_mutex_t error_mutex = PTHREAD_MUTEX_INITIALIZER;
    int error_count = 0;
    
    /* Listar archivos en el directorio de entrada */
    result = list_directory(config->input_path, &files, &file_count);
    if (result != GSEA_SUCCESS) {
        LOG_ERROR("Failed to list directory: %s", config->input_path);
        return result;
    }
    
    if (file_count == 0) {
        LOG_INFO("No files found in directory: %s", config->input_path);
        free_file_list(files, file_count);
        return GSEA_SUCCESS;
    }
    
    LOG_INFO("Found %d files to process", file_count);
    
    /* Crear directorio de salida si no existe */
    if (!is_directory(config->output_path)) {
        result = create_directory(config->output_path);
        if (result != GSEA_SUCCESS) {
            LOG_ERROR("Failed to create output directory");
            free_file_list(files, file_count);
            return result;
        }
    }
    
    /* Crear pool de hilos */
    int num_threads = (file_count < config->num_threads) ? file_count : config->num_threads;
    pool = thread_pool_create(num_threads);
    if (!pool) {
        LOG_ERROR("Failed to create thread pool");
        free_file_list(files, file_count);
        return GSEA_ERROR_THREAD;
    }
    
    /* Encolar tareas para cada archivo */
    for (int i = 0; i < file_count; i++) {
        process_args_t *args = malloc(sizeof(process_args_t));
        if (!args) {
            LOG_ERROR("Memory allocation failed for process args");
            error_count++;
            continue;
        }

        /* Construir ruta de salida */
        const char *filename = strrchr(files[i], '/');
        filename = filename ? filename + 1 : files[i];

        strncpy(args->input_path, files[i], MAX_PATH_LENGTH - 1);
        args->input_path[MAX_PATH_LENGTH - 1] = '\0';

        int written = snprintf(args->output_path, MAX_PATH_LENGTH, "%s/%s",
                            config->output_path, filename);

        if (written < 0 || written >= (int)MAX_PATH_LENGTH) {
            LOG_ERROR("Output path too long, truncated: %s/%s",
                    config->output_path, filename);
            args->output_path[MAX_PATH_LENGTH - 1] = '\0';
        }

        args->config = (gsea_config_t *)config;
        args->error_count = &error_count;
        args->error_mutex = &error_mutex;

        if (thread_pool_add_task(pool, process_file_worker, args) != GSEA_SUCCESS) {
            LOG_ERROR("Failed to add task to thread pool");
            free(args);
            error_count++;
        }
    }

    
    /* Esperar a que terminen todas las tareas */
    thread_pool_wait(pool);
    
    /* Limpiar */
    thread_pool_destroy(pool);
    free_file_list(files, file_count);
    pthread_mutex_destroy(&error_mutex);
    
    if (error_count > 0) {
        LOG_ERROR("Processing completed with %d errors", error_count);
        return GSEA_ERROR_FILE;
    }
    
    LOG_INFO("All files processed successfully");
    return GSEA_SUCCESS;
}

/**
 * @brief Función principal
 */
int main(int argc, char *argv[]) {
    gsea_config_t config;
    int result;
    clock_t start_time, end_time;
    
    printf("=================================================\n");
    printf("  GSEA - Gestión Segura y Eficiente de Archivos\n");
    printf("  Universidad EAFIT - Sistemas Operativos\n");
    printf("=================================================\n\n");
    
    /* Parsear argumentos */
    result = parse_arguments(argc, argv, &config);
    if (result != GSEA_SUCCESS) {
        return EXIT_FAILURE;
    }
    
    /* Mostrar configuración */
    if (config.verbose) {
        LOG_INFO("Configuration:");
        LOG_INFO("  Input: %s", config.input_path);
        LOG_INFO("  Output: %s", config.output_path);
        LOG_INFO("  Operations: %s%s%s%s",
                 (config.operations & OP_COMPRESS) ? "COMPRESS " : "",
                 (config.operations & OP_DECOMPRESS) ? "DECOMPRESS " : "",
                 (config.operations & OP_ENCRYPT) ? "ENCRYPT " : "",
                 (config.operations & OP_DECRYPT) ? "DECRYPT " : "");
        LOG_INFO("  Threads: %d", config.num_threads);
    }
    
    /* Iniciar temporizador */
    start_time = clock();
    
    /* Procesar entrada */
    if (is_directory(config.input_path)) {
        result = process_directory(&config);
    } else if (is_regular_file(config.input_path)) {
        result = process_file_operations(config.input_path, config.output_path, &config);
    } else {
        LOG_ERROR("Input path does not exist or is not accessible: %s", 
                  config.input_path);
        return EXIT_FAILURE;
    }
    
    /* Calcular tiempo transcurrido */
    end_time = clock();
    double elapsed = ((double)(end_time - start_time)) / CLOCKS_PER_SEC;
    
    /* Mostrar resultados */
    if (result == GSEA_SUCCESS) {
        printf("\n=================================================\n");
        printf("  Operation completed successfully!\n");
        printf("  Time elapsed: %.3f seconds\n", elapsed);
        printf("=================================================\n");
        return EXIT_SUCCESS;
    } else {
        printf("\n=================================================\n");
        printf("  Operation failed with error code: %d\n", result);
        printf("  Time elapsed: %.3f seconds\n", elapsed);
        printf("=================================================\n");
        return EXIT_FAILURE;
    }
}