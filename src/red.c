/**
 * @file red.c
 * @author Joaquin Jimenez Lopez de Castro (joaquin.jimenezl@estudiante.uam.es)
 * @brief Implementacion de la libreria red
 * @version 1.0
 * @date 02-02-2020
 * 
 * 
 */
#include <syslog.h>
#include <ctype.h>
#include "red.h"

int socket_proto(const char *proto_transp, struct sockaddr_in *sock_info, int puerto);

/**
 * @brief Crea un socket para un servidor dado el protocolo a utilizar, puerto y el tamaño de buffer de la cola.
 *        Realiza el bind (y el listen si corresponde).
 * @details Basado en: https://www.tenouk.com/Module40a.html
 *          info sobre protoent: https://linux.die.net/man/3/getprotobyname 
 *          info sobre sockaddr: https://www.gta.ufrj.br/ensino/eel878/sockets/sockaddr_inman.html
 *          info sobre socket(): man socket
 *          info sobre bind():   man 2 bind
 *          info sobre listen(): man 2 listen
 * 
 * @param proto_transp String que representa el nombre del protocolo: ej "udp", "tcp"... Si NULL o incorrecto, usa TCP
 * @param qlen Tamaño del buffer de la cola del socket
 * @param puerto Numero de puerto donde se abrira el socket 
 * @return int (descriptor de fichero del socket creado)
 */
int socket_srv(const char *proto_transp, int qlen, int puerto)
{
    struct sockaddr_in sock_info;       /* Informacion sobre direcciones del socket */
    int socket_fd;                      /* Descriptor de fichero del socket */

    /* Obtiene fd del socket de especificaciones necesarias para el protocolo indicado */
    if ( (socket_fd = socket_proto(proto_transp, &sock_info, puerto)) < 0 )
        return socket_fd;

    /* Enlazamos el socket a la informacion que hemos rellenado */
    if ( bind(socket_fd, (struct sockaddr *)&sock_info, sizeof(sock_info)) < 0 )
        return -2;
        
    /* Si el socket no es udp, no hace falta el listen; intentamos hacerlo con CdE */
    if ( (!proto_transp || !strcmp(proto_transp, TCP)) && listen(socket_fd, qlen) < 0 )
        return -3;

    return socket_fd;
}

/**
 * @brief Crea un socket de cliente y se conecta al servidor en el puerto indicado
 * 
 * @param proto_transp Protocolo a utilizar: tcp o udp
 * @param puerto Puerto del servidor
 * @return int 
 */
int socket_clt(const char *proto_transp, int puerto)
{
    struct sockaddr_in sock_info;       /* Informacion sobre direcciones del socket destino */
    int socket_fd;

    /* Crea un socket para conectarse (o no) al servidor */
    socket_fd = socket_proto(proto_transp, &sock_info, puerto);

    /* Se asume que el cliente usara recvfrom y sendto si es UDP; si no, hacer un connect*/
    if ( !proto_transp || !strcmp(proto_transp, TCP) )
        if ( (connect(socket_fd, (struct sockaddr *)&sock_info, sizeof(sock_info))) < 0 )
            return -4;

    return socket_fd;
}

/**
 * @brief Crea un socket con las especificaciones propias del protocolo indicado: "udp" o "tcp"
 * 
 * @param proto_transp Nombre del protocolo
 * @param sock_info Estructura donde almacenar info de socket
 * @param puerto Puerto origen o destino segun corresponda
 * @return int Descriptor de fichero del socket
 */
int socket_proto(const char *proto_transp, struct sockaddr_in *sock_info, int puerto)
{
    struct protoent *ppe;               /* Informacion de protocolo */
    int proto_num;                      /* Numero asociado al protocolo */
    int tipo_socket;                    /* Tipo de socket: stream, datagramas... */
    int socket_fd;                      /* Descriptor de fichero del socket */
    int optval = 1;

    /* Obtenemos el numero asociado al protocolo pasado como argumento */
    if ( !proto_transp || (ppe = getprotobyname(proto_transp)) == 0 )
        proto_num = 0;
    else
        proto_num = ppe->p_proto;

    /* Elegimos tipo de socket en funcion de protocolo */
    if ( !proto_num || strcmp(proto_transp, UDP) )
        tipo_socket = SOCK_STREAM;
    else
        tipo_socket = SOCK_DGRAM;

    /* Abrir el socket que como todo en POSIX, es un fichero, representado por un descriptor */
    if ( (socket_fd = socket(AF_INET, tipo_socket, proto_num)) < 0 )
		return socket_fd;

    if ( setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) < 0 )
        return -2;

    /* Si es socket TCP, usamos TCP_CORK para mayor eficiencia a la hora de enviar cabeceras */
    if ( tipo_socket == SOCK_STREAM )
        setsockopt(socket_fd, ppe->p_proto, TCP_CORK, &optval, sizeof(int));

    /* Inicializamos estructura de dirección y puerto */
	bzero(sock_info, sizeof(struct sockaddr_in));   /* Rellena de 0s, equivalente a memset */
	sock_info->sin_family = AF_INET;                /* Dominio internet */
	sock_info->sin_port = htons(puerto);            /* htons() convierte a formato de red */
	sock_info->sin_addr.s_addr = INADDR_ANY;        /* direccio IP */

    return socket_fd;
}

/**
 * @brief Escribe en un socket el contenido del fichero src_fd
 * 
 * @param client_fd descriptor del socket
 * @param src_fd descriptor del fichero origen
 * @return int numero de bytes escritos
 */
int socket_dump_fd(int client_fd, int src_fd)
{
    size_t tam = file_size(src_fd);
    /* Enviar todo el fichero */
    return sendfile(client_fd, src_fd, 0, tam);
}