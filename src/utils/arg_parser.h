/**
 * @file arg_parser.h
 * @brief Interfaz del parser de argumentos de línea de comandos
 */

#ifndef ARG_PARSER_H
#define ARG_PARSER_H

#include "../common.h"

/**
 * @brief Parsea los argumentos de línea de comandos
 * @param argc Número de argumentos
 * @param argv Array de argumentos
 * @param config Estructura de configuración a llenar
 * @return GSEA_SUCCESS si fue exitoso, código de error en caso contrario
 */
int parse_arguments(int argc, char *argv[], gsea_config_t *config);

#endif /* ARG_PARSER_H */