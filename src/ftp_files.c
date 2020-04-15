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
#define IP_LEN sizeof("xxx.xxx.xxx.xxx")
#define IP_FIELD_LEN sizeof("xxx")

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
    char *buff = alloca((strlen(current_dir) + strlen(path) + 1)*sizeof(char)); /* 'Alloca' libera memoria de funcion automaticamente */
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

    int ret = 1;
    if ( !realpath(buff, real_path) )
    {
        if ( errno != ENOENT )
            return -2;
        char *buff2 = alloca(strlen(buff) + 1);
        if ( !buff2 ) return -1;
        strcpy(buff2, buff);
        /* Si es por archivo no existente, mirar a ver si al menos el directorio existe */
        if ( !realpath(dirname(buff), real_path) ) 
            return -2;
        /* Si el directorio si existe, posiblemente el path es a un fichero que se va a crear */
        strcat(real_path, "/");
        strcat(real_path, basename(buff2));
        ret = 0;
    }

    /* Finalmente, comprobar que el path es root o un subdirectorio de este */
    if ( memcmp(real_path, root, root_size) ) return -2;
    /* Tras la cadena de root, o acaba el path, o hay subdirectorios, que no contienen '..' gracias a realpath */
    if ( !(real_path[root_size] == '\0' || real_path[root_size] =='/') ) return -2;

    /* Path correcto y listo en real_path */
    return ret;
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
    char *buff = alloca((strlen(current_dir) + strlen(path) + 1) * sizeof(char));
    if ( !buff ) return NULL;
    /* Recoger path */
    if ( get_real_path(current_dir, path, buff) < 0 ) return NULL;

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
    char *buff = alloca((strlen(current_dir) + strlen(path) + 1) * sizeof(char));
    if ( !buff ) return NULL;
    /* Recoger path */
    if ( get_real_path(current_dir, path, buff) < 0 ) return NULL;

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
    char *buff = alloca((strlen(current_dir) + strlen(path) + 1) * sizeof(char));
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
    char *buff = alloca((strlen(current_dir) + 1) * sizeof(char));
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

/**
 * @brief Leer contenido de un socket a un buffer
 * 
 * @param socket_fd Socket origen
 * @param dest Buffer destino
 * @param buf_len Tamaño de buffer
 * @param ascii_mode Modo ascii
 * @return ssize_t leidos
 */
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

/**
 * @brief Conectarse en modo activo a un puerto de datos del cliente
 * 
 * @param port_string Argumento del comando PORT
 * @param srv_ip Ip de despliegue del servidor
 * @return int menor que 0 si error
 */
int active_data_socket_fd(char *port_string, char *srv_ip)
{
    char ip[IP_LEN] = "";
    int port = 0;
    int len, comma_count;
    size_t field_width, command_len = strlen(port_string);
    char *start = port_string;

    /* IP */
    for ( len = 0, comma_count = 4; comma_count; comma_count--, port_string = &port_string[field_width + 1])
    {
        field_width = strcspn(port_string, ",");
        /* Campo correcto */
        if ( field_width >= IP_FIELD_LEN || !field_width || !is_number(port_string, field_width) )
            return -1;
        if ( comma_count > 1 ) port_string[field_width] = '.';
        len += (field_width + 1);
        if ( len >= command_len ) return -1;
    }
    strncpy(ip, start, len - 1); /* Copiar IP omitiendo la , final */

    /* PUERTO */
    /* Byte superior */
    field_width = strcspn(port_string, ",");
    if ( !field_width || field_width > sizeof("xxx") || port_string[field_width] != ',' || !is_number(port_string, field_width) )
        return -1;
    port_string[field_width] = '\0';
    port += (atoi(port_string) << sizeof(char));
    port_string = &port_string[field_width + 1];
    /* Byte inferior */
    field_width = strlen(port_string);
    if ( !field_width || field_width > sizeof("xxx") || !is_number(port_string, field_width) )
        return -1;
    port += atoi(port_string);
    
    return socket_clt_connection(FTP_DATA_PORT, srv_ip, port, ip);
}

/**
 * @brief Crea un socket de datos para que se conecte el cliente
 * 
 * @param srv_ip Ip del servidor 
 * @param passive_port_count Semaforo con el numero de conexiones de datos creadas
 * @param qlen Tamaño maximo de la cola de conexiones pendientes
 * @return int 
 */
int passive_data_socket_fd(char *srv_ip, sem_t *passive_port_count, int qlen)
{
    int socket_fd;
    /* No hay sockets de datos disponibles en este momento */
    if ( sem_trywait(passive_port_count) == -1 && errno == EAGAIN )
        return -1;
    if ( (socket_fd = socket_srv("tcp", qlen, 0, srv_ip)) < 0 )
        return socket_fd;
    
    /* Devolver el puerto que ha sido encontrado */
    struct sockaddr_in addrinfo;
    socklen_t info_len = sizeof(struct sockaddr);
    getsockname(socket_fd, (struct sockaddr *) &addrinfo, &info_len);
    return ntohs(addrinfo.sin_port);
}