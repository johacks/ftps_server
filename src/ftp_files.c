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
#include "ftp_files.h"
#define SEND_BUFFER XXXL_SZ /*!< Tamaño de buffer de envio */
#define IP_LEN sizeof("xxx.xxx.xxx.xxx") /*!< Tamaño de ipv4 */
#define LS_CMD "ls -l1 --numeric-uid-gid --hyperlink=never --time-style=iso --color=never '" /*!< Comando ls */
#define IP_FIELD_LEN sizeof("xxx") /*!< Tamaño de un subcampo de ipv4 */
#define VIRTUAL_ROOT "/" /*!< Raiz */

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
    char *buff = alloca((sizeof(LS_CMD) + strlen(current_dir) + strlen(path) + 1) * sizeof(char));
    if ( !buff ) return NULL;
    /* Recoger path */
    strcpy(buff, LS_CMD);
    if ( get_real_path(current_dir, path, &buff[strlen(buff)]) < 0 ) return NULL;
    strcat(buff, "'"); /* Quote de nombre de fichero */
    /* Llamada a ls, redirigiendo el output */
    FILE *output = popen(buff, "r");
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
 * 
 * @param ctx Contexto TLS
 * @param socket_fd Descriptor del socket
 * @param buf a enviar
 * @param buf_len cuanto enviar
 * @param ascii_mode Si no 0, transformar saltos de linea a formato universal
 * @return ssize_t si menor que 0, error
 */
ssize_t send_buffer(struct TLSContext *ctx, int socket_fd, char *buf, size_t buf_len, int ascii_mode)
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
        return ssend(ctx, socket_fd, ascii_buf, new_buflen, MSG_NOSIGNAL); /* Enviar contenido filtrado */
    }
    return ssend(ctx, socket_fd, buf, buf_len, MSG_NOSIGNAL); /* Enviar contenido */
}

/**
 * @brief Enviar el contenido de f a traves de un socket
 * 
 * @param ctx Contexto TLS
 * @param socket_fd Descriptor del socket
 * @param f Fichero a abrir
 * @param ascii_mode Modo ascii
 * @param abort_transfer Permite cancelar transferencia
 * @return ssize_t
 */
ssize_t send_file(struct TLSContext *ctx, int socket_fd, FILE *f, int ascii_mode, int *abort_transfer)
{
    int aux = 0;
    ssize_t sent_b, read_b, total = 0;
    char buf[SEND_BUFFER];
    if ( !abort_transfer ) abort_transfer = &aux;

    /* Enviar contenido de f en bloques */
    while ( !(*abort_transfer) && (read_b = fread(buf, sizeof(char), SEND_BUFFER, f)) )
    {
        if ( *abort_transfer ) return 0; /* Se puede cambiar un flag externo para cancelar la transferencia */
        if ( (sent_b = send_buffer(ctx, socket_fd, buf, read_b, ascii_mode)) < 0 )
            return -1;
        total += sent_b;
    }
    return total; /* Transferencia correcta */
}

/**
 * @brief Leer contenido de un socket a un buffer
 * 
 * @param ctx Contexto TLS
 * @param socket_fd Socket origen
 * @param dest Buffer destino
 * @param buf_len Tamaño de buffer
 * @param ascii_mode Modo ascii
 * @return ssize_t leidos
 */
ssize_t read_to_buffer(struct TLSContext *ctx, int socket_fd, char *dest, size_t buf_len, int ascii_mode)
{
    /* Filtrar lo leido de ascii a local (CRLF a LF) */
    if ( ascii_mode )
    {
        char *ascii_buf = alloca(buf_len);
        ssize_t n_read, new_buflen = 0;
        if ( (n_read = srecv(ctx, socket_fd, ascii_buf, buf_len, MSG_NOSIGNAL)) <= 0 )
            return n_read;
        for ( int i = 0; i < n_read; i++ )
            if ( ascii_buf[i] != '\r' )
                dest[new_buflen++] = ascii_buf[i];
        return new_buflen; 
    }
    return srecv(ctx, socket_fd, dest, buf_len, MSG_NOSIGNAL);
}

/**
 * @brief Lee el contenido de un socket a un fichero
 * 
 * @param ctx Contexto TLS
 * @param f Fichero destino.
 * @param socket_fd socket origen
 * @param ascii_mode Modo de trasferencia FTP
 * @param abort_transfer Permite cancelar la transferencia
 * @return ssize_t bytes leido 
 */
ssize_t read_to_file(struct TLSContext *ctx, FILE *f, int socket_fd, int ascii_mode, int *abort_transfer)
{
    int aux = 0;
    ssize_t sent_b, read_b, total = 0;
    char buf[SEND_BUFFER];
    if ( !abort_transfer ) abort_transfer = &aux;

    /* Lee de buffer en buffer hasta terminar o ser interrumpido */
    while ( !(*abort_transfer) && ((read_b = read_to_buffer(ctx, socket_fd, buf, SEND_BUFFER, ascii_mode)) > 0) )
    {
        if ( (sent_b = fwrite(buf, sizeof(char), read_b, f)) != read_b )
            return -1;
        total += read_b;
    }
    return total;
}

/**
 * @brief Parsea un port string de formato xxx,xxx,xxx,xxx,ppp,ppp
 * 
 * @param port_string Argumento del comando PORT
 * @param clt_ip Buffer donde se guardara la ip del cliente
 * @param clt_port Donde se guardara el puerto del cliente
 * @return int menor que 0 si error
 */
int parse_port_string(char *port_string, char *clt_ip, int *clt_port)
{
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
    strncpy(clt_ip, start, len - 1); /* Copiar IP omitiendo la , final */

    /* PUERTO */
    *clt_port = 0;
    /* Byte superior */
    field_width = strcspn(port_string, ",");
    if ( !field_width || field_width > sizeof("xxx") || port_string[field_width] != ',' || !is_number(port_string, field_width) )
        return -1;
    port_string[field_width] = '\0';
    *clt_port += (atoi(port_string) << 8); /* Multiplicar por 256 */
    port_string = &port_string[field_width + 1];
    /* Byte inferior */
    field_width = strlen(port_string);
    if ( !field_width || field_width > sizeof("xxx") || !is_number(port_string, field_width) )
        return -1;
    *clt_port += atoi(port_string);
    return 1;   
}

/**
 * @brief Crea un socket de datos para que se conecte el cliente
 * 
 * @param srv_ip Ip del servidor 
 * @param passive_port_count Semaforo con el numero de conexiones de datos creadas
 * @param socket_fd Socket resultante
 * @return int menor que 0 si error, si no, puerto
 */
int passive_data_socket_fd(char *srv_ip, sem_t *passive_port_count, int *socket_fd)
{
    /* No hay sockets de datos disponibles en este momento */
    if ( sem_trywait(passive_port_count) == -1 && errno == EAGAIN )
        return -1;
    if ( (*socket_fd = socket_srv("tcp", 10, 0, srv_ip)) < 0 )
        return *socket_fd;
    /* Establecer un timeout */
    set_socket_timeouts(*socket_fd, DATA_SOCKET_TIMEOUT);
    /* Devolver el puerto que ha sido encontrado */
    struct sockaddr_in addrinfo;
    socklen_t info_len = sizeof(struct sockaddr);
    getsockname(*socket_fd, (struct sockaddr *) &addrinfo, &info_len);
    return ntohs(addrinfo.sin_port);
}

/**
 * @brief Genera port string para el comando PASV
 * 
 * @param ip Ip del servidor
 * @param port Puerto de datos
 * @param buf Buffer que almacenara el resultado
 * @return char* Resultado
 */
char *make_port_string(char *ip, int port, char *buf)
{
    int i;
    strcpy(buf, ip);
    for ( i = 0; i < strlen(buf); i++ ) /* Puntos por comas en ip */
        if ( buf[i] == '.' )
            buf[i] = ',';
    strcat(&buf[i++], ","); /* Y una coma al final */
    div_t p1p2 = div(port, 256); /* Obtener ambos numeros del puerto y ponerlos al final */
    sprintf(&buf[i], "%d,%d", p1p2.quot, p1p2.rem);
    return buf;
}

/**
 * @brief Devuelve un puntero al path sin el root
 * 
 * @param full_path Path completo
 * @return char* Puntero al path sin el root
 */
char *path_no_root(char *full_path)
{
    if ( full_path[root_size] == '/' ) /* Se ha preguntado por un path que va mas alla del root */
        return &full_path[root_size];
    return VIRTUAL_ROOT; /* Se ha preguntado por un path que ya era el root */
}