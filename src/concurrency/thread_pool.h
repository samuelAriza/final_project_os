/**
 * @file thread_pool.h
 * @brief Interfaz del pool de hilos para procesamiento concurrente
 */

#ifndef THREAD_POOL_H
#define THREAD_POOL_H

/* Tipo opaco para el pool de hilos */
typedef struct thread_pool thread_pool_t;

/**
 * @brief Crea un nuevo pool de hilos
 * @param num_threads Número de hilos en el pool
 * @return Puntero al pool de hilos o NULL en caso de error
 */
thread_pool_t *thread_pool_create(int num_threads);

/**
 * @brief Añade una tarea al pool para ejecución asíncrona
 * @param pool Pool de hilos
 * @param function Función a ejecutar
 * @param arg Argumento para la función
 * @return 0 si fue exitoso, código de error en caso contrario
 */
int thread_pool_add_task(thread_pool_t *pool, void (*function)(void *), void *arg);

/**
 * @brief Espera a que todas las tareas pendientes se completen
 * @param pool Pool de hilos
 */
void thread_pool_wait(thread_pool_t *pool);

/**
 * @brief Destruye el pool de hilos y libera recursos
 * @param pool Pool de hilos a destruir
 */
void thread_pool_destroy(thread_pool_t *pool);

#endif /* THREAD_POOL_H */