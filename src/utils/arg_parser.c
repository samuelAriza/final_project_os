/**
 * @file arg_parser.c
 * @brief Parser de argumentos de línea de comandos
 */

#include "arg_parser.h"
#include "../common.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

/**
 * @brief Imprime mensaje de uso del programa
 */
static void print_usage(const char *program_name) {
    printf("Usage: %s [OPTIONS]\n\n", program_name);
    printf("Options:\n");
    printf("  -c                    Compress data\n");
    printf("  -d                    Decompress data\n");
    printf("  -e                    Encrypt data\n");
    printf("  -u                    Decrypt data\n");
    printf("  --comp-alg ALG        Compression algorithm (lz77, huffman, rle)\n");
    printf("  --enc-alg ALG         Encryption algorithm (aes128, des, vigenere)\n");
    printf("  -i PATH               Input file or directory\n");
    printf("  -o PATH               Output file or directory\n");
    printf("  -k KEY                Encryption/Decryption key\n");
    printf("  -t NUM                Number of threads (default: 4)\n");
    printf("  -v                    Verbose output\n");
    printf("  -h, --help            Show this help message\n\n");
    printf("Examples:\n");
    printf("  %s -c --comp-alg lz77 -i input.txt -o output.lz77\n", program_name);
    printf("  %s -ce --comp-alg lz77 --enc-alg aes128 -i data/ -o backup.enc -k secret\n", 
           program_name);
    printf("  %s -du --enc-alg aes128 --comp-alg lz77 -i backup.enc -o restored/ -k secret\n",
           program_name);
}

/**
 * @brief Parsea el algoritmo de compresión
 */
static int parse_compression_algorithm(const char *alg_name, 
                                       compression_algorithm_t *alg) {
    if (strcmp(alg_name, "lz77") == 0) {
        *alg = COMP_LZ77;
        return GSEA_SUCCESS;
    } else if (strcmp(alg_name, "huffman") == 0) {
        *alg = COMP_HUFFMAN;
        LOG_ERROR("Huffman algorithm not yet implemented");
        return GSEA_ERROR_ARGS;
    } else if (strcmp(alg_name, "rle") == 0) {
        *alg = COMP_RLE;
        LOG_ERROR("RLE algorithm not yet implemented");
        return GSEA_ERROR_ARGS;
    } else {
        LOG_ERROR("Unknown compression algorithm: %s", alg_name);
        return GSEA_ERROR_ARGS;
    }
}

/**
 * @brief Parsea el algoritmo de encriptación
 */
static int parse_encryption_algorithm(const char *alg_name,
                                      encryption_algorithm_t *alg) {
    if (strcmp(alg_name, "aes128") == 0 || strcmp(alg_name, "aes") == 0) {
        *alg = ENC_AES128;
        return GSEA_SUCCESS;
    } else if (strcmp(alg_name, "des") == 0) {
        *alg = ENC_DES;
        LOG_ERROR("DES algorithm not yet implemented");
        return GSEA_ERROR_ARGS;
    } else if (strcmp(alg_name, "vigenere") == 0) {
        *alg = ENC_VIGENERE;
        LOG_ERROR("Vigenere algorithm not yet implemented");
        return GSEA_ERROR_ARGS;
    } else {
        LOG_ERROR("Unknown encryption algorithm: %s", alg_name);
        return GSEA_ERROR_ARGS;
    }
}

/**
 * @brief Deriva una clave de 16 bytes desde una clave de texto
 */
static void derive_key(const char *password, uint8_t key[16]) {
    /* Simple derivación de clave usando hash repetido */
    memset(key, 0, 16);
    size_t pass_len = strlen(password);
    
    for (size_t i = 0; i < 16; i++) {
        if (i < pass_len) {
            key[i] = password[i];
        }
        /* Mezclar con XOR simple para extensión de clave */
        key[i] ^= (uint8_t)(i * 17 + 13);
    }
    
    /* Segunda pasada para mayor difusión */
    for (int round = 0; round < 3; round++) {
        for (int i = 0; i < 16; i++) {
            key[i] ^= key[(i + 7) % 16];
            key[i] = (key[i] << 3) | (key[i] >> 5);
        }
    }
}

/**
 * @brief Parsea los argumentos de línea de comandos
 */
int parse_arguments(int argc, char *argv[], gsea_config_t *config) {
    if (!argv || !config || argc < 2) {
        LOG_ERROR("Invalid arguments");
        print_usage(argv[0]);
        return GSEA_ERROR_ARGS;
    }
    
    /* Inicializar configuración con valores por defecto */
    memset(config, 0, sizeof(gsea_config_t));
    config->operations = OP_NONE;
    config->comp_alg = COMP_LZ77;  /* Default */
    config->enc_alg = ENC_AES128;  /* Default */
    config->num_threads = 4;        /* Default */
    config->verbose = false;
    
    /* Parsear argumentos */
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            exit(0);
        }
        else if (strcmp(argv[i], "-v") == 0) {
            config->verbose = true;
        }
        else if (argv[i][0] == '-' && argv[i][1] != '-' && strlen(argv[i]) > 1) {
            /* Parsear operaciones combinadas (ej: -ce) */
            for (size_t j = 1; j < strlen(argv[i]); j++) {
                switch (argv[i][j]) {
                    case 'c':
                        config->operations |= OP_COMPRESS;
                        break;
                    case 'd':
                        config->operations |= OP_DECOMPRESS;
                        break;
                    case 'e':
                        config->operations |= OP_ENCRYPT;
                        break;
                    case 'u':
                        config->operations |= OP_DECRYPT;
                        break;
                    case 'v':
                        config->verbose = true;
                        break;
                    case 'i':
                        /* -i requiere argumento */
                        if (j != strlen(argv[i]) - 1) {
                            LOG_ERROR("-i must be the last flag in combined operations");
                            return GSEA_ERROR_ARGS;
                        }
                        if (i + 1 >= argc) {
                            LOG_ERROR("Missing argument for -i");
                            return GSEA_ERROR_ARGS;
                        }
                        strncpy(config->input_path, argv[++i], MAX_PATH_LENGTH - 1);
                        goto next_arg;
                    case 'o':
                        if (j != strlen(argv[i]) - 1) {
                            LOG_ERROR("-o must be the last flag in combined operations");
                            return GSEA_ERROR_ARGS;
                        }
                        if (i + 1 >= argc) {
                            LOG_ERROR("Missing argument for -o");
                            return GSEA_ERROR_ARGS;
                        }
                        strncpy(config->output_path, argv[++i], MAX_PATH_LENGTH - 1);
                        goto next_arg;
                    case 'k':
                        if (j != strlen(argv[i]) - 1) {
                            LOG_ERROR("-k must be the last flag in combined operations");
                            return GSEA_ERROR_ARGS;
                        }
                        if (i + 1 >= argc) {
                            LOG_ERROR("Missing argument for -k");
                            return GSEA_ERROR_ARGS;
                        }
                        derive_key(argv[++i], (uint8_t *)config->key);
                        config->key_len = 16;
                        goto next_arg;
                    case 't':
                        if (j != strlen(argv[i]) - 1) {
                            LOG_ERROR("-t must be the last flag in combined operations");
                            return GSEA_ERROR_ARGS;
                        }
                        if (i + 1 >= argc) {
                            LOG_ERROR("Missing argument for -t");
                            return GSEA_ERROR_ARGS;
                        }
                        config->num_threads = atoi(argv[++i]);
                        if (config->num_threads <= 0 || config->num_threads > MAX_THREADS) {
                            LOG_ERROR("Invalid thread count: %d", config->num_threads);
                            return GSEA_ERROR_ARGS;
                        }
                        goto next_arg;
                    default:
                        LOG_ERROR("Unknown option: -%c", argv[i][j]);
                        return GSEA_ERROR_ARGS;
                }
            }
        }
        else if (strcmp(argv[i], "--comp-alg") == 0) {
            if (i + 1 >= argc) {
                LOG_ERROR("Missing argument for --comp-alg");
                return GSEA_ERROR_ARGS;
            }
            if (parse_compression_algorithm(argv[++i], &config->comp_alg) != GSEA_SUCCESS) {
                return GSEA_ERROR_ARGS;
            }
        }
        else if (strcmp(argv[i], "--enc-alg") == 0) {
            if (i + 1 >= argc) {
                LOG_ERROR("Missing argument for --enc-alg");
                return GSEA_ERROR_ARGS;
            }
            if (parse_encryption_algorithm(argv[++i], &config->enc_alg) != GSEA_SUCCESS) {
                return GSEA_ERROR_ARGS;
            }
        }
        else {
            LOG_ERROR("Unknown option: %s", argv[i]);
            return GSEA_ERROR_ARGS;
        }
        next_arg:;
    }
    
    /* Validar configuración */
    if (config->operations == OP_NONE) {
        LOG_ERROR("No operation specified. Use -c, -d, -e, or -u");
        return GSEA_ERROR_ARGS;
    }
    
    if (strlen(config->input_path) == 0) {
        LOG_ERROR("Input path (-i) is required");
        return GSEA_ERROR_ARGS;
    }
    
    if (strlen(config->output_path) == 0) {
        LOG_ERROR("Output path (-o) is required");
        return GSEA_ERROR_ARGS;
    }
    
    /* Verificar operaciones incompatibles */
    if ((config->operations & OP_COMPRESS) && (config->operations & OP_DECOMPRESS)) {
        LOG_ERROR("Cannot compress and decompress simultaneously");
        return GSEA_ERROR_ARGS;
    }
    
    if ((config->operations & OP_ENCRYPT) && (config->operations & OP_DECRYPT)) {
        LOG_ERROR("Cannot encrypt and decrypt simultaneously");
        return GSEA_ERROR_ARGS;
    }
    
    /* Verificar que se proporcione clave para encriptación/desencriptación */
    if ((config->operations & (OP_ENCRYPT | OP_DECRYPT)) && config->key_len == 0) {
        LOG_ERROR("Encryption key (-k) is required for encryption/decryption operations");
        return GSEA_ERROR_ARGS;
    }
    
    return GSEA_SUCCESS;
}