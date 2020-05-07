/**
 * @file ftps_server.c
 * @author Joaquín Jiménez López de Castro (joaquin.jimenezl@estudiante.uam.es)
 * @brief Implementación de servidor ftps
 * @version 1.0
 * @date 07-04-2020
 * 
 * 
 */
#define _DEFAULT_SOURCE /*!< Acceso a funciones de GNU */
#define _POSIX_C_SOURCE 200112L /*!< Acceso a funciones de POSIX */
#include "utils.h"
#include "red.h"
#include "ftp.h"
#include "authenticate.h"
#include "callbacks.h"
#include "ftp_session.h"
#include "config_parser.h"
#include "ftp_files.h"

#define MAX_PASSWORD MEDIUM_SZ /*!< Tamaño maximo de contraseña */
#define USING_AUTHBIND "--using-authbind" /*!< Indica ejecuciona actual con authbind */
#define THREAD_CLOSE_WAIT 2 /*!< Maximo de tiempo que se espera que se cierre un hilo */
#define CONTROL_SOCKET_TIMEOUT 150 /*!< Timeout maximo de conexion de control */
// #define DEBUG

void accept_loop(int socket_control_fd);
void set_ftp_credentials();
void *ftp_session_loop(void *args);
void set_end_flag(int sig);
void set_handlers();
void data_callback_loop(session_info *session, request_info *ri, serverconf * server_conf,char *buf);
void tls_start();

serverconf server_conf; /*!< Configuracion global del servidor */
sem_t n_clients; /*!< Controla que no se exceda el número de sesiones FTP */
int end = 0; /*!< Indica que hay que finalizar el programa */

/**
 * @brief Punto de entrada a la aplicación
 * 
 * @param argc Número de argumentos
 * @param argv Se utiliza el argumento especial USING_AUTHBIND para indicar que la ejecución se realiza con authbind
 * @return int exito si 0 
 */
int main(int argc, char *argv[])
{
    /* Comprobacion un poco burda de que la ejecucion debe ser con authbind */
    #ifndef DEBUG
    if (!(argc == 2 && !strcmp(argv[1], USING_AUTHBIND)))
        execlp("authbind", "authbind", argv[0], USING_AUTHBIND, NULL);
    #endif

    /* Leer la configuracion del servidor */
    if ( parse_server_conf(&server_conf) < 0 )
        errexit("Fallo al procesar fichero de configuracion\n");

    /* Establecer root path del servidor */
    set_root_path(server_conf.server_root);

    /* Establecer los credenciales del servidor y quitarse permisos de root si fueron dados */
    set_ftp_credentials();

    /* Establecer manejadores de señales */
    set_handlers();

    /* Ahora, habria que crear el socket de control */
    int socket_control_fd = socket_srv("tcp", 10, FTP_CONTROL_PORT, server_conf.ftp_host);
    if ( socket_control_fd < 0 )
        errexit("Fallo al abrir socket de control %s\n", strerror(errno));
    set_socket_timeouts(socket_control_fd, CONTROL_SOCKET_TIMEOUT);
    
    /* Inicializar el contexto TLS */
    tls_start();

    /* Establecer configuracion de logueo */
    set_log_conf(!server_conf.daemon_mode, server_conf.daemon_mode, "Servidor FTPS");

    printf("Configuracion terminada, servidor desplegado\n");

    /* Entrar en modo demonio si se ha indicado */
    if ( server_conf.daemon_mode ) daemon(1, 0);

    /* Loop de aceptacion de peticiones en control */
    accept_loop(socket_control_fd);
    
    /* Liberar recursos */
    sclose(&(server_conf.server_ctx), &socket_control_fd);
    exit(0);
}

/**
 * @brief Loop de aceptacion de nuevas conexiones
 * 
 * @param socket_control_fd Socket de conexiones de control entrantes
 */
void accept_loop(int socket_control_fd)
{
    struct sockaddr clt_info;
    size_t clt_info_size = sizeof(clt_info);
    intptr_t clt_fd;
    pthread_t session_thread;
    struct timespec timeout;

    /* Establecer numero maximo de clientes */
    sem_init(&n_clients, 0, server_conf.max_sessions);
    /* Realizar accepts hasta que se cierre el servidor */
    while ( !end )
    {
        /* Esperar a que haya un hueco libre y aceptar al siguiente cliente */
        sem_wait(&n_clients);
        clt_fd = accept(socket_control_fd, &clt_info, (socklen_t *) &clt_info_size);
        if ( end ) { sem_post(&n_clients); break; }
        /* Saludar al cliente */
        send(clt_fd, CODE_220_WELCOME_MSG, sizeof(CODE_220_WELCOME_MSG) - 1, 0);
        /* Abrir cada peticion nueva en un hilo para comenzar la sesion */
        if ( end ) { sem_post(&n_clients); break; }
        session_thread = pthread_create(&session_thread, NULL, ftp_session_loop, (void *) clt_fd);
        pthread_detach(session_thread); /* Convertir en hilo independiente para no tener que hacer join */
    }
    /* Esperar a que todos los hilos terminen, con cierto timeout */
    clock_gettime(CLOCK_REALTIME, &timeout);
    timeout.tv_sec += THREAD_CLOSE_WAIT;
    for ( int i = 0; i < server_conf.max_sessions; i++ )
        if ( sem_timedwait(&n_clients, &timeout) == -1 )
            break;
    return;
}

/**
 * @brief Bucle principal de recepcion de comandos FTP 
 * 
 * @param args Contiene descriptor del cliente
 * @return void* 
 */
void *ftp_session_loop(void *args)
{
    char buff[XXXL_SZ + 1];
    session_info s1, s2, *current = &s1, *previous = &s2, *aux;
    request_info ri = {.command_arg = "", .command_name = "", .response = "", .response_len = 0, .implemented_command = NOOP};
    intptr_t clt_fd = (intptr_t) args, cb_ret = CALLBACK_RET_PROCEED;
    data_conn dc = {.socket_fd = -1, .conn_state = DATA_CONN_CLOSED, .abort = 0, .conn_fd = -1};
    ssize_t read_b;

    /* Sesion FTP: valores constantes */
    sem_init(&(dc.mutex), 0, 1); /* Controla acceso concurrente a la estructura */
    sem_init(&(dc.data_conn_sem), 0, 0); /* Si a 1, Callback de datos ya ha terminado preparativos de callback de datos */
    sem_init(&(dc.control_conn_sem), 0, 0); /* Si a 1, Callback de datos ha finalizado transmision e indicado codigo de retorno */
    s1.data_connection = s2.data_connection = &dc;
    s1.clt_fd = s2.clt_fd = clt_fd;

     /* Sesion FTP: atributos variables */
    init_session_info(current, NULL);
    strcpy(current->current_dir, server_conf.server_root);
    current->ascii_mode = server_conf.default_ascii;

    /* Bucle principal de la sesion */
    while( !end && cb_ret != CALLBACK_RET_END_CONNECTION )
    {
        /* Recoger siguiente comando comprobando de vez en cuando el flag */
        while ( !end && (((read_b = srecv(current->context,clt_fd, buff, XXXL_SZ, MSG_DONTWAIT | MSG_NOSIGNAL)) < 0) && (errno == EWOULDBLOCK || errno == EAGAIN)) && !usleep(1000) );
        if ( end || read_b <= 0) break;
        buff[read_b] = '\0'; /* Por motivos de seguridad aseguramos un cero */
        parse_ftp_command(&ri, buff);
        #ifdef DEBUG
        flog(LOG_DEBUG, "%s %s\n", ri.command_name, (!strcmp(ri.command_name, "PASS")) ? "XXXX" : ri.command_arg);
        #endif
        if ( ri.ignored_command == -1 && ri.implemented_command == -1 ) /* Comando no reconocido */
            ssend(current->context, clt_fd, CODE_500_UNKNOWN_CMD, sizeof(CODE_500_UNKNOWN_CMD) - 1, MSG_NOSIGNAL);
        else if ( ri.implemented_command == -1 ) /* Comando reconocido pero no implementado */
            ssend(current->context, clt_fd, CODE_502_NOT_IMP_CMD, sizeof(CODE_502_NOT_IMP_CMD) - 1, MSG_NOSIGNAL);
        else /* Comando implementado, llamar al callback y devolver respuesta controlando la posible conexion de datos */
        {           
            cb_ret = command_callback(&server_conf, current, &ri);
            /* Si es una transmision de datos entramos en un loop distinto */
            if ( DATA_CALLBACK(ri.implemented_command) && cb_ret != CALLBACK_RET_END_CONNECTION )
                data_callback_loop(current, &ri, &server_conf, buff);
            /* Enviar respuesta final del callbacj */
            if ( cb_ret == CALLBACK_RET_PROCEED  || ri.implemented_command == QUIT)
            {
                ssend(current->context, clt_fd, ri.response, ri.response_len, ((cb_ret != CALLBACK_RET_PROCEED) & MSG_DONTWAIT) | MSG_NOSIGNAL);
                #ifdef DEBUG
                flog(LOG_DEBUG, "-->%s\n", ri.response);
                #endif
            }
            aux = previous;
            previous = current;
            current = aux; /* Sesion actual pasa a ser la sesion previa */
            init_session_info(current, previous); /* La rellenamos a partir de la anterior quitando atributos que expiran */ 
        }
    }
    /* Liberar los atributos de sesion y cerrar la conexion */
    sclose(&(current->context), &(current->clt_fd));
    free_attributes(current);
    sem_post(&n_clients);
    return NULL;
}

/**
 * @brief Queda a la espera de la informacion del thread de conexion de datos
 *        y posibles peticiones nuevas de control, de las cuales solo se aceptaran las de abort
 * @param session Contiene informacion de la sesion
 * @param server_conf Contiene semaforo de puertos en modo pasivo
 * @param ri Se va actualizando con las respuestas del thread de datos
 * @param buf Buffer para recibir datos
 */
void data_callback_loop(session_info *session, request_info *ri, serverconf * server_conf,char *buf)
{
    if ( session->data_connection->conn_state == DATA_CONN_CLOSED ) /* Debe haberse iniciado una conexion de datos con PORT O PASV */
    {
        ssend(session->context, session->clt_fd, CODE_503_BAD_SEQUENCE, sizeof(CODE_503_BAD_SEQUENCE) - 1, MSG_DONTWAIT | MSG_NOSIGNAL);
        return;
    }
    /* Cuando se avance por este semaforo, se habra rellenado en el cliente el codigo de envio inicial 150 */
    sem_wait(&(session->data_connection->data_conn_sem));
    /* Enviamos el codigo de respuesta */
    ssend(session->context, session->clt_fd, ri->response, ri->response_len, MSG_DONTWAIT | MSG_NOSIGNAL);
    /* Esperamos a que el thread de datos comience la transmision */
    sem_post(&(session->data_connection->control_conn_sem));
    sem_wait(&(session->data_connection->data_conn_sem));
    /* Esperamos a que el thread de datos termine la transmision */
    while ( sem_trywait(&(session->data_connection->data_conn_sem)) == -1 )
    {
        /* Comprobar si ha llegado una peticion nueva */
        ssize_t len = srecv(session->context, session->clt_fd, buf, XXXL_SZ + 1, MSG_DONTWAIT | MSG_NOSIGNAL);
        if ( len != -1 )
        {
            /* Si es ABORT, activamos flag de abort (atomico) */
            if ( len >= sizeof("ABORT") && !memcmp(buf, "ABORT", sizeof("ABORT") - 1) )
                session->data_connection->abort = 1;
            /* Ignoramos el resto de peticiones */
            else
                ssend(session->context, session->clt_fd, CODE_421_BUSY_DATA, sizeof(CODE_421_BUSY_DATA) - 1, MSG_DONTWAIT | MSG_NOSIGNAL);
        }
    }
    /* Cerramos el socket de datos y dejamos que el thread principal envie la ultima respuesta */
    sclose(&(session->data_connection->context), &(session->data_connection->conn_fd));
    sclose(NULL, &(session->data_connection->socket_fd));
    /* Si era transmision en modo pasivo, hay un nuevo puerto pasivo libre */
    if ( session->data_connection->conn_state != DATA_CONN_CLOSED && session->data_connection->is_passive )
        sem_post(&(server_conf->free_passive_ports));
    session->data_connection->conn_state = DATA_CONN_CLOSED;
    /* Levantar el flag de abort */
    session->data_connection->abort = 0;
    return;
}

/**
 * @brief Establece los credenciales del servidor ftp en base al fichero de configuracion
 * Si se proporciono un nombre de usuario, se pedira una contraseña para dicho usuario
 * Si no se proporciono, se usaran los credenciales del usuario que ejecuta el programa, 
 * por lo que la ejecucion debera ser con permisos de root.
 * Al finalizar, se quitan los permisos de root por motivos de seguridad
 */
void set_ftp_credentials()
{
    /* Decidir metodo de autenticacion del servidor */
    if ( server_conf.ftp_user[0] == '\0' )
    {
        char *uname = get_username();
        printf("Usuario no especificado en server.conf, se usaran las credenciales del usuario %s\n", uname);
        /* Si no se proporciona usuario, usar credenciales de usuario que ejecuta el programa */
        if ( !set_credentials(NULL, NULL) ) 
            errexit("Fallo al establecer credenciales de usuario que ejecuta el programa. Comprobar permisos de root\n");
        /* Quitarnos permisos de superusuario si los teniamos */
        #ifndef DEBUG
        drop_root_privileges();
        #endif
    }
    else /* Pedir una contraseña para el usuario proporcionado */
    {
        char pass[MAX_PASSWORD + 1], pass_repeat[MAX_PASSWORD + 1];
        char *password; /* Necesario para usar en get_password */
        /* Solicitar una contraseña para el usuario proporcionado */
        do
        {
            password = pass;
            do
                printf("Establezca una contraseña para el usuario '%s':\n", server_conf.ftp_user);
            while ( get_password((char **) &password, MAX_PASSWORD) <= 0 );
            password = pass_repeat;
            do
                printf("Repita la contraseña para el usuario '%s':\n", server_conf.ftp_user);
            while ( get_password((char **) &password, MAX_PASSWORD) <= 0 );
        } while ( strcmp(pass, pass_repeat) && printf("Las contraseñas no coinciden\n") );
        /* Usar credenciales proporcionados */
        if ( !set_credentials(server_conf.ftp_user, pass) )
            errexit("Fallo al establecer credenciales de usuario que ejecuta el programa. Comprobar permisos de root\n");
        /* Limpiar contraseñas */
        memset(pass, '\0', MAX_PASSWORD);
        memset(pass_repeat, '\0', MAX_PASSWORD);
    }
}

/**
 * @brief Añade manejadores para las señales SIGTERM, SIGINT y SIGPIPE
 * 
 */
void set_handlers()
{
    struct sigaction act, act_ign;

    /* Señales SIGTERM y SIGINT activan flag de fin */
    act.sa_handler = set_end_flag;
	sigemptyset(&(act.sa_mask));
	act.sa_flags = 0;
    
    /* Señal SIGPIPE es ignorada */
    act_ign.sa_handler = SIG_IGN;
    sigemptyset(&(act_ign.sa_mask));
    act_ign.sa_flags = 0;

	if ( sigaction(SIGTERM, &act, NULL) < 0 || sigaction(SIGINT, &act, NULL) < 0 || sigaction(SIGPIPE, &act, NULL) < 0)
        errexit("Fallo al crear mascara: %s\n", strerror(errno));
    return;
}

/**
 * @brief Activa flag de fin de programa
 * 
 * @param sig Señal recibida
 */
void set_end_flag(int sig)
{
    end = 1;
}

/**
 * @brief Carga certificado y clave publica del servidor
 * 
 */
void tls_start()
{
    tls_init();
    server_conf.server_ctx = tls_create_context(1, TLS_V12);
    if ( !server_conf.server_ctx )
        errexit("Fallo al crear contexto TLS\n");
    if ( !load_keys(server_conf.server_ctx, server_conf.certificate_path, server_conf.private_key_path) )
    {
        tls_destroy_context(server_conf.server_ctx);
        errexit("Fallo al cargar la clave privada y/o certificado\n");
    }
}
