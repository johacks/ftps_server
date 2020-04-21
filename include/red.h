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

/**
 * @brief Lee contenido de un fichero a un buffer
 * https://github.com/eduardsui/tlse
 * @param fname Nombre del fichero
 * @param buf Buffer destino
 * @param max_len Tamaño maximo de buffer
 * @return int menor o igual que 0 si error
 */
int read_from_file(const char *fname, char *buf, int max_len);

/**
 * @brief Carga clave y certificado del servidor al contexto
 * https://github.com/eduardsui/tlse
 * @param context Contexto TLS
 * @param fname Nombre del fichero pem con certificado
 * @param priv_fname Nombre del fichero pem con clave privada
 * 
 * @return int 1 si todo ok, 0 si error
 */
int load_keys(struct TLSContext *context, char *fname, char *priv_fname);

/**
 * @brief Manda bytes de protocolo TLS que quedan pendientes de envio
 * https://github.com/eduardsui/tlse
 * @param client_sock Socket al que se envian
 * @param context Contexto TLS
 * @return int Error si menor que 0
 */
int send_pending(int client_sock, struct TLSContext *context);

/**
 * @brief Digiere un mensaje tls del socket 
 * 
 * @param tls_context Contexto TLS
 * @param conn_fd descriptor de la conexion
 * @param buf Buffer destino
 * @param buf_len Tamaño maximo de buffer
 * @param flags flags de recv asociado
 * @return int bytes leido o menor que 0 si error
 */
int digest_tls(struct TLSContext *tls_context, int conn_fd, char *buf, ssize_t buf_len, int flags);

/**
 * @brief Recibe de manera segura un mensaje
 * 
 * @param tls_context Contexto TLS
 * @param conn_fd Descriptor de la conexion
 * @param buf Buffer destino
 * @param buf_len Tamaño maximo de buffer
 * @param flags Flags de recv
 * @return int Mayor que 0 si bytes leidos, 0 si se pierde conexion TLS, -1 si error
 */
int srecv(struct TLSContext *tls_context, int conn_fd, char *buf, ssize_t buf_len, int flags);

/**
 * @brief Envia de forma segura el contenido de un buffer
 * 
 * @param tls_context Contexto TLS
 * @param conn_fd Descriptor de la conexion
 * @param buf Buffer a enviar
 * @param buf_len Tamaño del buffer
 * @param flags Flags de send
 */
int ssend(struct TLSContext *tls_context, int conn_fd, char *buf, ssize_t buf_len, int flags);

/**
 * @brief Cierra un socket o conexion
 * 
 * @param tls_context Si se da, se entiende que es una conexion y se cierra
 * @param fd Debe ser mayor que 0
 * @return int menor que 0 si error
 */
int sclose(struct TLSContext **tls_context, int *fd);

/**
 * @brief Acepta a un nuevo cliente y realiza el handshake
 * 
 * @param gen_context Contexto TLS general
 * @param context Almacenara el contexto TLS de sesion
 * @param sock_fd Socket de escucha
 * @param expected_pkey Si no NULL, verifica que el certificado del cliente tiene una clave publica determinada
 *                      para su uso en la conexion TLS, si no, rechazar la peticion
 * @return int fd de la conexion o -1 si error
 */
int tls_accept_and_handshake(struct TLSContext *gen_context, struct TLSContext **context, int sock_fd, char *expected_pkey);

#endif /* RED_H */