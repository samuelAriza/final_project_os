/**
 * @file file_manager.c
 * @brief Gestor de archivos usando llamadas directas al sistema operativo
 * @details Usa open, read, write, close (POSIX) sin abstracciones de stdio.h
 */

#include "file_manager.h"
#include "common.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>

/**
 * @brief Lee un archivo completo en memoria usando syscalls
 */
int read_file(const char *path, file_buffer_t *buffer) {
    if (!path || !buffer) {
        LOG_ERROR("Invalid parameters for read_file");
        return GSEA_ERROR_ARGS;
    }
    
    /* Abrir archivo con syscall open */
    int fd = open(path, O_RDONLY);
    if (fd < 0) {
        LOG_ERROR("Failed to open file '%s': %s", path, strerror(errno));
        return GSEA_ERROR_FILE;
    }
    
    /* Obtener tamaño del archivo */
    struct stat st;
    if (fstat(fd, &st) < 0) {
        LOG_ERROR("Failed to stat file '%s': %s", path, strerror(errno));
        close(fd);
        return GSEA_ERROR_FILE;
    }
    
    size_t file_size = st.st_size;
    
    /* Asignar buffer */
    buffer->capacity = file_size + 1;
    buffer->data = malloc(buffer->capacity);
    buffer->size = 0;
    
    if (!buffer->data) {
        LOG_ERROR("Memory allocation failed for file buffer");
        close(fd);
        return GSEA_ERROR_MEMORY;
    }
    
    /* Leer archivo en bloques usando syscall read */
    ssize_t bytes_read;
    while ((bytes_read = read(fd, buffer->data + buffer->size, 
                              BUFFER_SIZE)) > 0) {
        buffer->size += bytes_read;
        
        /* Verificar si necesitamos más espacio */
        if (buffer->size + BUFFER_SIZE > buffer->capacity) {
            buffer->capacity *= 2;
            uint8_t *new_data = realloc(buffer->data, buffer->capacity);
            if (!new_data) {
                LOG_ERROR("Memory reallocation failed");
                free(buffer->data);
                close(fd);
                return GSEA_ERROR_MEMORY;
            }
            buffer->data = new_data;
        }
    }
    
    if (bytes_read < 0) {
        LOG_ERROR("Error reading file '%s': %s", path, strerror(errno));
        free(buffer->data);
        close(fd);
        return GSEA_ERROR_FILE;
    }
    
    /* Cerrar archivo con syscall close */
    close(fd);
    
    LOG_DEBUG("Read %zu bytes from '%s'", buffer->size, path);
    return GSEA_SUCCESS;
}

/**
 * @brief Escribe un buffer en un archivo usando syscalls
 */
int write_file(const char *path, const file_buffer_t *buffer) {
    if (!path || !buffer || !buffer->data) {
        LOG_ERROR("Invalid parameters for write_file");
        return GSEA_ERROR_ARGS;
    }
    
    /* Crear/abrir archivo con syscall open */
    int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) {
        LOG_ERROR("Failed to create file '%s': %s", path, strerror(errno));
        return GSEA_ERROR_FILE;
    }
    
    /* Escribir datos en bloques usando syscall write */
    size_t bytes_written = 0;
    while (bytes_written < buffer->size) {
        ssize_t result = write(fd, buffer->data + bytes_written,
                              buffer->size - bytes_written);
        if (result < 0) {
            LOG_ERROR("Error writing to file '%s': %s", path, strerror(errno));
            close(fd);
            return GSEA_ERROR_FILE;
        }
        bytes_written += result;
    }
    
    /* Sincronizar datos al disco */
    if (fsync(fd) < 0) {
        LOG_ERROR("Failed to sync file '%s': %s", path, strerror(errno));
    }
    
    /* Cerrar archivo con syscall close */
    close(fd);
    
    LOG_DEBUG("Wrote %zu bytes to '%s'", buffer->size, path);
    return GSEA_SUCCESS;
}

/**
 * @brief Libera un buffer de archivo
 */
void free_buffer(file_buffer_t *buffer) {
    if (buffer && buffer->data) {
        free(buffer->data);
        buffer->data = NULL;
        buffer->size = 0;
        buffer->capacity = 0;
    }
}

/**
 * @brief Verifica si una ruta es un directorio
 */
bool is_directory(const char *path) {
    struct stat st;
    if (stat(path, &st) < 0) {
        return false;
    }
    return S_ISDIR(st.st_mode);
}

/**
 * @brief Verifica si una ruta es un archivo regular
 */
bool is_regular_file(const char *path) {
    struct stat st;
    if (stat(path, &st) < 0) {
        return false;
    }
    return S_ISREG(st.st_mode);
}

/**
 * @brief Crea un directorio si no existe
 */
int create_directory(const char *path) {
    if (!path) {
        LOG_ERROR("Invalid path for create_directory");
        return GSEA_ERROR_ARGS;
    }
    
    struct stat st;
    if (stat(path, &st) == 0) {
        /* El directorio ya existe */
        return GSEA_SUCCESS;
    }
    
    /* Crear directorio con permisos 0755 */
    if (mkdir(path, 0755) < 0) {
        LOG_ERROR("Failed to create directory '%s': %s", path, strerror(errno));
        return GSEA_ERROR_FILE;
    }
    
    LOG_DEBUG("Created directory '%s'", path);
    return GSEA_SUCCESS;
}

/**
 * @brief Lista archivos en un directorio
 */
int list_directory(const char *path, char ***files, int *count) {
    if (!path || !files || !count) {
        LOG_ERROR("Invalid parameters for list_directory");
        return GSEA_ERROR_ARGS;
    }
    
    /* Abrir directorio con syscall opendir */
    DIR *dir = opendir(path);
    if (!dir) {
        LOG_ERROR("Failed to open directory '%s': %s", path, strerror(errno));
        return GSEA_ERROR_FILE;
    }
    
    /* Contar archivos primero */
    *count = 0;
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        /* Ignorar . y .. */
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        
        /* Construir ruta completa */
        char full_path[MAX_PATH_LENGTH];
        snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);
        
        /* Solo contar archivos regulares */
        if (is_regular_file(full_path)) {
            (*count)++;
        }
    }
    
    /* Asignar array de strings */
    *files = malloc(sizeof(char *) * (*count));
    if (!*files) {
        LOG_ERROR("Memory allocation failed for file list");
        closedir(dir);
        return GSEA_ERROR_MEMORY;
    }
    
    /* Reiniciar lectura del directorio */
    rewinddir(dir);
    
    /* Llenar array con nombres de archivos */
    int i = 0;
    while ((entry = readdir(dir)) != NULL && i < *count) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        
        char full_path[MAX_PATH_LENGTH];
        snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);
        
        if (is_regular_file(full_path)) {
            (*files)[i] = malloc(MAX_PATH_LENGTH);
            if (!(*files)[i]) {
                LOG_ERROR("Memory allocation failed for filename");
                /* Liberar memoria asignada previamente */
                for (int j = 0; j < i; j++) {
                    free((*files)[j]);
                }
                free(*files);
                closedir(dir);
                return GSEA_ERROR_MEMORY;
            }
            size_t len = strlen(full_path);
            size_t copy_len = (len < MAX_PATH_LENGTH - 1) ? len : MAX_PATH_LENGTH - 1;
            memcpy((*files)[i], full_path, copy_len);
            (*files)[i][copy_len] = '\0';
            i++;
        }
    }
    
    /* Cerrar directorio con syscall closedir */
    closedir(dir);
    
    LOG_DEBUG("Found %d files in directory '%s'", *count, path);
    return GSEA_SUCCESS;
}

/**
 * @brief Libera la lista de archivos retornada por list_directory
 */
void free_file_list(char **files, int count) {
    if (files) {
        for (int i = 0; i < count; i++) {
            if (files[i]) {
                free(files[i]);
            }
        }
        free(files);
    }
}