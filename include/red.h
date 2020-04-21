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
#include "tlse.h"
         

/* Strings asociados a protocolos */
#define TCP "tcp" /*!< Protocolo tcp */ 
#define UDP "udp" /*!< Protocolo udp */


/******************************
    MANIPULACION Y CREACION DE SOCKETS
*******************************/

/* Obtenido de https://stackoverflow.com/questions/4181784/how-to-set-socket-timeout-in-c-when-making-multiple-connections */
/**
 * @brief Añade opciones a un socket para que las operaciones de escritura y lectura de un socket tengan un timeout maximo
 * 
 * @param socket_fd Socket a modificar
 * @param seconds Numero de segundos de tiemout
 */
int set_socket_timeouts(int socket_fd, int seconds);

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
 * @param ip_srv Ip del servidor
 * @return int (descriptor de fichero del socket creado)
 */
int socket_srv(const char *proto_transp, int qlen, int puerto, char *ip_srv);

/**
 * @brief Crea un socket de cliente y hace un bind en un puerto especifico
 * 
 * @param proto_transp Protocolo a utilizar: tcp o udp
 * @param puerto Puerto del cliente, poner 0 si no se quiere uno en concretos
 * @param ip_clt Ip del cliente en ascii
 * @return int 
 */
int socket_clt(const char *proto_transp, char *ip_clt, int puerto);

/**
 * @brief Conecta un socket de client a un socket de servidor a partir de su ip y numero de puerto
 * 
 * @param socket_fd Descriptor de fichero del socket cliente
 * @param ip_srv Direccion ip del socket servidor, cadena de formato "w.x.y.z"
 * @param puerto_srv Numero de puerto del servidor
 * @return int 
 */
int socket_clt_connect(int socket_fd, char *ip_srv, int puerto_srv);

/**
 * @brief Abre un socket de cliente y se conecta a un servidor
 * 
 * @param puerto_clt Puerto del cliente
 * @param ip_clt Ip del cliente
 * @param puerto_srv Puerto del servidor
 * @param ip_srv Ip del servidor
 * @return int Menor que 0 si error
 */
int socket_clt_connection(int puerto_clt, char *ip_clt, int puerto_srv, char *ip_srv);

/**
 * @brief Escribe en un socket el contenido del fichero src_fd
 * 
 * @param client_fd descriptor del socket
 * @param src_fd descriptor del fichero origen
 * @return int numero de bytes escritos
 */
int socket_dump_fd(int socket_fd, int src_fd);

#endif /* RED_H */