/**
 * @file red.c
 * @author Joaquin Jimenez Lopez de Castro (joaquin.jimenezl@estudiante.uam.es)
 * @brief Implementacion de la libreria red
 * @version 1.0
 * @date 02-02-2020
 * 
 * 
 */
#define _DEFAULT_SOURCE
#include <syslog.h>
#include <ctype.h>
#include "red.h"

/**
 * @brief Lee contenido de un fichero a un buffer
 * https://github.com/eduardsui/tlse
 * @param fname Nombre del fichero
 * @param buf Buffer destino
 * @param max_len Tamaño maximo de buffer
 * @return int menor o igual que 0 si error
 */
int read_from_file(const char *fname, char *buf, int max_len) 
{
    FILE *f = fopen(fname, "rb");
    if ( !f )
        return 0;
    int size = fread(buf, 1, max_len - 1, f);
    buf[size > 0 ? size : 0] = 0; /* 0 de fin de cadena */
    fclose(f);
    return size;
}

/**
 * @brief Carga clave y certificado del servidor al contexto
 * https://github.com/eduardsui/tlse
 * @param context Contexto TLS
 * @param fname Nombre del fichero pem con certificado
 * @param priv_fname Nombre del fichero pem con clave privada
 * 
 * @return int 1 si todo ok, 0 si error
 */
int load_keys(struct TLSContext *context, char *fname, char *priv_fname) 
{
    char buf[0xFFFF];
    char buf2[0xFFFF];
    int size = read_from_file(fname, buf, 0xFFFF); /* Lee el certificado */
    int size2 = read_from_file(priv_fname, buf2, 0xFFFF); /* Lee clave privada */
    if (size > 0 && context) 
    {
        return tls_load_certificates(context, (unsigned char *) buf, size) > 0 /* Carga certificado */
               && tls_load_private_key(context, (unsigned char *) buf2, size2) > 0; /* Carga clave privada */
    }
    return 0;
}

/**
 * @brief Manda bytes de protocolo TLS que quedan pendientes de envio
 * https://github.com/eduardsui/tlse
 * @param client_sock Socket al que se envian
 * @param context Contexto TLS
 * @return int Error si menor que 0
 */
int send_pending(int client_sock, struct TLSContext *context) 
{
    unsigned int out_buffer_len = 0;
    const unsigned char *out_buffer = tls_get_write_buffer(context, &out_buffer_len); /* Bytes TLS pendientes de envio */
    int send_res = 0;
    /* Envia normalmente los bytes por el socket */
    send_res = send(client_sock, (char *)out_buffer, out_buffer_len, MSG_NOSIGNAL);
    tls_buffer_clear(context);
    return send_res;
}

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
int digest_tls(struct TLSContext *tls_context, int conn_fd, char *buf, ssize_t buf_len, int flags)
{
    ssize_t read_b;
    if ( (read_b = recv(conn_fd, buf, buf_len, flags)) > 0 )
    {
        if ( tls_consume_stream(tls_context, (unsigned char *) buf, read_b, NULL) < 0 )
            return -1;
        if ( send_pending(conn_fd, tls_context) < 0 )
            return -1;
    }
    return read_b;
}

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
int srecv(struct TLSContext *tls_context, int conn_fd, char *buf, ssize_t buf_len, int flags)
{
    if ( !tls_context )
        return recv(conn_fd, buf, buf_len, flags);
    char buf2[buf_len];
    if ( digest_tls(tls_context, conn_fd, buf2, buf_len, flags) < 0 ) /* Recoger mensaje TLS y rellenar campos de estructura */
        return -1;
    if ( tls_established(tls_context) ) /* Correcto estado de conexion TLS */
        return tls_read(tls_context, (unsigned char *) buf, buf_len); /* Leer a un buffer descifrando */
    return 0;
}

/**
 * @brief Envia de forma segura el contenido de un buffer
 * 
 * @param tls_context Contexto TLS
 * @param conn_fd Descriptor de la conexion
 * @param buf Buffer a enviar
 * @param buf_len Tamaño del buffer
 * @param flags Flags de send
 */
int ssend(struct TLSContext *tls_context, int conn_fd, char *buf, ssize_t buf_len, int flags)
{
    if ( !tls_context )
        return send(conn_fd, buf, buf_len, flags);
    if ( tls_write(tls_context, (unsigned char *) buf, buf_len) < 0)
        return -1;
    return send_pending(conn_fd, tls_context);
}

/**
 * @brief Cierra un socket o conexion
 * 
 * @param tls_context Si se da, se entiende que es una conexion y se cierra
 * @param fd Debe ser mayor que 0
 * @return int menor que 0 si error
 */
int sclose(struct TLSContext **tls_context, int *fd)
{
    if ( !fd || *fd <= 0 )
        return -1;
    if ( tls_context && *tls_context ) /* Si es cierre de conexion */
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
 * @brief Acepta a un nuevo cliente y realiza el handshake
 * 
 * @param gen_context Contexto TLS general
 * @param context Almacenara el contexto TLS de sesion
 * @param sock_fd Socket de escucha
 * @param expected_pkey Si no NULL, verifica que el certificado del cliente tiene una clave publica determinada
 *                      para su uso en la conexion TLS, si no, rechazar la peticion
 * @return int fd de la conexion o -1 si error
 */
int tls_accept_and_handshake(struct TLSContext *gen_context, struct TLSContext **context, int sock_fd, char *expected_pkey)
{
    int conn_fd = -1;
    char buf[XXXL_SZ];
    do
    {
        if ( conn_fd > 0 ) /* Liberar posible cliente erroneo anterior */
        {
            tls_close_notify(*context);
            tls_destroy_context(*context);
            close(conn_fd);
        }
        conn_fd = accept(sock_fd, NULL, 0);
        if ( conn_fd < 0 )
            return -1;
        *context = tls_accept(gen_context); /* Crear un nuevo contexto para la sesion */
        if ( !*context )
            return -1;
        tls_request_client_certificate(*context); /* Necesitamos certificado del cliente */
        digest_tls(*context, conn_fd, buf, XXXL_SZ, MSG_NOSIGNAL); /* Envia peticion de certificado al cliente */
        digest_tls(*context, conn_fd, buf, XXXL_SZ, MSG_NOSIGNAL); /* Recibe certificado del cliente */
        /* No aceptar certificado si la clave publica no es la esperada */
    } while ( !tls_context_check_client_certificate(expected_pkey, *context) );
    return conn_fd;
}

/* Obtenido de https://stackoverflow.com/questions/4181784/how-to-set-socket-timeout-in-c-when-making-multiple-connections */
/**
 * @brief Añade opciones a un socket para que las operaciones de escritura y lectura de un socket tengan un timeout maximo
 * 
 * @param socket_fd Socket a modificar
 * @param seconds Numero de segundos de timeout
 */
int set_socket_timeouts(int socket_fd, int seconds)
{
    struct timeval timeout = {.tv_sec = seconds, .tv_usec = 0};
    return MIN(setsockopt(socket_fd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout)),
               setsockopt(socket_fd, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout, sizeof(timeout)));
}

int socket_proto(const char *proto_transp, struct sockaddr_in *sock_info, int puerto, char *ip);

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
int socket_srv(const char *proto_transp, int qlen, int puerto, char *ip_srv)
{
    struct sockaddr_in sock_info;       /* Informacion sobre direcciones del socket */
    int socket_fd;                      /* Descriptor de fichero del socket */

    /* Obtiene fd del socket de especificaciones necesarias para el protocolo indicado */
    if ( (socket_fd = socket_proto(proto_transp, &sock_info, puerto, ip_srv)) < 0 )
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
 * @brief Abre un socket de cliente y se conecta a un servidor
 * 
 * @param puerto_clt Puerto del cliente
 * @param ip_clt Ip del cliente
 * @param puerto_srv Puerto del servidor
 * @param ip_srv Ip del servidor
 * @return int Menor que 0 si error
 */
int socket_clt_connection(int puerto_clt, char *ip_clt, int puerto_srv, char *ip_srv)
{
    int socket_fd = socket_clt("tcp", ip_clt, puerto_clt);
    if ( socket_fd < 0 )
        return socket_fd;
    return socket_clt_connect(socket_fd, ip_srv, puerto_srv);
}

/**
 * @brief Crea un socket de cliente y hace un bind en un puerto especifico
 * 
 * @param proto_transp Protocolo a utilizar: tcp o udp
 * @param puerto Puerto del cliente, poner 0 si no se quiere uno en concretos
 * @param ip_clt Ip del cliente en ascii
 * @return int 
 */
int socket_clt(const char *proto_transp, char *ip_clt, int puerto)
{
    struct sockaddr_in sock_info;       /* Informacion sobre direcciones del socket destino */
    int socket_fd;

    /* Crea un socket para conectarse (o no) al servidor */
    socket_fd = socket_proto(proto_transp, &sock_info, puerto, ip_clt);

    /* Realizar un bind en el puerto especificado */
    if ( puerto == 0 )
        return 1;
    if ( bind(socket_fd, (struct sockaddr *)&sock_info, sizeof(sock_info)) < 0 )
        return -2;

    return socket_fd;
}

/**
 * @brief Conecta un socket de client a un socket de servidor a partir de su ip y numero de puerto
 * 
 * @param socket_fd Descriptor de fichero del socket cliente
 * @param ip_srv Direccion ip del socket servidor, cadena de formato "w.x.y.z"
 * @param puerto_srv Numero de puerto del servidor
 * @return int socket o menor que 0 para error
 */
int socket_clt_connect(int socket_fd, char *ip_srv, int puerto_srv)
{
    struct sockaddr_in sock_info;

    /* Inicializamos estructura de dirección y puerto */
	bzero(&sock_info, sizeof(struct sockaddr_in));   /* Rellena de 0s, equivalente a memset */
	sock_info.sin_family = AF_INET;                  /* Dominio internet */
	sock_info.sin_port = htons(puerto_srv);          /* htons() convierte a formato de red */
	sock_info.sin_addr.s_addr = inet_addr(ip_srv);   /* direccion IP del servidor */

    /* Conectarse al servidor */ 
    if ( connect(socket_fd, (struct sockaddr *) &sock_info, sizeof(sock_info)) < 0 )
        return -4;
    return socket_fd;
}

/**
 * @brief Crea un socket con las especificaciones propias del protocolo indicado: "udp" o "tcp"
 * 
 * @param proto_transp Nombre del protocolo
 * @param sock_info Estructura donde almacenar info de socket
 * @param puerto Puerto origen o destino segun corresponda
 * @param ip Ip a poner en sock_info
 * @return int Descriptor de fichero del socket
 */
int socket_proto(const char *proto_transp, struct sockaddr_in *sock_info, int puerto, char *ip)
{
    struct protoent *ppe;               /* Informacion de protocolo */
    int proto_num;                      /* Numero asociado al protocolo */
    int tipo_socket;                    /* Tipo de socket: stream, datagramas... */
    int socket_fd;                      /* Descriptor de fichero del socket */

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

    /* Inicializamos estructura de dirección y puerto */
	bzero(sock_info, sizeof(struct sockaddr_in));   /* Rellena de 0s, equivalente a memset */
	sock_info->sin_family = AF_INET;                /* Dominio internet */
	sock_info->sin_port = htons(puerto);            /* htons() convierte a formato de red */
	sock_info->sin_addr.s_addr = inet_addr(ip);     /* direccion IP */

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