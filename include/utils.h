/**
 * @file utils.h
 * @author Joaquín Jiménez López de Castro (joaquin.jimenezl@estudiante.uam.es)
 * @brief Diversas macros y funciones miscelaneas
 * @version 1.0
 * @date 17-03-2020
 * 
 * 
 */

#ifndef UTILS_H
#define UTILS_H

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <strings.h>
#include <libgen.h>
#include <stdarg.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <sys/stat.h> 
#include <sys/param.h>
#include <semaphore.h>
#include <syslog.h>
#include <fcntl.h>
#include <pthread.h>
#include <errno.h>
#include <ctype.h>
#include <shadow.h>
#include <pwd.h>
#include <crypt.h>
#include <grp.h>
#include <termios.h>

/* Tamaños */
#define TINY_SZ 8       /*!< Tamaño enano */
#define SMALL_SZ 64     /*!< Tamaño pequeño */
#define MEDIUM_SZ 256   /*!< Tamaño mediano */
#define XL_SZ 1024      /*!< Tamaño grande */
#define XXL_SZ 4096     /*!< Tamaño enorme */
#define XXXL_SZ 8192    /*!< Tamaño gigante */

/**
 * @brief Tamaño de un fichero dado su descriptor
 * 
 * @param fd descriptor del fichero
 * @return size_t tamaño
 */
size_t file_size(int fd);

/* SEMAFOROS Y MEMORIA COMPARTIDA */

/**
 * @brief Cierra un conjunto de semaforos
 * 
 * @param n_sem Numero de semaforos a cerrar
 * @param ... Sucesion de semaforos y sus respectivos nombres. Poner NULL en cualquiera de ellos si se quiere evitar close/unlink
 * @return int 
 */
int close_semaphores(int n_sem, ...);

/**
 * @brief Cierra un conjunto de semaforos
 * 
 * @param n_shm Numero de fichero de memoria compartida a cerrar
 * @param ... Sucesion de tripletos estructura, tamaño de estructura y nombre
 * @return int 
 */
int close_shm(int n_shm, ...);

/**
 * @brief Crea un fichero shm y mapea en el buffer mapped
 * 
 * @param name Nombre del ficherp
 * @param size Tamaño del mapeo
 * @param mapped Buffer donde colocar la estructura
 * @return int descriptor de fichero del mapeo
 */
int create_shm(char *name, size_t size, void **mapped);

/**
 * @brief Crea un semaforo
 * 
 * @param name Nombre del semaforo
 * @param initial_val Valor inicial del semaforo
 * @return sem_t* (NULL si fallo)
 */
sem_t *create_sem(char *name, int initial_val);

/**
 * @brief Abre un fichero de memoria compartida
 * 
 * @param name Nombre del fichero
 * @param size Tamaño del fichero
 * @param mapped Buffer
 * @return int 
 */
int open_shm(char *name, size_t size, void **mapped);

/**
 * @brief Abre un semaforo
 * 
 * @param name Nombre del semaforo
 * @return sem_t* (NULL si fallo)
 */
sem_t *open_sem(char *name);

/* LOGGING Y SALIDA ESTANDAR */

/**
 * @brief Indica la configuracion a utilizar en el logueo de mensajes
 * 
 * @param use_std Los mensajes se imprimen en salida estandar
 * @param use_syslog Los mensajes se imprimen en el log del sistema 
 * @param syslog_ident Especificar identificador en el log del sistema
 */
void set_log_conf(int use_std, int use_syslog, char *syslog_ident);

/**
 * @brief Loguea usando los argumentos
 * 
 * @param priority Prioridad en syslog si se va a usar syslog
 * @param format Cadena formateada
 * @param param Lista de argumentos
 */
void vflog(int priority, char *format, va_list param);

/**
 * @brief Loguea usando los argumentos
 * 
 * @param priority Prioridad en syslog si se va a usar syslog
 * @param format Cadena formateada
 * @param param Numero variable de argumentos
 */
void flog(int priority, char *format, ...);

/**
 * @brief Imprime la cadena formateada de argumentos variables y cierra el proceso con exit(1)
 * 
 * @param format Cadena de caracteres formateada
 * @param ... Parametros
 */
void errexit(char *formato, ...);

#endif /* UTILS_H */