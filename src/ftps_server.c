/**
 * @file ftps_server.c
 * @author Joaquín Jiménez López de Castro (joaquin.jimenezl@estudiante.uam.es)
 * @brief FTP server implementation
 * @version 1.0
 * @date 07-04-2020
 *
 *
 */
#define _DEFAULT_SOURCE         /*!< Access to GNU functions*/
#define _POSIX_C_SOURCE 200112L /*!< Access to POSIX functions*/
#include "utils.h"
#include "network.h"
#include "ftp.h"
#include "authenticate.h"
#include "callbacks.h"
#include "ftp_session.h"
#include "config_parser.h"
#include "ftp_files.h"

#define MAX_PASSWORD MEDIUM_SZ            /*!< Maximum password size*/
#define USING_AUTHBIND "--using-authbind" /*!< Indicates current execution with authbind*/
#define THREAD_CLOSE_WAIT 2               /*!< Maximum time to wait for a thread to close*/
#define CONTROL_SOCKET_TIMEOUT 150        /*!< Maximum timeout of control connection*/
//#define DEBUG

void accept_loop(int socket_control_fd);
void set_ftp_credentials();
void *ftp_session_loop(void *args);
void set_end_flag(int sig);
void set_handlers();
void data_callback_loop(session_info *session, request_info *ri, serverconf *server_conf, char *buf);
void tls_start();

serverconf server_conf; /*!< Global server configuration*/
sem_t n_clients;        /*!< Controls that the number of FTP sessions is not exceeded*/
int end = 0;            /*!< Indicates that the program must be terminated*/
/**
 * @brief Application entry point
 *
 * @param argc Number of arguments
 * @param argv The special argument USING_AUTHBIND is used to indicate that the execution is performed with authbind
 * @return int success if 0
 */
int main(int argc, char *argv[])
{
    /*A bit crude check that the execution must be with authbind*/
#ifndef DEBUG
    if (!(argc == 2 && !strcmp(argv[1], USING_AUTHBIND)))
        execlp("authbind", "authbind", argv[0], USING_AUTHBIND, NULL);
#endif

    /*Read the server configuration*/
    if (parse_server_conf(&server_conf) < 0)
        errexit("Fallo al procesar fichero de configuracion\n");

    /*Set server root path*/
    set_root_path(server_conf.server_root);

    /*Set server credentials and remove root permissions if given*/
    set_ftp_credentials();

    /*Set signal handlers*/
    set_handlers();

    /*Now, we would have to create the control socket*/
    int socket_control_fd = socket_srv("tcp", 10, FTP_CONTROL_PORT, server_conf.ftp_host);
    if (socket_control_fd < 0)
        errexit("Fallo al abrir socket de control %s\n", strerror(errno));
    set_socket_timeouts(socket_control_fd, CONTROL_SOCKET_TIMEOUT);

    /*Initialize the TLS context*/
    tls_start();

    /*Set login configuration*/
    set_log_conf(!server_conf.daemon_mode, server_conf.daemon_mode, "Servidor FTPS");

    printf("Configuracion terminada, servidor desplegado\n");

    /*Enter daemon mode if specified*/
    if (server_conf.daemon_mode)
        daemon(1, 0);

    /*Request acceptance loop in control*/
    accept_loop(socket_control_fd);

    /*Release resources*/
    sclose(&(server_conf.server_ctx), &socket_control_fd);
    exit(0);
}

/**
 * @brief Loop of acceptance of new connections
 *
 * @param socket_control_fd Socket of incoming control connections
 */
void accept_loop(int socket_control_fd)
{
    struct sockaddr clt_info;
    size_t clt_info_size = sizeof(clt_info);
    intptr_t clt_fd;
    pthread_t session_thread;
    struct timespec timeout;

    /*Set maximum number of clients*/
    sem_init(&n_clients, 0, server_conf.max_sessions);
    /*Perform accepts until the server is closed*/
    while (!end)
    {
        /*Wait for a free slot and accept the next client*/
        sem_wait(&n_clients);
        clt_fd = accept(socket_control_fd, &clt_info, (socklen_t *)&clt_info_size);
        if (end)
        {
            sem_post(&n_clients);
            break;
        }
        /*Greet the client*/
        send(clt_fd, CODE_220_WELCOME_MSG, sizeof(CODE_220_WELCOME_MSG) - 1, 0);
        /*Open each new request in a thread to start the session*/
        if (end)
        {
            sem_post(&n_clients);
            break;
        }
        session_thread = pthread_create(&session_thread, NULL, ftp_session_loop, (void *)clt_fd);
        pthread_detach(session_thread); /*Convert to independent thread so you don't have to join*/
    }
    /*Wait for all threads to finish, with a certain timeout*/
    clock_gettime(CLOCK_REALTIME, &timeout);
    timeout.tv_sec += THREAD_CLOSE_WAIT;
    for (int i = 0; i < server_conf.max_sessions; i++)
        if (sem_timedwait(&n_clients, &timeout) == -1)
            break;
    return;
}

/**
 * @brief Main loop for reception of FTP commands
 *
 * @param args Contains client descriptor
 * @return void*
 */
void *ftp_session_loop(void *args)
{
    char buff[XXXL_SZ + 1];
    session_info s1, s2, *current = &s1, *previous = &s2, *aux;
    request_info ri = {.command_arg = "", .command_name = "", .response = "", .response_len = 0, .implemented_command = NOOP};
    intptr_t clt_fd = (intptr_t)args, cb_ret = CALLBACK_RET_PROCEED;
    data_conn dc = {.socket_fd = -1, .conn_state = DATA_CONN_CLOSED, .abort = 0, .conn_fd = -1};
    ssize_t read_b;

    /*FTP session: constant values*/
    sem_init(&(dc.mutex), 0, 1);            /*Control concurrent access to the structure*/
    sem_init(&(dc.data_conn_sem), 0, 0);    /*If set to 1, data callback has already finished data callback preparations*/
    sem_init(&(dc.control_conn_sem), 0, 0); /*If 1, Data Callback has finished transmission and indicated return code*/
    s1.data_connection = s2.data_connection = &dc;
    s1.clt_fd = s2.clt_fd = clt_fd;

    /*FTP session: variable attributes*/
    init_session_info(current, NULL);
    strcpy(current->current_dir, server_conf.server_root);
    current->ascii_mode = server_conf.default_ascii;

    /*Main session loop*/
    while (!end && cb_ret != CALLBACK_RET_END_CONNECTION)
    {
        /*Fetch next command checking the flag from time to time*/
        while (!end && (((read_b = srecv(current->context, clt_fd, buff, XXXL_SZ, MSG_DONTWAIT | MSG_NOSIGNAL)) < 0) && (errno == EWOULDBLOCK || errno == EAGAIN)) && !usleep(1000))
            ;
        if (end || read_b <= 0)
            break;
        buff[read_b] = '\0'; /*For security reasons we ensure a zero*/
        parse_ftp_command(&ri, buff);
#ifdef DEBUG
        flog(LOG_DEBUG, "%s %s\n", ri.command_name, (!strcmp(ri.command_name, "PASS")) ? "XXXX" : ri.command_arg);
#endif
        if (ri.ignored_command == -1 && ri.implemented_command == -1) /*Unrecognized command*/
            ssend(current->context, clt_fd, CODE_500_UNKNOWN_CMD, sizeof(CODE_500_UNKNOWN_CMD) - 1, MSG_NOSIGNAL);
        else if (ri.implemented_command == -1) /*Command recognized but not implemented*/
            ssend(current->context, clt_fd, CODE_502_NOT_IMP_CMD, sizeof(CODE_502_NOT_IMP_CMD) - 1, MSG_NOSIGNAL);
        else /*Command implemented, call the callback and return response controlling the possible data connection*/
        {
            cb_ret = command_callback(&server_conf, current, &ri);
            /*If it is a data transmission we enter a different loop*/
            if (DATA_CALLBACK(ri.implemented_command) && cb_ret != CALLBACK_RET_END_CONNECTION)
                data_callback_loop(current, &ri, &server_conf, buff);
            /*Send final response from callback*/
            if (cb_ret == CALLBACK_RET_PROCEED || ri.implemented_command == QUIT)
            {
                ssend(current->context, clt_fd, ri.response, ri.response_len, ((cb_ret != CALLBACK_RET_PROCEED) & MSG_DONTWAIT) | MSG_NOSIGNAL);
#ifdef DEBUG
                flog(LOG_DEBUG, "-->%s\n", ri.response);
#endif
            }
            aux = previous;
            previous = current;
            current = aux;                        /*Current session becomes the previous session*/
            init_session_info(current, previous); /*We fill it from the previous one removing attributes that expire*/
        }
    }
    /*Release the session attributes and close the connection*/
    sclose(&(current->context), &(current->clt_fd));
    free_attributes(current);
    sem_post(&n_clients);
    return NULL;
}

/**
 * @brief Waits for information from the data connection thread
 * and possible new control requests, of which only abort requests will be accepted
 * @param session Contains session information
 * @param server_conf Contains port semaphore in passive mode
 * @param ri It is updated with the responses of the data thread
 * @param buf Buffer to receive data
 */
void data_callback_loop(session_info *session, request_info *ri, serverconf *server_conf, char *buf)
{
    if (session->data_connection->conn_state == DATA_CONN_CLOSED) /*A data connection must have been initiated with PORT OR PASV*/
    {
        ssend(session->context, session->clt_fd, CODE_503_BAD_SEQUENCE, sizeof(CODE_503_BAD_SEQUENCE) - 1, MSG_DONTWAIT | MSG_NOSIGNAL);
        return;
    }
    /*When this semaphore is advanced, the initial shipping code 150 will have been filled in the client*/
    sem_wait(&(session->data_connection->data_conn_sem));
    /*We send the response code*/
    ssend(session->context, session->clt_fd, ri->response, ri->response_len, MSG_DONTWAIT | MSG_NOSIGNAL);
    /*Wait for the data thread to start transmitting*/
    sem_post(&(session->data_connection->control_conn_sem));
    sem_wait(&(session->data_connection->data_conn_sem));
    /*Wait for the data thread to finish transmitting*/
    while (sem_trywait(&(session->data_connection->data_conn_sem)) == -1)
    {
        /*Check if a new request has arrived*/
        ssize_t len = srecv(session->context, session->clt_fd, buf, XXXL_SZ + 1, MSG_DONTWAIT | MSG_NOSIGNAL);
        if (len != -1)
        {
            /*If it is ABORT, we activate the abort flag (atomic)*/
            if (len >= sizeof("ABORT") && !memcmp(buf, "ABORT", sizeof("ABORT") - 1))
                session->data_connection->abort = 1;
            /*Ignore all other requests*/
            else
                ssend(session->context, session->clt_fd, CODE_421_BUSY_DATA, sizeof(CODE_421_BUSY_DATA) - 1, MSG_DONTWAIT | MSG_NOSIGNAL);
        }
    }
    /*Close the data socket and let the main thread send the last response*/
    sclose(&(session->data_connection->context), &(session->data_connection->conn_fd));
    sclose(NULL, &(session->data_connection->socket_fd));
    /*If it was transmit in passive mode, there is a new free passive port*/
    if (session->data_connection->conn_state != DATA_CONN_CLOSED && session->data_connection->is_passive)
        sem_post(&(server_conf->free_passive_ports));
    session->data_connection->conn_state = DATA_CONN_CLOSED;
    /*Raise the abort flag*/
    session->data_connection->abort = 0;
    return;
}

/**
 * @brief Set the ftp server credentials based on the configuration file
 * If a username was provided, a password will be requested for that user
 * If not provided, the credentials of the user running the program will be used,
 * so the execution must be with root permissions.
 * Upon completion, root permissions are removed for security purposes
 */
void set_ftp_credentials()
{
    /*Decide server authentication method*/
    if (server_conf.ftp_user[0] == '\0')
    {
        char *uname = get_username();
        printf("Usuario no especificado en server.conf, se usaran las credenciales del usuario %s\n", uname);
        /*If no user is provided, use credentials of user executing the program*/
        if (!set_credentials(NULL, NULL))
            errexit("Fallo al establecer credenciales de usuario que ejecuta el programa. Comprobar permisos de root\n");
            /*Remove superuser permissions if we had them*/
#ifndef DEBUG
        drop_root_privileges();
#endif
    }
    else /*Prompt for a password for the given user*/
    {
        char pass[MAX_PASSWORD + 1], pass_repeat[MAX_PASSWORD + 1];
        char *password; /*Needed to use in get_password*/
                        /*Request a password for the provided user*/
        do
        {
            password = pass;
            do
                printf("Establezca una contraseña para el usuario '%s':\n", server_conf.ftp_user);
            while (get_password((char **)&password, MAX_PASSWORD) <= 0);
            password = pass_repeat;
            do
                printf("Repita la contraseña para el usuario '%s':\n", server_conf.ftp_user);
            while (get_password((char **)&password, MAX_PASSWORD) <= 0);
        } while (strcmp(pass, pass_repeat) && printf("Las contraseñas no coinciden\n"));
        /*Use supplied credentials*/
        if (!set_credentials(server_conf.ftp_user, pass))
            errexit("Fallo al establecer credenciales de usuario que ejecuta el programa. Comprobar permisos de root\n");
        /*Clear passwords*/
        memset(pass, '\0', MAX_PASSWORD);
        memset(pass_repeat, '\0', MAX_PASSWORD);
    }
}

/**
 * @brief Add handlers for SIGTERM, SIGINT and SIGPIPE signals
 *
 */
void set_handlers()
{
    struct sigaction act, act_ign;

    /*SIGTERM and SIGINT signals set end flag*/
    act.sa_handler = set_end_flag;
    sigemptyset(&(act.sa_mask));
    act.sa_flags = 0;

    /*SIGPIPE signal is ignored*/
    act_ign.sa_handler = SIG_IGN;
    sigemptyset(&(act_ign.sa_mask));
    act_ign.sa_flags = 0;

    if (sigaction(SIGTERM, &act, NULL) < 0 || sigaction(SIGINT, &act, NULL) < 0 || sigaction(SIGPIPE, &act, NULL) < 0)
        errexit("Fallo al crear mascara: %s\n", strerror(errno));
    return;
}

/**
 * @brief Activates end of program flag
 *
 * @param sig Signal received
 */
void set_end_flag(int sig)
{
    end = 1;
}

/**
 * @brief Load certificate and public key of the server
 *
 */
void tls_start()
{
    tls_init();
    server_conf.server_ctx = tls_create_context(1, TLS_V12);
    if (!server_conf.server_ctx)
        errexit("Fallo al crear contexto TLS\n");
    if (!load_keys(server_conf.server_ctx, server_conf.certificate_path, server_conf.private_key_path))
    {
        tls_destroy_context(server_conf.server_ctx);
        errexit("Fallo al cargar la clave privada y/o certificado\n");
    }
}
