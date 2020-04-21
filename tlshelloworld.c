#include "../tlse.c"

#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>

/**
 * @brief Lee contenido de un fichero a un buffer
 * 
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
 * 
 * @param context Contexto TLS
 * @param fname Nombre del fichero pem con certificado
 * @param priv_fname Nombre del fichero pem con clave privada
 */
void load_keys(struct TLSContext *context, char *fname, char *priv_fname) 
{
    unsigned char buf[0xFFFF];
    unsigned char buf2[0xFFFF];
    int size = read_from_file(fname, buf, 0xFFFF);
    int size2 = read_from_file(priv_fname, buf2, 0xFFFF);
    if (size > 0 && context) 
    {
        tls_load_certificates(context, buf, size);
        tls_load_private_key(context, buf2, size2);
    }
}

/**
 * @brief Manda bytes de protocolo TLS que quedan pendientes de envio
 * 
 * @param client_sock Socket al que se envian
 * @param context Contexto TLS
 * @return int Error si menor que 0
 */
int send_pending(int client_sock, struct TLSContext *context) 
{
    unsigned int out_buffer_len = 0;
    const unsigned char *out_buffer = tls_get_write_buffer(context, &out_buffer_len); /* Bytes TLS pendientes de envio */
    unsigned int out_buffer_index = 0;
    int send_res = 0;
    /* Envia normalmente los bytes por el socket */
    send_res = send(client_sock, out_buffer, out_buffer_len, MSG_NOSIGNAL);
    tls_buffer_clear(context);
    return send_res;
}

int main(int argc , char *argv[]) {
    int socket_desc , client_sock , read_size;
    socklen_t c;
    struct sockaddr_in server , client;
    char client_message[0xFFFF];

    socket_desc = socket(AF_INET , SOCK_STREAM , 0);
    if (socket_desc == -1) {
        printf("Could not create socket");
        return 0;
    }

    int port = 2000;
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(port);
     
    setsockopt(socket_desc, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int));

    if( bind(socket_desc,(struct sockaddr *)&server , sizeof(server)) < 0) 
    {
        perror("bind failed. Error");
        return 1;
    }
     
    listen(socket_desc, 3);
     
    c = sizeof(struct sockaddr_in);

    tls_init();
    struct TLSContext *server_context = tls_create_context(1, TLS_V12);
    load_keys(server_context, "testcert/fullchain.pem", "testcert/privkey.pem");
    
    int x = 2;
    while (x--) /* Accept loop */
    {
        client_sock = accept(socket_desc, (struct sockaddr *)&client, &c);
        if (client_sock < 0) 
        {
            perror("accept failed");
            return 1;
        }
        struct TLSContext *context = tls_accept(server_context);

        tls_request_client_certificate(context);

        fprintf(stderr, "Client connected\n");
        while ((read_size = recv(client_sock, client_message, sizeof(client_message), 0)) > 0) 
            if (tls_consume_stream(context, client_message, read_size, NULL) > 0)
                break;

        send_pending(client_sock, context);

        while ((read_size = recv(client_sock, client_message, sizeof(client_message), 0)) > 0) 
        {
            if (tls_consume_stream(context, client_message, read_size, NULL) < 0) 
            {
                fprintf(stderr, "Error in stream consume\n");
                break;
            }
            send_pending(client_sock, context);
            if (tls_established(context) == 1) 
            {
                unsigned char read_buffer[0xFFFF];
                int read_size = tls_read(context, read_buffer, sizeof(read_buffer) - 1);
                read_buffer[read_size] = '\0';
                printf("%s\n", read_buffer);
                if (read_size > 0) 
                {
                    tls_write(context, "200 Hola\r\n", sizeof("200 Hola\r\n"));
                    tls_close_notify(context);
                    send_pending(client_sock, context);
                    break;
                }
            }
        }
        shutdown(client_sock, SHUT_RDWR);
        close(client_sock);
        tls_destroy_context(context);
    }
    tls_destroy_context(server_context);
    return 0;
}
