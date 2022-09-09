/**
 * @file utils.c
 * @author Joaquín Jiménez López de Castro (joaquin.jimenezl@estudiante.uam.es)
 * @brief Implementation of various utility functions
 * @version 1.0
 * @date 04-07-2020
 *
 */

#include "utils.h"

/**
 * @brief Size of a file given its descriptor
 *
 * @param fd file descriptor
 * @return size_t size
 */
size_t file_size(int fd)
{
    struct stat st;
    if (fd < 0 || fstat(fd, &st) == -1)
        return -1;
    return st.st_size;
}

/**
 * @brief Indicates the operating system in use
 *
 * @return char*
 */
char *operating_system()
{
#ifdef __linux__
    return "Linux";
#elif __FreeBSD__
    return "FreeBSD";
#elif __unix || __unix__
    return "Unix";
#else
    return "Other";
#endif
}

/**
 * @brief Size of a file given its name
 *
 * @param path Name of the file
 * @return size_t size
 */
size_t name_file_size(char *path)
{
    struct stat st;
    if (!path || stat(path, &st) == -1)
        return -1;
    return st.st_size;
}

/**
 * @brief Str is an integer
 *
 * @param str String
 * @param len Size to parse, if you want to call strlen, put less than 0
 * @return int
 */
int is_number(char *str, int len)
{
    len = (len < 0) ? strlen(str) : len;
    for (int i = 0; i < len; i++)
        if (!isdigit(str[i]))
            return 0;
    return 1;
}

/*SEMAPHORES AND SHARED MEMORY*/
/**
 * @brief Closes a set of semaphores
 *
 * @param n_sem Number of semaphores to close
 * @param ... Sequence of semaphores and their respective names. Put NULL in any of them if you want to avoid close/unlink
 * @return int
 */
int close_semaphores(int n_sem, ...)
{
    va_list param;
    sem_t *sem;
    char *sem_name;

    n_sem *= 2;
    va_start(param, n_sem);
    for (int i = n_sem; i >= 0; i--)
    {
        sem = va_arg(param, sem_t *);
        sem_name = va_arg(param, char *);
        if (sem)
            sem_close(sem);
        if (sem_name)
        {
            sem_unlink(sem_name);
        }
    }
    va_end(param);

    return 1;
}

/**
 * @brief Closes a set of semaphores
 *
 * @param n_shm Shared memory file number to close
 * @param ... Sequence of structure triplets, structure size and name
 * @return int
 */
int close_shm(int n_shm, ...)
{
    va_list param;
    void *mapped = NULL;
    size_t size;
    char *name = NULL;
    n_shm *= 3;

    va_start(param, n_shm);
    for (int i = n_shm; i > 0; i -= 3)
    {
        mapped = va_arg(param, void *);
        size = va_arg(param, size_t);
        name = va_arg(param, char *);
        if (mapped)
            munmap(mapped, size);
        if (name)
            shm_unlink(name);
    }
    va_end(param);

    return 1;
}

/**
 * @brief Create a shm file and map into the mapped buffer
 *
 * @param name Name of the file
 * @param size Size of the mapping
 * @param mapped Buffer where to place the structure
 * @return int mapping file descriptor
 */
int create_shm(char *name, size_t size, void **mapped)
{
    int fd;

    /*Open shared memory file*/
    if ((fd = shm_open(name, O_RDWR | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR)) == -1)
        return -1;

    /*Resize the size of the shared memory area to that of the serverinfo structure*/
    if (ftruncate(fd, size) < 0)
    {
        close_shm(1, NULL, 0, name);
        return -1;
    }
    /*Bind the shared memory to the current process*/
    (*mapped) = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if ((*mapped) == MAP_FAILED)
    {
        close_shm(1, NULL, 0, name);
        return -1;
    }

    return fd;
}

/**
 * @brief Opens a shared memory file
 *
 * @param name Name of the file
 * @param size File size
 * @param mapped Buffer
 * @return int
 */
int open_shm(char *name, size_t size, void **mapped)
{
    int fd_shm;

    /*Open shared memory and get its descriptor*/
    if ((fd_shm = shm_open(name, O_RDWR, 0)) == -1)
        return -1;

    /*Bind the shared memory to the current process*/
    *mapped = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd_shm, 0);
    if ((*mapped) == MAP_FAILED)
        return -1;
    return fd_shm;
}

/**
 * @brief Create a semaphore
 *
 * @param name Number of the semaphore
 * @param initial_val Inicial value of the semaphore
 * @return sem_t *(NULL if you fail)
 */
sem_t *create_sem(char *name, int initial_val)
{
    sem_t *sem = NULL;
    if ((sem = sem_open(name, O_RDWR | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR, initial_val)) == SEM_FAILED)
        return NULL;
    return sem;
}

/**
 * @brief Abre a semaphore
 *
 * @param name Number of the semaphore
 * @return sem_t *(NULL if you fail)
 */
sem_t *open_sem(char *name)
{
    sem_t *sem = NULL;
    if ((sem = sem_open(name, O_RDWR)) == SEM_FAILED)
        return NULL;
    return sem;
}

/*LOGGING AND STANDARD OUTPUT*/

int _use_syslog = 0; /*!< Whether or not to use systemlog to print messages*/
int _use_std = 1;    /*!< Whether or not to use standard output to print messages*/
/**
 * @brief Indicates the configuration to use in message logging
 *
 * @param use_std Messages are printed to standard output
 * @param use_syslog Messages are printed to the system log
 * @param syslog_ident Specify identifier in the system log
 */
void set_log_conf(int use_std, int use_syslog, char *syslog_ident)
{
    _use_std = use_std;
    _use_syslog = use_syslog;
    if (use_syslog)
        openlog(syslog_ident, LOG_PID, LOG_FTP);
    return;
}

/**
 * @brief Log in using the arguments
 *
 * @param priority Priority in syslog if syslog is to be used
 * @param format Formatted string
 * @param param Argument list
 */
void vflog(int priority, char *format, va_list param)
{
    if (_use_std)
        vprintf(format, param);
    if (_use_syslog)
        vsyslog(priority, format, param);
    return;
}

/**
 * @brief Log in using the arguments
 *
 * @param priority Priority in syslog if syslog is to be used
 * @param format Formatted string
 */
void flog(int priority, char *format, ...)
{
    if (!format)
        return;
    va_list param;

    va_start(param, format);
    vflog(priority, format, param);
    va_end(param);
    return;
}

/**
 * @brief Prints the formatted string of variable arguments and closes the process with exit(1)
 *
 * @param format Formatted character string
 * @param ... Parameters
 */
void errexit(char *format, ...)
{
    if (!format)
        return;
    va_list param;

    va_start(param, format);
    vflog(LOG_ERR, format, param);
    va_end(param);

    exit(1);
}