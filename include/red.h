/**
 * @file red.h
 * @author Joaquin Jimenez Lopez de Castro (joaquin.jimenezl@estudiante.uam.es)
 * @brief Declaraciones e inclusion de bibliotecas útiles para la programacion de red en C en un entorno UNIX
 * @version 1.0
 * @date 01-02-2020
 * 
 */

#ifndef RED_H
#define RED_H
#ifndef _GNU_SOURCE
#define _GNU_SOURCE       /*!< Permite usar memfd_create */
#endif
#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE   /*!< Estructuras para sockets */
#endif

#include <sys/socket.h>   /* Funciones de sockets */
#include <sys/sendfile.h> /* Para poder enviar un fichero */
#include <resolv.h>       /* man resolver (3) */
#include <arpa/inet.h>    /* man inet(3) */
#include <netinet/tcp.h>  /* TCP_CORK */
#include <netdb.h>        /* Permite identificar mediante el nombre un protocolo */
#include "utils.h"
         

/* Strings asociados a protocolos */
#define TCP "tcp" /*!< Protocolo tcp */ 
#define UDP "udp" /*!< Protocolo udp */


/******************************
    MANIPULACION Y CREACION DE SOCKETS
*******************************/

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
int socket_srv(const char *proto_transp, int qlen, int puerto);

/**
 * @brief Crea un socket de cliente y se conecta al servidor en el puerto indicado
 * 
 * @param proto_transp Protocolo a utilizar: tcp o udp
 * @param puerto Puerto del servidor
 * @return int 
 */
int socket_clt(const char *proto_transp, int puerto);

/**
 * @brief Escribe en un socket el contenido del fichero src_fd
 * 
 * @param client_fd descriptor del socket
 * @param src_fd descriptor del fichero origen
 * @return int numero de bytes escritos
 */
int socket_dump_fd(int socket_fd, int src_fd);

#endif /* RED_H */