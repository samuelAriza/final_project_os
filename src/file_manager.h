/**
 * @file file_manager.h
 * @brief Interfaz del gestor de archivos con llamadas al sistema
 */

#ifndef FILE_MANAGER_H
#define FILE_MANAGER_H

#include "common.h"

/**
 * @brief Lee un archivo completo en un buffer
 * @param path Ruta del archivo a leer
 * @param buffer Buffer donde se almacenarán los datos
 * @return GSEA_SUCCESS si fue exitoso, código de error en caso contrario
 */
int read_file(const char *path, file_buffer_t *buffer);

/**
 * @brief Escribe un buffer en un archivo
 * @param path Ruta del archivo de destino
 * @param buffer Buffer con los datos a escribir
 * @return GSEA_SUCCESS si fue exitoso, código de error en caso contrario
 */
int write_file(const char *path, const file_buffer_t *buffer);

/**
 * @brief Libera la memoria de un buffer de archivo
 * @param buffer Buffer a liberar
 */
void free_buffer(file_buffer_t *buffer);

/**
 * @brief Verifica si una ruta es un directorio
 * @param path Ruta a verificar
 * @return true si es un directorio, false en caso contrario
 */
bool is_directory(const char *path);

/**
 * @brief Verifica si una ruta es un archivo regular
 * @param path Ruta a verificar
 * @return true si es un archivo regular, false en caso contrario
 */
bool is_regular_file(const char *path);

/**
 * @brief Crea un directorio si no existe
 * @param path Ruta del directorio a crear
 * @return GSEA_SUCCESS si fue exitoso, código de error en caso contrario
 */
int create_directory(const char *path);

/**
 * @brief Lista todos los archivos en un directorio
 * @param path Ruta del directorio
 * @param files [out] Array de strings con rutas de archivos
 * @param count [out] Número de archivos encontrados
 * @return GSEA_SUCCESS si fue exitoso, código de error en caso contrario
 */
int list_directory(const char *path, char ***files, int *count);

/**
 * @brief Libera la lista de archivos retornada por list_directory
 * @param files Array de strings a liberar
 * @param count Número de elementos en el array
 */
void free_file_list(char **files, int count);

#endif /* FILE_MANAGER_H */