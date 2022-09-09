/**
 * @file utils.h
 * @author Joaquin Jimenez Lopez Castro (joaquin.jimenezl@student.uam.es)
 * @brief Miscellaneous macros and functions
 * @version 1.0
 * @date 17-03-2
 *
 *
 */

#ifndef UTILS_H
#define UTILS_H

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include <ctype.h>
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

/*sizes*/
#define TINY_SZ 8     /*!< dwarf size*/
#define SMALL_SZ 64   /*!< small size*/
#define MEDIUM_SZ 256 /*!< medium size*/
#define XL_SZ 1024    /*!< large size*/
#define XXL_SZ 4096   /*!< huge size*/
#define XXXL_SZ 65536 /*!< giant size*/

#define MUTEX_DO(mutex, atomic_op)    \
    {                                 \
        sem_wait(&(mutex));           \
        atomic_op sem_post(&(mutex)); \
    } /*!< Protects an atomic operation with mutex*/
/**
 * @brief Size of a file given its descriptor
 *
 * @param fd file descriptor
 * @return size_t size
 */
size_t file_size(int fd);

/**
 * @brief Indicates the operating system in use
 *
 * @return char*
 */
char *operating_system();

/**
 * @brief Size of a file given its name
 *
 * @param path Name of the file
 * @return size_t size
 */
size_t name_file_size(char *path);

/**
 * @brief Str is an integer
 *
 * @param str String
 * @param len Size to parse, if you want to call strlen, put less than 0
 * @return int
 */
int is_number(char *str, int len);

/*SEMAPHORES AND SHARED MEMORY*/
/**
 * @brief Closes a set of semaphores
 *
 * @param n_sem Number of semaphores to close
 * @param ... Sequence of semaphores and their respective names. Put NULL in any of them if you want to avoid close/unlink
 * @return int
 */
int close_semaphores(int n_sem, ...);

/**
 * @brief Closes a set of semaphores
 *
 * @param n_shm Shared memory file number to close
 * @param ... Sequence of structure triplets, structure size and name
 * @return int
 */
int close_shm(int n_shm, ...);

/**
 * @brief Create a shm file and map into the mapped buffer
 *
 * @param name Name of the file
 * @param size Size of the mapping
 * @param mapped Buffer where to place the structure
 * @return int mapping file descriptor
 */
int create_shm(char *name, size_t size, void **mapped);

/**
 * @brief Create a semaphore
 *
 * @param name Number of the semaphore
 * @param initial_val Inicial value of the semaphore
 * @return sem_t *(NULL if you fail)
 */
sem_t *create_sem(char *name, int initial_val);

/**
 * @brief Opens a shared memory file
 *
 * @param name Name of the file
 * @param size File size
 * @param mapped Buffer
 * @return int
 */
int open_shm(char *name, size_t size, void **mapped);

/**
 * @brief Abre a semaphore
 *
 * @param name Number of the semaphore
 * @return sem_t *(NULL if you fail)
 */
sem_t *open_sem(char *name);

/*LOGGING AND STANDARD OUTPUT*/
/**
 * @brief Indicates the configuration to use in message logging
 *
 * @param use_std Messages are printed to standard output
 * @param use_syslog Messages are printed to the system log
 * @param syslog_ident Specify identifier in the system log
 */
void set_log_conf(int use_std, int use_syslog, char *syslog_ident);

/**
 * @brief Log in using the arguments
 *
 * @param priority Priority in syslog if syslog is to be used
 * @param format Formatted string
 * @param param Argument list
 */
void vflog(int priority, char *format, va_list param);

/**
 * @brief Log in using the arguments
 *
 * @param priority Priority in syslog if syslog is to be used
 * @param format Formatted string
 */
void flog(int priority, char *format, ...);

/**
 * @brief Prints the formatted string of variable arguments and closes the process with exit(1)
 *
 * @param format Formatted character string
 * @param ... Parameters
 */
void errexit(char *formato, ...);

#endif /*UTILS_H*/
