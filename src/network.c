/**
 * @file network.c
 * @author Joaquin Jimenez Lopez de Castro (joaquin.jimenezl@estudiante.uam.es)
 * @brief Network library implementation
 * @version 1.0
 * @date 02-02-2020
 *
 *
 */
#define _DEFAULT_SOURCE
#include <syslog.h>
#include <ctype.h>
#include "network.h"

/**
 * @brief Read contents of a file into a buffer
 * https://github.com/eduardsui/tlse
 * @param fname Name of the file
 * @param buffer destination buffer
 * @param max_len Maximum buffer size
 * @return int less than or equal to 0 on error
 */
int read_from_file(const char *fname, char *buf, int max_len)
{
    FILE *f = fopen(fname, "rb");
    if (!f)
        return 0;
    int size = fread(buf, 1, max_len - 1, f);
    buf[size > 0 ? size : 0] = 0; /*end of string 0*/
    fclose(f);
    return size;
}

/**
 * @brief Load key and certificate from server to context
 * https://github.com/eduardsui/tlse
 * @param context TLS context
 * @param fname Name of the pem file with certificate
 * @param priv_fname Name of the pem file with private key
 *
 * @return int 1 if all ok, 0 if error
 */
int load_keys(struct TLSContext *context, char *fname, char *priv_fname)
{
    char buf[0xFFFF];
    char buf2[0xFFFF];
    int size = read_from_file(fname, buf, 0xFFFF);        /*Read the certificate*/
    int size2 = read_from_file(priv_fname, buf2, 0xFFFF); /*read private key*/
    if (size > 0 && context)
    {
        return tls_load_certificates(context, (unsigned char *)buf, size) > 0      /*Load certificate*/
               && tls_load_private_key(context, (unsigned char *)buf2, size2) > 0; /*Load private key*/
    }
    return 0;
}

/**
 * @brief Send bytes of TLS protocol that are pending to be sent
 * https://github.com/eduardsui/tlse
 * @param client_sock Socket to send to
 * @param context TLS context
 * @return int Error if less than 0
 */
int send_pending(int client_sock, struct TLSContext *context)
{
    unsigned int out_buffer_len = 0;
    const unsigned char *out_buffer = tls_get_write_buffer(context, &out_buffer_len); /*TLS bytes pending to be sent*/
    int send_res = 0;
    /*Normally send the bytes through the socket*/
    send_res = send(client_sock, (char *)out_buffer, out_buffer_len, MSG_NOSIGNAL);
    tls_buffer_clear(context);
    return send_res;
}

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
int digest_tls(struct TLSContext *tls_context, int conn_fd, char *buf, ssize_t buf_len, int flags)
{
    ssize_t read_b;
    if ((read_b = recv(conn_fd, buf, buf_len, flags)) > 0)
    {
        if (tls_consume_stream(tls_context, (unsigned char *)buf, read_b, NULL) < 0)
            return -1;
        if (send_pending(conn_fd, tls_context) < 0)
            return -1;
    }
    return read_b;
}

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
int srecv(struct TLSContext *tls_context, int conn_fd, char *buf, ssize_t buf_len, int flags)
{
    if (!tls_context)
        return recv(conn_fd, buf, buf_len, flags);
    char buf2[buf_len];
    if (digest_tls(tls_context, conn_fd, buf2, buf_len, flags) < 0) /*Get TLS message and fill structure fields*/
        return -1;
    if (tls_established(tls_context))                                /*Correct TLS connection status*/
        return tls_read(tls_context, (unsigned char *)buf, buf_len); /*Read to a decrypting buffer*/
    return 0;
}

/**
 * @brief Securely sends the contents of a buffer
 *
 * @param tls_context TLS context
 * @param conn_fd Connection descriptor
 * @param buf Buffer to send
 * @param buf_len Buffer size
 * @param flags send flags
 */
int ssend(struct TLSContext *tls_context, int conn_fd, char *buf, ssize_t buf_len, int flags)
{
    if (!tls_context)
        return send(conn_fd, buf, buf_len, flags);
    if (tls_write(tls_context, (unsigned char *)buf, buf_len) < 0)
        return -1;
    return send_pending(conn_fd, tls_context);
}

/**
 * @brief Closes a socket or connection
 *
 * @param tls_context If given, it is assumed to be a connection and is closed
 * @param fd Must be greater than 0
 * @return int less than 0 on error
 */
int sclose(struct TLSContext **tls_context, int *fd)
{
    if (!fd || *fd <= 0)
        return -1;
    if (tls_context && *tls_context) /*If it is connection closure*/
    {
        tls_close_notify(*tls_context);
        send_pending(*fd, *tls_context);
        tls_destroy_context(*tls_context);
        *tls_context = NULL;
    }
    close(*fd);
    *fd = -1;
    return 1;
}

/**
 * @brief Handshake TLS
 *
 * @param gen_context General TLS context
 * @param context TLS context to fill for the session
 * @param expected_pkey If not NULL, check that the public key of the certificate matches a certain one
 * @param conn_fd Connection created
 * @return int 1 if everything ok, 0 if client certificate failed, -1 if connection failed
 */
int tls_handshake(struct TLSContext *gen_context, struct TLSContext **context, char *expected_pkey, int conn_fd)
{
    char buf[XXXL_SZ];
    if (conn_fd < 0)
        return -1;
    *context = tls_accept(gen_context); /*Create a new context for the session*/
    if (!*context)
        return -1;
    tls_request_client_certificate(*context);                             /*We need client certificate*/
    digest_tls(*context, conn_fd, buf, XXXL_SZ, MSG_NOSIGNAL);            /*Send certificate request to client*/
    digest_tls(*context, conn_fd, buf, XXXL_SZ, MSG_NOSIGNAL);            /*Receive client certificate*/
    return tls_context_check_client_certificate(expected_pkey, *context); /*Check correct certificate*/
}

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
int tls_accept_and_handshake(struct TLSContext *gen_context, struct TLSContext **context, int sock_fd, char *expected_pkey)
{
    int conn_fd = -1, ret;
    do
    {
        if (conn_fd > 0) /*Release possible previous bad client*/
        {
            tls_close_notify(*context);
            tls_destroy_context(*context);
            close(conn_fd);
        }
        conn_fd = accept(sock_fd, NULL, 0);
        ret = tls_handshake(gen_context, context, expected_pkey, conn_fd);
        /*Do not accept certificate if the public key is not the expected one*/
    } while (!ret);
    if (ret < 0)
        return ret;
    return conn_fd;
}

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
int connect_and_handshake(struct TLSContext *gen_ctx, struct TLSContext **ctx, char *expected_pkey, int port, int clt_port, char *srv_ip, char *clt_ip)
{
    int conn_fd = -1, ret;
    do
    {
        if (conn_fd > 0) /*Release possible previous bad client*/
        {
            tls_close_notify(*ctx);
            tls_destroy_context(*ctx);
            close(conn_fd);
        }
        conn_fd = socket_clt_connection(clt_port, clt_ip, port, srv_ip); /*Connect to the server*/
        ret = tls_handshake(gen_ctx, ctx, expected_pkey, conn_fd);
        /*Do not accept certificate if the public key is not the expected one*/
    } while (!ret);
    if (ret < 0)
        return ret;
    return conn_fd;
}

/*Retrieved from https://stackoverflow.com/questions/4181784/how-to-set-socket-timeout-in-c-when-making-multiple-connections*/
/**
 * @brief Add options to a socket so that socket read and write operations have a maximum timeout
 *
 * @param socket_fd Socket to modify
 * @param seconds Number of timeout seconds
 */
int set_socket_timeouts(int socket_fd, int seconds)
{
    if (socket_fd < 0)
        return socket_fd;
    struct timeval timeout = {.tv_sec = seconds, .tv_usec = 0};
    return MIN(setsockopt(socket_fd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout)),
               setsockopt(socket_fd, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout, sizeof(timeout)));
}

int socket_proto(const char *proto_transp, struct sockaddr_in *sock_info, int puerto, char *ip);

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
int socket_srv(const char *proto_transp, int qlen, int puerto, char *ip_srv)
{
    struct sockaddr_in sock_info; /*Socket address information*/
    int socket_fd;                /*socket file descriptor*/

    /*Obtain socket fd of necessary specifications for the indicated protocol*/
    if ((socket_fd = socket_proto(proto_transp, &sock_info, puerto, ip_srv)) < 0)
        return socket_fd;

    /*We bind the socket to the information we have filled*/
    if (bind(socket_fd, (struct sockaddr *)&sock_info, sizeof(sock_info)) < 0)
        return -2;

    /*If the socket is not udp, the listen is not needed; we try to do it with CdE*/
    if ((!proto_transp || !strcmp(proto_transp, TCP)) && listen(socket_fd, qlen) < 0)
        return -3;

    return socket_fd;
}

/**
 * @brief Opens a client socket and connects to a server
 *
 * @param clt_port Client port
 * @param ip_clt Client IP
 * @param srv_port Server port
 * @param ip_srv IP of the server
 * @return int Less than 0 on error
 */
int socket_clt_connection(int puerto_clt, char *ip_clt, int puerto_srv, char *ip_srv)
{
    int socket_fd = socket_clt("tcp", ip_clt, puerto_clt);
    if (socket_fd < 0)
        return socket_fd;
    return socket_clt_connect(socket_fd, ip_srv, puerto_srv);
}

/**
 * @brief Create a client socket and bind on a specified port
 *
 * @param proto_transp Protocol to use: tcp or udp
 * @param port Port of the client, put 0 if you do not want a specific one
 * @param ip_clt Ip of the client in ascii
 * @return int
 */
int socket_clt(const char *proto_transp, char *ip_clt, int puerto)
{
    struct sockaddr_in sock_info; /*Information about destination socket addresses*/
    int socket_fd;

    /*Create a socket to connect (or not) to the server*/
    socket_fd = socket_proto(proto_transp, &sock_info, puerto, ip_clt);

    /*Perform a bind on the specified port*/
    if (puerto == 0)
        return 1;
    if (bind(socket_fd, (struct sockaddr *)&sock_info, sizeof(sock_info)) < 0)
        return -2;

    return socket_fd;
}

/**
 * @brief Connect a client socket to a server socket from its ip and port number
 *
 * @param socket_fd Client socket file descriptor
 * @param ip_srv IP address of the server socket, format string "w.x.y.z"
 * @param srv_port Server port number
 * @return int socket or less than 0 for error
 */
int socket_clt_connect(int socket_fd, char *ip_srv, int puerto_srv)
{
    struct sockaddr_in sock_info;

    /*Initialize address and port structure*/
    bzero(&sock_info, sizeof(struct sockaddr_in)); /*Padding with 0s, equivalent to memset*/
    sock_info.sin_family = AF_INET;                /*Internet domain*/
    sock_info.sin_port = htons(puerto_srv);        /*htons() convert to network format*/
    sock_info.sin_addr.s_addr = inet_addr(ip_srv); /*IP address of the server*/

    /*Connect to the server*/
    if (connect(socket_fd, (struct sockaddr *)&sock_info, sizeof(sock_info)) < 0)
        return -4;
    return socket_fd;
}

/**
 * @brief Creates a socket with the specifications of the indicated protocol: "udp" or "tcp"
 *
 * @param proto_transp Protocol name
 * @param sock_info Structure where to store socket info
 * @param port Port of origin or destination as appropriate
 * @param ip Ip to put in sock_info
 * @return int socket file descriptor
 */
int socket_proto(const char *proto_transp, struct sockaddr_in *sock_info, int puerto, char *ip)
{
    struct protoent *ppe; /*Protocol information*/
    int proto_num;        /*Number associated with the protocol*/
    int tipo_socket;      /*Socket type: stream, datagrams...*/
    int socket_fd;        /*socket file descriptor*/

    /*We get the number associated with the protocol passed as argument*/
    if (!proto_transp || (ppe = getprotobyname(proto_transp)) == 0)
        proto_num = 0;
    else
        proto_num = ppe->p_proto;

    /*We choose socket type based on protocol*/
    if (!proto_num || strcmp(proto_transp, UDP))
        tipo_socket = SOCK_STREAM;
    else
        tipo_socket = SOCK_DGRAM;

    /*Open the socket which, like everything in POSIX, is a file, represented by a descriptor*/
    if ((socket_fd = socket(AF_INET, tipo_socket, proto_num)) < 0)
        return socket_fd;

    if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) < 0)
        return -2;

    /*Initialize address and port structure*/
    bzero(sock_info, sizeof(struct sockaddr_in)); /*Padding with 0s, equivalent to memset*/
    sock_info->sin_family = AF_INET;              /*Internet domain*/
    sock_info->sin_port = htons(puerto);          /*htons() convert to network format*/
    sock_info->sin_addr.s_addr = inet_addr(ip);   /*IP adress*/

    return socket_fd;
}

/**
 * @brief Write the contents of the src_fd file to a socket
 *
 * @param client_fd socket descriptor
 * @param src_fd source file descriptor
 * @return int number of bytes written
 */
int socket_dump_fd(int client_fd, int src_fd)
{
    size_t tam = file_size(src_fd);
    /*send the whole file*/
    return sendfile(client_fd, src_fd, 0, tam);
}