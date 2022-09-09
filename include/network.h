/**
 * @file network.h
 * @author Joaquin Jimenez Lopez de Castro (joaquin.jimenezl@estudiante.uam.es)
 * @brief Declarations and inclusion of useful libraries for network programming in C in a UNIX environment
 * @version 1.0
 * @date 02-01-2020
 *
 */

#ifndef RED_H
#define RED_H
#ifndef _GNU_SOURCE
#define _GNU_SOURCE /*!< Allow memfd_create to be used*/
#endif
#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE /*!< Structures for sockets*/
#endif

#include <sys/socket.h>   /*socket functions*/
#include <sys/sendfile.h> /*In order to send a file*/
#include <resolv.h>       /*man resolver (3)*/
#include <arpa/inet.h>    /*man inet(3)*/
#include <netinet/tcp.h>  /*Tcp cork*/
#include <netdb.h>        /*Allows a protocol to be identified by name*/
#include "utils.h"
#include "tlse.h"

/*Strings associated with protocols*/
#define TCP "tcp" /*!< tcp protocol*/
#define UDP "udp" /*!< Protocol udp*/
/******************************
MANIPULATION AND CREATION OF SOCKETS
*******************************/
/*Retrieved from https://stackoverflow.com/questions/4181784/how-to-set-socket-timeout-in-c-when-making-multiple-connections*/
/**
 * @brief Add options to a socket so that socket read and write operations have a maximum timeout
 *
 * @param socket_fd Socket to modify
 * @param seconds Number of seconds to timeout
 */
int set_socket_timeouts(int socket_fd, int seconds);

/**
 * @brief Create a socket for a server given the protocol to use, port and the queue buffer size.
 * Performs the bind (and the listen if applicable).
 * @details Based on: https://www.tenouk.com/Module40a.html
 * protoent info: https://linux.die.net/man/3/getprotobyname
 * info about sockaddr: https://www.gta.ufrj.br/ensino/eel878/sockets/sockaddr_inman.html
 * info about socket(): man socket
 * info about bind(): man 2 bind
 * info about listen(): man 2 listen
 *
 * @param proto_transp String representing the name of the protocol: eg "udp", "tcp"... If NULL or incorrect, use TCP
 * @param qlen Socket queue buffer size
 * @param port Port number where the socket will be opened
 * @param ip_srv IP of the server
 * @return int (file descriptor of the created socket)
 */
int socket_srv(const char *proto_transp, int qlen, int puerto, char *ip_srv);

/**
 * @brief Create a client socket and bind on a specified port
 *
 * @param proto_transp Protocol to use: tcp or udp
 * @param port Port of the client, put 0 if you do not want a specific one
 * @param ip_clt Ip of the client in ascii
 * @return int
 */
int socket_clt(const char *proto_transp, char *ip_clt, int puerto);

/**
 * @brief Connect a client socket to a server socket from its ip and port number
 *
 * @param socket_fd Client socket file descriptor
 * @param ip_srv IP address of the server socket, format string "w.x.y.z"
 * @param srv_port Server port number
 * @return int
 */
int socket_clt_connect(int socket_fd, char *ip_srv, int puerto_srv);

/**
 * @brief Opens a client socket and connects to a server
 *
 * @param clt_port Client port
 * @param ip_clt Client IP
 * @param srv_port Server port
 * @param ip_srv IP of the server
 * @return int Less than 0 on error
 */
int socket_clt_connection(int puerto_clt, char *ip_clt, int puerto_srv, char *ip_srv);

/**
 * @brief Write the contents of the src_fd file to a socket
 *
 * @param client_fd socket descriptor
 * @param src_fd source file descriptor
 * @return int number of bytes written
 */
int socket_dump_fd(int socket_fd, int src_fd);

/**
 * @brief Read contents of a file into a buffer
 * https://github.com/eduardsui/tlse
 * @param fname Name of the file
 * @param buffer destination buffer
 * @param max_len Maximum buffer size
 * @return int less than or equal to 0 on error
 */
int read_from_file(const char *fname, char *buf, int max_len);

/**
 * @brief Load key and certificate from server to context
 * https://github.com/eduardsui/tlse
 * @param context TLS context
 * @param fname Name of the pem file with certificate
 * @param priv_fname Name of the pem file with private key
 *
 * @return int 1 if all ok, 0 if error
 */
int load_keys(struct TLSContext *context, char *fname, char *priv_fname);

/**
 * @brief Send bytes of TLS protocol that are pending to be sent
 * https://github.com/eduardsui/tlse
 * @param client_sock Socket to send to
 * @param context TLS context
 * @return int Error if less than 0
 */
int send_pending(int client_sock, struct TLSContext *context);

/**
 * @brief Digest a tls message from the socket
 *
 * @param tls_context TLS context
 * @param conn_fd connection descriptor
 * @param buffer destination buffer
 * @param buf_len maximum buffer size
 * @param flags associated recv flags
 * @return int bytes read or less than 0 if error
 */
int digest_tls(struct TLSContext *tls_context, int conn_fd, char *buf, ssize_t buf_len, int flags);

/**
 * @brief Securely receive a message
 *
 * @param tls_context TLS context
 * @param conn_fd Connection descriptor
 * @param buffer destination buffer
 * @param buf_len maximum buffer size
 * @param flags recv flags
 * @return int Greater than 0 if bytes read, 0 if TLS connection lost, -1 if error
 */
int srecv(struct TLSContext *tls_context, int conn_fd, char *buf, ssize_t buf_len, int flags);

/**
 * @brief Securely sends the contents of a buffer
 *
 * @param tls_context TLS context
 * @param conn_fd Connection descriptor
 * @param buf Buffer to send
 * @param buf_len Buffer size
 * @param flags send flags
 */
int ssend(struct TLSContext *tls_context, int conn_fd, char *buf, ssize_t buf_len, int flags);

/**
 * @brief Closes a socket or connection
 *
 * @param tls_context If given, it is assumed to be a connection and is closed
 * @param fd Must be greater than 0
 * @return int less than 0 on error
 */
int sclose(struct TLSContext **tls_context, int *fd);

/**
 * @brief Accept a new client and perform the handshake
 *
 * @param gen_context General TLS context
 * @param context will store the TLS session context
 * @param sock_fd Listening socket
 * @param expected_pkey If not NULL, verify that the client certificate has a given public key
 * for use in the TLS connection, if not, reject the request
 * @return int fd of the connection or -1 if error
 */
int tls_accept_and_handshake(struct TLSContext *gen_context, struct TLSContext **context, int sock_fd, char *expected_pkey);

/**
 * @brief Connect securely to a server
 *
 * @param gen_ctx General TLS context
 * @param ctx TLS context to fill
 * @param expected_pkey Public key expected from the client certificate
 * @param port Server port
 * @param clt_port Client port
 * @param srv_ip Server IP
 * @param clt_ip Client IP
 * @return int 1 if all ok, -1 if error
 */
int connect_and_handshake(struct TLSContext *gen_ctx, struct TLSContext **ctx, char *expected_pkey, int port, int clt_port, char *srv_ip, char *clt_ip);

#endif /*RED_H*/