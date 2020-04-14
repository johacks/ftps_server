/**
 * @file path.c
 * @author Joaquín Jiménez López de Castro (joaquin.jimenezl@estudiante.uam.es)
 * @brief Manipulacion de paths, ficheros y directorios
 * @version 1.0
 * @date 14-04-2020
 * 
 * @copyright Copyright (c) 2020
 * 
 */

#define _DEFAULT_SOURCE
#include "utils.h"
#include "red.h"
#include "ftp.h"
#include "config_parser.h"
#define SEND_BUFFER 2048

static char root[SERVER_ROOT_MAX] = "";
static size_t root_size; 

/**
 * @brief Por comodidad del programador para no tener que indicar el path
 * en cada llamada, se establece el path en una variable estatica
 * @param server_root 
 */
void set_root_path(char *server_root)
{
    strncpy(root, server_root, SERVER_ROOT_MAX - 1);
    root_size = strlen(root);
}

/**
 * @brief Limpia un path y coloca el path absoluto real_path, utilizando el directorio actual
 * si path es relativo para poder generarlo
 * 
 * @param current_dir 
 * @param path 
 * @param real_path 
 * @return int 
 */
int get_real_path(char *current_dir, char *path, char *real_path)
{
    char buff = alloca((strlen(current_dir) + strlen(path) + 1)*sizeof(char)); /* 'Alloca' libera memoria de funcion automaticamente */
    if ( !buff ) return -1;

    /* Si el path dado es relativo, usaremos el directorio actual */
    if ( path[0] == '\0' || path[0] != '/' )
    {
        strcpy(buff, current_dir);
        strcat(buff, "/");
        strcat(buff, path);
    }
    /* Si el path es absoluto, concatenamos al root */
    else
    {
        strcpy(buff, root);
        strcat(buff, path);
    }
    
    /* El siguiente paso es resolver el path */
    if ( !realpath(buff, real_path) ) return -2;
    /* Finalmente, comprobar que el path es root o un subdirectorio de este */
    if ( memcmp(real_path, root, root_size) ) return -2;
    /* Tras la cadena de root, o acaba el path, o hay subdirectorios, que no contienen '..' gracias a realpath */
    if ( !(real_path[root_size] == '\0' || real_path[root_size] =='/') ) return -2;

    /* Path correcto y listo en real_path */
    return real_path;
}

/**
 * @brief Devuelve un fichero con el output de ls
 * 
 * @param path Path sin limpiar
 * @param current_dir Directorio actual de sesion FTP
 * @return FILE* Como cualquier otro FILE, sin embargo, debe cerrarse con pclose
 */
FILE *list_directories(char *path, char *current_dir)
{
    int ret;
    char buff = alloca((strlen(current_dir) + strlen(path) + 1) * sizeof(char));
    if ( !buff ) return NULL;
    /* Recoger path */
    if ( (ret = get_real_path(current_dir, path, buff)) < 0 ) return ret;

    /* Llamada a ls, redirigiendo el output */
    FILE *output = popen("ls -lbq --numeric-uid-gid --hyperlink=never --color=never", "r");

    return output;
}

/**
 * @brief Abre un fichero, ayudandose de current_dir para localizarlo
 * 
 * @param path path relativo o absoluto al fichero
 * @param current_dir directorio actual, posiblemente necesario
 * @param mode modo de apertura del fichero
 * @return FILE* 
 */
FILE *file_open(char *path, char *current_dir, char *mode)
{
    int ret;
    char buff = alloca((strlen(current_dir) + strlen(path) + 1) * sizeof(char));
    if ( !buff ) return NULL;
    /* Recoger path */
    if ( (ret = get_real_path(current_dir, path, buff)) < 0 ) return ret;

    /* Llamada a fopen con el path ya completo */
    return fopen(buff, mode);
}

/**
 * @brief Indica si un path apunta a un directorio
 * 
 * @param path Path al directorio
 * @return int 1 si verdadero
 */
int path_is_dir(char *path)
{
   struct stat st;
   if ( stat(path, &st) < 0 )
       return 0;
   return S_ISDIR(st.st_mode);
}

/**
 * @brief Indica si un path apunta a un fichero
 * 
 * @param path Path al fichero
 * @return int 1 si verdadero
 */
int path_is_file(char *path)
{
   struct stat st;
   if ( stat(path, &st) < 0 )
       return 0;
   return S_ISREG(st.st_mode);
}

/**
 * @brief Modifica el directorio actual
 * 
 * @param current_dir Directorio actual
 * @param path Path al directorio nuevo
 * @return int 1 si correcto, -1 fallo de memoria, -2 path incorecto
 */
int ch_current_dir(char *current_dir, char *path)
{
    int ret;
    char buff = alloca((strlen(current_dir) + strlen(path) + 1) * sizeof(char));
    if ( !buff ) return -1;
    /* Recoger path */
    if ( (ret = get_real_path(current_dir, path, buff)) < 0 ) return ret;

    /* Permisos de acceso y directorio existe */
    if ( access(buff, R_OK) != 0 )
        return -2;

    /* Debe ser un directorio */
    if ( !path_is_dir(buff) )
        return -2;

    /* Todo correcto, cambiar el directorio actual */
    strcpy(current_dir, buff);
    return 1;
}

/**
 * @brief Cambiar al directorio padre
 * 
 * @param current_dir Directorio actual
 * @return int 0 si ya en root, 1 si cambio efectivo, -1 fallo de memoria
 */
int ch_to_parent_dir(char *current_dir)
{
    char buff = alloca((strlen(current_dir) + 1) * sizeof(char));
    if ( !buff ) return -1;
    /* Recoger path del directorio superior, si ya estamos en root, no nos cambiamos */
    if ( get_real_path(current_dir, "../", buff) < 0 )
        return 0;
    strcpy(current_dir, buff);
    return 1;
}

/**
 * @brief Envia el contenido de un buffer a traves de un socket con posibilidad de
 *        que se le aplique un filtro ascii
 * @param socket_fd Descriptor del socket
 * @param buf a enviar
 * @param buf_len cuanto enviar
 * @param ascii_mode Si no 0, transformar saltos de linea a formato universal
 * @return ssize_t si menor que 0, error
 */
ssize_t send_buffer(int socket_fd, char *buf, size_t buf_len, int ascii_mode)
{
    /* Transmision en modo VST ascii */
    if ( ascii_mode )
    {
        char *ascii_buf = alloca( 2 * buf_len ); /* Puede crecer hasta el doble */
        if ( !ascii_buf ) return -1;
        size_t new_buflen = 0;
        for (int i = 0; i < buf_len; i++)
        {
            if ( buf[i] == '\n' ) /* Saltos de linea del servidor son \n */
                ascii_buf[new_buflen++] = '\r';
            ascii_buf[new_buflen++] = (buf[i] & 0x7F); /* Bit mas significativo a 0 siempre */ 
        }
        return send(socket_fd, ascii_buf, new_buflen, MSG_NOSIGNAL); /* Enviar contenido filtrado */
    }
    return send(socket_fd, buf,buf_len, MSG_NOSIGNAL); /* Enviar contenido */
}

/**
 * @brief Enviar el contenido de f a traves de un socket
 * 
 * @param socket_fd Descriptor del socket
 * @param f Fichero a abrir
 * @param ascii_mode Modo ascii
 * @param abort_transfer Permite cancelar transferencia
 * @return ssize_t
 */
ssize_t send_file(int socket_fd, FILE *f, int ascii_mode, int *abort_transfer)
{
    int aux = 0;
    ssize_t sent_b, read_b, total;
    char buf[SEND_BUFFER];
    if ( !abort_transfer ) abort_transfer = &aux;

    /* Enviar contenido de f en bloques */
    while ( !(*abort_transfer) && (read_b = fread(send_buffer, sizeof(char), SEND_BUFFER, f)) )
    {
        if ( *abort_transfer ) return 0; /* Se puede cambiar un flag externo para cancelar la transferencia */
        if ( (sent_b = send_buffer(socket_fd, buf, read_b, ascii_mode)) < 0 )
            return -1;
        total += sent_b;
    }
    return total; /* Transferencia correcta */
}

ssize_t read_to_buffer(int socket_fd, char *dest, size_t buf_len, int ascii_mode)
{
    /* Filtrar lo leido de ascii a local (CRLF a LF) */
    if ( ascii_mode )
    {
        char *ascii_buf = alloca(buf_len);
        ssize_t n_read;
        if ( (n_read = recv(socket_fd, ascii_buf, buf_len, MSG_NOSIGNAL)) < 0 )
            return -1;
        ssize_t new_buflen = 0;
        for ( int i = 0; i < n_read; i++ )
            if ( ascii_buf[i] != '\r' )
                dest[new_buflen++] = ascii_buf[i];
        return new_buflen; 
    }
    return recv(socket_fd, dest, buf_len, MSG_NOSIGNAL);
}

/**
 * @brief Lee el contenido de un socket a un fichero
 * 
 * @param f Fichero destino.
 * @param socket_fd socket origen
 * @param ascii_mode Modo de trasferencia FTP
 * @param abort_transfer Permite cancelar la transferencia
 * @return ssize_t bytes leido 
 */
ssize_t read_to_file(FILE *f, int socket_fd, int ascii_mode, int *abort_transfer)
{
    int aux = 0;
    ssize_t sent_b, read_b, total;
    char buf[SEND_BUFFER];
    if ( !abort_transfer ) abort_transfer = &aux;

    while ( !(*abort_transfer) && (read_b = read_to_buffer(socket_fd, buf, SEND_BUFFER, ascii_mode)) > 0 )
    {
        if ( (sent_b = fwrite(buf, sizeof(char), SEND_BUFFER, f)) != read_b )
            return -1;
        total += read_b;
    }
    return total;
}

int active_data_socket_fd(char *port_string)
{
    
}