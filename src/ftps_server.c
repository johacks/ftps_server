/**
 * @file ftps_server.c
 * @author Joaquín Jiménez López de Castro (joaquin.jimenezl@estudiante.uam.es)
 * @brief Implementación de servidor ftps
 * @version 1.0
 * @date 07-04-2020
 * 
 * 
 */
#define _DEFAULT_SOURCE
#define _POSIX_C_SOURCE 200112L
#include "utils.h"
#include "red.h"
#include "ftp.h"
#include "authenticate.h"
#include "callbacks.h"
#include "ftp_session.h"
#include "config_parser.h"

#define MAX_PASSWORD MEDIUM_SZ
#define FTP_CONTROL_PORT 21
#define FTP_DATA_PORT 20
#define USING_AUTHBIND "--using-authbind"
#define THREAD_CLOSE_WAIT 2 /*!< Maximo de tiempo que se espera que se cierre un hilo */

void free_resources(int socket_control_fd, int socket_data_fd);
void accept_loop(int socket_control_fd);
void set_ftp_credentials();
void *ftp_session_loop(void *args);
void set_end_flag(int sig);
void set_handlers();

int modo_daemon = 0; /* TODO: Añadir al fichero de configuracion */
serverconf server_conf;
sem_t n_clients;
int end = 0; /*!< Indica que hay que finalizar el programa */

int main(int argc, char *argv[])
{
    /* Comprobacion un poco burda de que la ejecucion debe ser con authbind */
    // if (!(argc == 2 && !strcmp(argv[1], USING_AUTHBIND)))
    //     execlp("authbind", "authbind", argv[0], USING_AUTHBIND, NULL);

    /* Leer la configuracion del servidor */
    if ( parse_server_conf(&server_conf) < 0 )
        errexit("Fallo al procesar fichero de configuracion\n");

    /* Establecer los credenciales del servidor y quitarse permisos de root si fueron dados */
    set_ftp_credentials();

    /* Establecer manejadores de señales */
    set_handlers();

    /* Ahora, habria que crear el socket de control */
    int socket_control_fd = socket_srv("tcp", 10, FTP_CONTROL_PORT, server_conf.ftp_host);
    if ( socket_control_fd < 0 )
        errexit("Fallo al abrir socket de control %s\n", strerror(errno));

    /* Establecer configuracion de logueo */
    set_log_conf(1, 0, NULL);

    /* Loop de aceptacion de peticiones en control */
    accept_loop(socket_control_fd);
    
    /* Liberar recursos */
    free_resources(socket_control_fd, -1);
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
        send(clt_fd, CODE_220_WELCOME_MSG, sizeof(CODE_220_WELCOME_MSG), 0);
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
    char buff[XL_SZ + 1];
    session_info s1, s2, *current = &s1, *previous = &s2, *aux;
    request_info ri;
    intptr_t clt_fd = (intptr_t) args, cb_ret = CALLBACK_RET_PROCEED;
    s1.clt_fd = s2.clt_fd = clt_fd;

    init_session_info(current, NULL); /* Iniciar estructura de sesion con valores por defecto */

    /* Bucle principal de la sesion */
    while( !end && cb_ret != CALLBACK_RET_END_CONNECTION )
    {
        /* Recoger siguiente comando comprobando de vez en cuando el flag */
        while ( !end && (recv(clt_fd, buff, XL_SZ, MSG_DONTWAIT) == -1) && !usleep(10) );
        if ( end ) break;

        parse_ftp_command(&ri, buff);
        if ( ri.ignored_command == -1 && ri.implemented_command == -1 ) /* Comando no reconocido */
            send(clt_fd, CODE_501_UNKNOWN_CMD_MSG, sizeof(CODE_501_UNKNOWN_CMD_MSG), 0);
        else if ( ri.implemented_command == -1 ) /* Comando reconocido pero no implementado */
            send(clt_fd, CODE_502_NOT_IMP_CMD_MSG, sizeof(CODE_502_NOT_IMP_CMD_MSG), 0);
        else /* Comando implementado, llamar al callback */
        {
           cb_ret = command_callback(&server_conf, current, &ri);
           aux = previous;
           previous = current;
           current = aux; /* Sesion actual pasa a ser la sesion previa */
           init_session_info(current, previous); /* La rellenamos a partir de la anterior quitando atributos que expiran */ 
        }
    }
    /* Liberar los atributos de sesion y cerrar la conexion */
    free_attributes(current);
    close(clt_fd);
    sem_post(&n_clients);
    return NULL;
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
        printf("Usuario no especificado en server.conf, se usaran las credenciales del usuario %s\n", getlogin());
        /* Si no se proporciona usuario, usar credenciales de usuario que ejecuta el programa */
        if ( !set_credentials(NULL, NULL) ) 
            errexit("Fallo al establecer credenciales de usuario que ejecuta el programa. Comprobar permisos de root\n");
        /* Quitarnos permisos de superusuario si los teniamos */
        // drop_root_privileges();
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

	if ( sigaction(SIGTERM, &act, NULL) < 0 || sigaction(SIGINT, &act, NULL) < 0 || sigaction(SIGTERM, &act, NULL) < 0)
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

void free_resources(int socket_control_fd, int socket_data_fd)
{
    if ( socket_control_fd > 0 ) close(socket_control_fd);
    if ( socket_data_fd > 0 ) close(socket_data_fd);
    return;
}
