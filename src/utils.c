/**
 * @file utils.c
 * @author Joaquín Jiménez López de Castro (joaquin.jimenezl@estudiante.uam.es)
 * @brief Implementacion de funciones de utilidad varias
 * @version 1.0
 * @date 07-04-2020
 * 
 */

#include "utils.h"

/**
 * @brief Tamaño de un fichero dado su descriptor
 * 
 * @param fd descriptor del fichero
 * @return size_t tamaño
 */
size_t file_size(int fd)
{
    struct stat st;

    if (fd < 0)
        return -1;
    fstat(fd, &st);
    
    return st.st_size;
}

/**
 * @brief Imprime la cadena formateada de argumentos variables en stderr y cierra el proceso con exit(1)
 * 
 * @param formato Cadena de caracteres formateada
 * @param ... Parametros
 */
void errexit(char* formato, ...)
{
    if ( !formato )
        return;
    va_list param;

    va_start(param, formato);
    vfprintf(stderr, formato, param);
    va_end(param);

    exit(1);
}

/**
 * @brief Cierra un conjunto de semaforos
 * 
 * @param n_sem Numero de semaforos a cerrar
 * @param ... Sucesion de semaforos y sus respectivos nombres. Poner NULL en cualquiera de ellos si se quiere evitar close/unlink
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
 * @brief Cierra un conjunto de semaforos
 * 
 * @param n_shm Numero de fichero de memoria compartida a cerrar
 * @param ... Sucesion de tripletos estructura, tamaño de estructura y nombre
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
    for (int i = n_shm; i > 0; i-= 3)
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
 * @brief Crea un fichero shm y mapea en el buffer mapped
 * 
 * @param name Nombre del ficherp
 * @param size Tamaño del mapeo
 * @param mapped Buffer donde colocar la estructura
 * @return int descriptor de fichero del mapeo
 */
int create_shm(char *name, size_t size, void **mapped)
{
    int fd;

    /* Abrir fichero de memoria compartida */
	if( (fd = shm_open(name, O_RDWR | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR)) == -1 )
		return -1;

	/* Redimensiona el tamaño de la zona de memoria compartida al de la estructura serverinfo */
	if( ftruncate(fd, size) < 0)
    {
		close_shm(1, NULL, 0, name);
        return -1;
	}
	/* Enlaza la memoria compartida al proceso actual */
	(*mapped) = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if((*mapped) == MAP_FAILED) 
    {
        close_shm(1, NULL, 0, name);
        return -1;
	}

    return fd;
}

/**
 * @brief Abre un fichero de memoria compartida
 * 
 * @param name Nombre del fichero
 * @param size Tamaño del fichero
 * @param mapped Buffer
 * @return int 
 */
int open_shm(char *name, size_t size, void **mapped)
{
    int fd_shm;

    /* Abrir memoria compartida y obtener su descriptor */
	if( (fd_shm = shm_open(name, O_RDWR, 0)) == -1 )
		return -1;

	/* Enlaza la memoria compartida al proceso actual */
	*mapped = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd_shm, 0);
	if((*mapped) == MAP_FAILED)
		return -1;
    return fd_shm;    
}

/**
 * @brief Crea un semaforo
 * 
 * @param name Nombre del semaforo
 * @param initial_val Valor inicial del semaforo
 * @return sem_t* (NULL si fallo)
 */
sem_t *create_sem(char *name, int initial_val)
{
    sem_t *sem = NULL;
    if ((sem = sem_open(name, O_RDWR | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR, initial_val)) == SEM_FAILED)
        return NULL;
    return sem;
}

/**
 * @brief Abre un semaforo
 * 
 * @param name Nombre del semaforo
 * @return sem_t* (NULL si fallo)
 */
sem_t *open_sem(char *name)
{
    sem_t *sem = NULL;
    if ((sem = sem_open(name, O_RDWR)) == SEM_FAILED)
        return NULL;
    return sem;
}