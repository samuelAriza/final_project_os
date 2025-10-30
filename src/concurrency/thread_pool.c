/**
 * @file thread_pool.c
 * @brief Implementación de un pool de hilos para procesamiento paralelo
 * @details Usa POSIX threads (pthreads) para procesamiento concurrente
 */

#include "thread_pool.h"
#include "../common.h"
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* Estructura de nodo para la cola de tareas */
typedef struct task_node {
    void (*function)(void *);
    void *arg;
    struct task_node *next;
} task_node_t;

/* Estructura de cola de tareas */
typedef struct {
    task_node_t *head;
    task_node_t *tail;
    int count;
} task_queue_t;

/* Estructura del pool de hilos */
struct thread_pool {
    pthread_t *threads;
    int thread_count;
    task_queue_t queue;
    pthread_mutex_t queue_mutex;
    pthread_cond_t queue_cond;
    pthread_cond_t idle_cond;
    int active_threads;
    bool shutdown;
};

/**
 * @brief Función worker que ejecutan los hilos del pool
 */
static void *worker_thread(void *arg) {
    thread_pool_t *pool = (thread_pool_t *)arg;
    
    while (1) {
        pthread_mutex_lock(&pool->queue_mutex);
        
        /* Esperar a que haya tareas o se indique apagado */
        while (pool->queue.count == 0 && !pool->shutdown) {
            pthread_cond_wait(&pool->queue_cond, &pool->queue_mutex);
        }
        
        if (pool->shutdown && pool->queue.count == 0) {
            pthread_mutex_unlock(&pool->queue_mutex);
            break;
        }
        
        /* Obtener tarea de la cola */
        task_node_t *task = pool->queue.head;
        if (task) {
            pool->queue.head = task->next;
            if (pool->queue.head == NULL) {
                pool->queue.tail = NULL;
            }
            pool->queue.count--;
            pool->active_threads++;
        }
        
        pthread_mutex_unlock(&pool->queue_mutex);
        
        /* Ejecutar tarea */
        if (task) {
            task->function(task->arg);
            free(task);
            
            /* Decrementar contador de hilos activos */
            pthread_mutex_lock(&pool->queue_mutex);
            pool->active_threads--;
            
            /* Señalar si no hay tareas pendientes */
            if (pool->active_threads == 0 && pool->queue.count == 0) {
                pthread_cond_signal(&pool->idle_cond);
            }
            pthread_mutex_unlock(&pool->queue_mutex);
        }
    }
    
    return NULL;
}

/**
 * @brief Crea un nuevo pool de hilos
 */
thread_pool_t *thread_pool_create(int num_threads) {
    if (num_threads <= 0 || num_threads > MAX_THREADS) {
        LOG_ERROR("Invalid thread count: %d", num_threads);
        return NULL;
    }
    
    thread_pool_t *pool = malloc(sizeof(thread_pool_t));
    if (!pool) {
        LOG_ERROR("Failed to allocate thread pool");
        return NULL;
    }
    
    /* Inicializar estructura */
    memset(pool, 0, sizeof(thread_pool_t));
    pool->thread_count = num_threads;
    pool->shutdown = false;
    
    /* Inicializar mutex y variables de condición */
    if (pthread_mutex_init(&pool->queue_mutex, NULL) != 0) {
        LOG_ERROR("Failed to initialize queue mutex");
        free(pool);
        return NULL;
    }
    
    if (pthread_cond_init(&pool->queue_cond, NULL) != 0) {
        LOG_ERROR("Failed to initialize queue condition variable");
        pthread_mutex_destroy(&pool->queue_mutex);
        free(pool);
        return NULL;
    }
    
    if (pthread_cond_init(&pool->idle_cond, NULL) != 0) {
        LOG_ERROR("Failed to initialize idle condition variable");
        pthread_cond_destroy(&pool->queue_cond);
        pthread_mutex_destroy(&pool->queue_mutex);
        free(pool);
        return NULL;
    }
    
    /* Crear hilos */
    pool->threads = malloc(sizeof(pthread_t) * num_threads);
    if (!pool->threads) {
        LOG_ERROR("Failed to allocate thread array");
        pthread_cond_destroy(&pool->idle_cond);
        pthread_cond_destroy(&pool->queue_cond);
        pthread_mutex_destroy(&pool->queue_mutex);
        free(pool);
        return NULL;
    }
    
    for (int i = 0; i < num_threads; i++) {
        if (pthread_create(&pool->threads[i], NULL, worker_thread, pool) != 0) {
            LOG_ERROR("Failed to create thread %d", i);
            pool->thread_count = i;
            thread_pool_destroy(pool);
            return NULL;
        }
    }
    
    LOG_INFO("Thread pool created with %d threads", num_threads);
    return pool;
}

/**
 * @brief Añade una tarea al pool de hilos
 */
int thread_pool_add_task(thread_pool_t *pool, void (*function)(void *), void *arg) {
    if (!pool || !function) {
        LOG_ERROR("Invalid parameters for thread_pool_add_task");
        return GSEA_ERROR_ARGS;
    }
    
    /* Crear nodo de tarea */
    task_node_t *task = malloc(sizeof(task_node_t));
    if (!task) {
        LOG_ERROR("Failed to allocate task node");
        return GSEA_ERROR_MEMORY;
    }
    
    task->function = function;
    task->arg = arg;
    task->next = NULL;
    
    /* Añadir tarea a la cola */
    pthread_mutex_lock(&pool->queue_mutex);
    
    if (pool->shutdown) {
        pthread_mutex_unlock(&pool->queue_mutex);
        free(task);
        LOG_ERROR("Cannot add task to shutdown pool");
        return GSEA_ERROR_THREAD;
    }
    
    if (pool->queue.tail) {
        pool->queue.tail->next = task;
    } else {
        pool->queue.head = task;
    }
    pool->queue.tail = task;
    pool->queue.count++;
    
    /* Señalar a un hilo que hay trabajo */
    pthread_cond_signal(&pool->queue_cond);
    pthread_mutex_unlock(&pool->queue_mutex);
    
    return GSEA_SUCCESS;
}

/**
 * @brief Espera a que todas las tareas se completen
 */
void thread_pool_wait(thread_pool_t *pool) {
    if (!pool) return;
    
    pthread_mutex_lock(&pool->queue_mutex);
    
    /* Esperar mientras haya tareas pendientes o hilos activos */
    while (pool->queue.count > 0 || pool->active_threads > 0) {
        pthread_cond_wait(&pool->idle_cond, &pool->queue_mutex);
    }
    
    pthread_mutex_unlock(&pool->queue_mutex);
    
    LOG_INFO("All tasks completed");
}

/**
 * @brief Destruye el pool de hilos
 */
void thread_pool_destroy(thread_pool_t *pool) {
    if (!pool) return;
    
    LOG_INFO("Destroying thread pool");
    
    /* Señalar apagado */
    pthread_mutex_lock(&pool->queue_mutex);
    pool->shutdown = true;
    pthread_cond_broadcast(&pool->queue_cond);
    pthread_mutex_unlock(&pool->queue_mutex);
    
    /* Esperar a que terminen todos los hilos */
    for (int i = 0; i < pool->thread_count; i++) {
        pthread_join(pool->threads[i], NULL);
    }
    
    /* Liberar tareas pendientes */
    task_node_t *current = pool->queue.head;
    while (current) {
        task_node_t *next = current->next;
        free(current);
        current = next;
    }
    
    /* Destruir sincronización */
    pthread_cond_destroy(&pool->idle_cond);
    pthread_cond_destroy(&pool->queue_cond);
    pthread_mutex_destroy(&pool->queue_mutex);
    
    /* Liberar memoria */
    free(pool->threads);
    free(pool);
    
    LOG_INFO("Thread pool destroyed");
}