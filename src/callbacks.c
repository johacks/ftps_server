/**
 * @file callbacks.c
 * @author Joaquín Jiménez López de Castro (joaquin.jimenezl@estudiante.uam.es)
 * @brief Define los callbacks correspondientes a cada comando
 * @version 1.0
 * @date 12-04-2020
 * 
 * @copyright Copyright (c) 2020
 * 
 */

#include "red.h"
#include "utils.h"
#include "ftp.h"
#include "callbacks.h"
#include "ftp_files.h"
#include "config_parser.h"
#include "authenticate.h"

/* Define el array de callbacks */
#define C(x) x ## _cb, /*!< Nombre de funcion de callback asociado a un comando implementado */
/* Array de funciones */
static const callback callbacks[IMP_COMMANDS_TOP] = { IMPLEMENTED_COMMANDS };
#undef C

/* Algunas macros de comprobaciones ampliamente repetidas */
#define CHECK_USERNAME(s, c)                                                            \
{                                                                                       \
    if ( !s->authenticated )                                                            \
    {                                                                                   \
        set_command_response(c, CODE_530_NO_LOGIN, sizeof(CODE_530_NO_LOGIN));          \
        return CALLBACK_RET_PROCEED;                                                    \
    }                                                                                   \
}

#define RESOLVE_PATH(s, c, buf, new)                                                    \
{                                                                                       \
    if ( get_real_path(s->current_dir, c->command_arg, buf) < ((new) ? 0 : 1) )         \
    {                                                                                   \
        set_command_response(c, CODE_550_NO_ACCESS, sizeof(CODE_550_NO_ACCESS));        \
        return CALLBACK_RET_PROCEED;                                                    \
    }                                                                                   \
}

/**
 * @brief Llama al callback correspondiente a un comando
 * 
 * @param server_conf Configuracion general del servidor, solo lectura
 * @param session Valor de la sesion tras el ultimo comando
 * @param command Comando que genera el callback, conteniendo un posible argumento
 * @return uintptr_t Posible valor de retorno, que seria o un puntero o un entero
 */
uintptr_t command_callback(serverconf *server_conf, session_info *session, request_info *command)
{
    return callbacks[command->implemented_command](server_conf, session, command);
}

/* CALLBACKS QUE UTILIZAN LA CONEXION DE DATOS */
/**
 * @brief Define los datos que se le pasan a un thread de datos
 * 
 */
typedef struct _data_thread_args
{
    pthread_t thread;
    serverconf *server_conf;
    session_info *session;
    request_info *command;
} data_thread_args;

/* Macro que hace de middleware entre el callback de un comando de datos y el thread de comando de datos */
#define DATA_cb(COMMAND) CALLBACK_RET COMMAND ## _cb (CALLBACK_ARGUMENTS)               \
{                                                                                       \
    data_thread_args *args = malloc(sizeof(data_thread_args));                          \
    if ( !args )                                                                        \
        return CALLBACK_RET_END_CONNECTION;                                             \
    args->server_conf = server_conf;                                                    \
    args->session = session;                                                            \
    args->command = command;                                                            \
    if ( pthread_create(&(args->thread), NULL, COMMAND ## _cb_thread, args) == -1)      \
    {                                                                                   \
        free(args);                                                                     \
        return CALLBACK_RET_END_CONNECTION;                                             \
    }                                                                                   \
    pthread_detach(args->thread);                                                       \
    return CALLBACK_RET_PROCEED;                                                        \
}     

/* Sincronizacion entre dos hilos para avanzar a la vez tras un evento */
#define RENDEZVOUS(mut1, mut2)                                                          \
{                                                                                       \
    sem_post(&(mut1));                                                                  \
    sem_wait(&(mut2));                                                                  \
    sem_post(&(mut1));                                                                  \
}                                                                                       \

/* Libera los recursos iniciales de un thread tras un fin prematuro */
#define THREAD_PREMATURE_EXIT(t)                                                        \
{                                                                                       \
    RENDEZVOUS(t->session->data_connection->data_conn_sem,                              \
               t->session->data_connection->control_conn_sem)                           \
    sem_post(&(t->session->data_connection->data_conn_sem));                            \
    free(t);                                                                            \
    return NULL;                                                                        \
}

/* Esta macro comprueba que hay una conexion de datos disponible, deshaciendo los semaforos
correspondientes y saliendo del thread si no es el caso */
#define CHECK_DATA_PORT(t, d)                                                           \
{                                                                                       \
    if ( d->conn_state != DATA_CONN_AVAILABLE )                                         \
    {                                                                                   \
        set_command_response(t->command, CODE_503_BAD_SEQUENCE);                        \
        THREAD_PREMATURE_EXIT(t)                                                        \
    }                                                                                   \
    if ( !d->is_passive )                                                               \
    {                                                                                   \
        d->conn_fd = socket_clt_connection(FTP_DATA_PORT,                               \
            d->client_ip, d->client_port, t->server_conf->ftp_host);                    \
        if ( d->conn_fd < 0 )                                                           \
        {                                                                               \
            set_command_response(t->command, CODE_425_CANNOT_OPEN_DATA,strerror(errno));\
            THREAD_PREMATURE_EXIT(t);                                                   \
        }                                                                               \
        set_socket_timeouts(d->conn_fd, DATA_SOCKET_TIMEOUT);                           \
    }                                                                                   \
    else                                                                                \
        d->conn_fd = accept(d->socket_fd, NULL, 0);                                     \
    d->conn_state = DATA_CONN_BUSY;                                                     \
}                                                                                       

/**
 * @brief Envia un fichero
 * 
 * @param args Argumentos del hilo
 * @return void* NULL
 */
void *RETR_cb_thread(void *args)
{
    data_thread_args *t_args = (data_thread_args *) args;
    CHECK_DATA_PORT(t_args, t_args->session->data_connection) /* Comprueba conexion de datos lista */
    /* Obtiene el elemento a dar */
    char path[XXL_SZ] = "";
    if ( get_real_path(t_args->session->current_dir, t_args->command->command_arg, path) < 1 || !path_is_file(path) )
    {
        set_command_response(t_args->command, CODE_550_NO_ACCESS);
        THREAD_PREMATURE_EXIT(t_args);
    }
    /* Indicar primera respuesta al hilo de control: 150, enviando archivo */
    set_command_response(t_args->command, CODE_150_RETR, path_no_root(path));
    /* Sigue el protocolo marcado de concurrencia */
    RENDEZVOUS(t_args->session->data_connection->data_conn_sem, t_args->session->data_connection->control_conn_sem)
    /* Inicia la transferencia */
    FILE *f = fopen(path, "rb");
    if ( !f )
        set_command_response(t_args->command, CODE_550_NO_ACCESS);
    else
    {
        ssize_t sent = send_file(t_args->session->data_connection->conn_fd, f, t_args->session->ascii_mode, &(t_args->session->data_connection->abort));
        if ( sent < 0 )
            set_command_response(t_args->command, CODE_550_NO_ACCESS);
        else
            set_command_response(t_args->command, CODE_226_DATA_TRANSFER, sent);
        fclose(f);
    }
    sem_post(&(t_args->session->data_connection->data_conn_sem)); /* Indicar transmision finalizada */
    free(t_args);
    return NULL;
}

/**
 * @brief Lista contenido de un path
 * 
 * @param args Argumentos del thread
 * @return void* 
 */
void *LIST_cb_thread(void *args)
{
    data_thread_args *t_args = (data_thread_args *) args;
    CHECK_DATA_PORT(t_args, t_args->session->data_connection) /* Comprueba conexion de datos lista */
    /* Obtiene el elemento a dar */
    char path[XXL_SZ] = "";
    if ( get_real_path(t_args->session->current_dir, t_args->command->command_arg, path) < 1)
    {
        set_command_response(t_args->command, CODE_550_NO_ACCESS);
        THREAD_PREMATURE_EXIT(t_args);
    }
    /* Indicar primera respuesta al hilo de control: 150, enviando archivo */
    set_command_response(t_args->command, CODE_150_LIST);
    /* Sigue el protocolo marcado de concurrencia */
    RENDEZVOUS(t_args->session->data_connection->data_conn_sem, t_args->session->data_connection->control_conn_sem)
    /* Inicia la transferencia */
    FILE *f = list_directories(t_args->command->command_arg, t_args->session->current_dir);
    if ( !f )
        set_command_response(t_args->command, CODE_550_NO_ACCESS);
    else
    {
        ssize_t sent = send_file(t_args->session->data_connection->conn_fd, f, t_args->session->ascii_mode, &(t_args->session->data_connection->abort));
        if ( sent < 0 )
            set_command_response(t_args->command, CODE_550_NO_ACCESS);
        else
            set_command_response(t_args->command, CODE_226_DATA_TRANSFER, sent);
        pclose(f);
    }
    sem_post(&(t_args->session->data_connection->data_conn_sem)); /* Indicar transmision finalizada */
    free(t_args);
    return NULL;
}

/**
 * @brief Recibe un fichero del cliente
 * 
 * @param args Argumentos del thread
 * @return void* NULL
 */
void *STOR_cb_thread(void *args)
{
    data_thread_args *t_args = (data_thread_args *) args;
    CHECK_DATA_PORT(t_args, t_args->session->data_connection) /* Comprueba conexion de datos lista */
    /* Obtiene el elemento a dar */
    char path[XXL_SZ] = "";
    if ( get_real_path(t_args->session->current_dir, t_args->command->command_arg, path) < 0 )
    {
        set_command_response(t_args->command, CODE_501_BAD_ARGS);
        THREAD_PREMATURE_EXIT(t_args);
    }
    /* Indicar primera respuesta al hilo de control: 150, enviando archivo */
    set_command_response(t_args->command, CODE_150_STOR, path_no_root(path));
    /* Sigue el protocolo marcado de concurrencia */
    RENDEZVOUS(t_args->session->data_connection->data_conn_sem, t_args->session->data_connection->control_conn_sem)
    /* Inicia la transferencia */
    FILE *f = fopen(path, "wb");
    if ( !f ) /* Posible error al abrir fichero */
        set_command_response(t_args->command, (errno == EACCES) ? CODE_550_NO_ACCESS : CODE_452_NO_SPACE);
    else /* Leer fichero */
    {
        ssize_t sent = read_to_file(f, t_args->session->data_connection->conn_fd, t_args->session->ascii_mode, &(t_args->session->data_connection->abort));
        if ( sent < 0 )
            set_command_response(t_args->command, CODE_451_DATA_CONN_LOST);
        else
            set_command_response(t_args->command, CODE_226_DATA_TRANSFER, sent);
        fclose(f);
    }
    sem_post(&(t_args->session->data_connection->data_conn_sem)); /* Indicar transmision finalizada */
    free(t_args);
    return NULL;
}

/* Genera los callbacks de funciones que necesitan thread de datos */
DATA_cb(RETR)
DATA_cb(LIST)
DATA_cb(STOR)

/* CALLBACKS DE CONTROL */

/**
 * @brief Lista de comandos implementados por el servidor
 * 
 * @param server_conf Configuracion del servidor
 * @param session Sesion FTP
 * @param command Comando que genera el callback
 * @return uintptr_t 
 */
uintptr_t HELP_cb(serverconf *server_conf, session_info *session, request_info *command)
{
    strcpy(command->response, CODE_214_HELP);
    for ( int i = 0; i < IMP_COMMANDS_TOP; i++ )
    {
        strcat(command->response, get_imp_command_name((imp_commands) i));
        strcat(command->response, ",");
    }
    command->response_len = strlen(command->response);
    command->response[command->response_len - 1] = '\r'; /* Cambiar ultima coma por salto de linea */
    command->response[command->response_len++] = '\n';
    return CALLBACK_RET_PROCEED;
}

/**
 * @brief Ofrece un puerto al cliente para transmision de datos
 * 
 * @param server_conf Configuracion del servidor
 * @param session Sesion actual, se guardara informacion del socket que se abre
 * @param command Comando que genera el callback
 * @return uintptr_t 
 */
uintptr_t PASV_cb(serverconf *server_conf, session_info *session, request_info *command)
{
    CHECK_USERNAME(session, command);
    int port;
    char port_string[sizeof("xxx,xxx,xxx,xxx,ppp,ppp")];

    MUTEX_DO(session->data_connection->mutex, /* Operacion atomica */
        if ( session->data_connection->conn_state != DATA_CONN_CLOSED ) /* Conexion de datos abierta */
            set_command_response(command, CODE_421_DATA_OPEN);
        /* Abre y escucha en un puerto pasivo */
        else if ( (port = passive_data_socket_fd(server_conf->ftp_host, /* Fallo al abrir socket */
                  &(server_conf->free_passive_ports), &(session->data_connection->socket_fd))) == -1 )
            set_command_response(command, CODE_425_CANNOT_OPEN_DATA, strerror(errno));
        else /* Generar string de respuesta PASV */
        {
            set_command_response(command, CODE_227_PASV_RES, make_port_string(server_conf->ftp_host, port, port_string));
            session->data_connection->conn_state = DATA_CONN_AVAILABLE;
            session->data_connection->is_passive = 1;
        })
    return CALLBACK_RET_PROCEED;
}

/**
 * @brief Se conecta a un puerto del cliente en modo de transmision de datos
 * 
 * @param server_conf Configuracion del servidor
 * @param session Sesion actual, se guardara informacion del socket que se abre
 * @param command Comando que genera el callback, se espera la ip y puerto de datos del cliente
 * @return uintptr_t 
 */
uintptr_t PORT_cb(serverconf *server_conf, session_info *session, request_info *command)
{
    CHECK_USERNAME(session, command)
    if ( command->command_arg[0] == '\0' ) /* Se espera un Port string */ 
    {
        set_command_response(command, CODE_501_BAD_ARGS);
        return CALLBACK_RET_PROCEED;
    }
    MUTEX_DO(session->data_connection->mutex, /* Operacion protegida */
        if ( session->data_connection->conn_state != DATA_CONN_CLOSED )
            set_command_response(command, CODE_421_DATA_OPEN); /* Intentar parsear port string */
        else if ( parse_port_string(command->command_arg, session->data_connection->client_ip, &(session->data_connection->client_port)) < 0 )
            set_command_response(command, CODE_501_BAD_ARGS); /* Fallo al parsear port string */
        else
        {
            set_command_response(command, CODE_200_OP_OK); /* Port string parseado correctamente */
            session->data_connection->conn_state = DATA_CONN_AVAILABLE;
            session->data_connection->is_passive = 0;
        })
    return CALLBACK_RET_PROCEED;
}

/**
 * @brief Borra un archivo del servidor
 * 
 * @param server_conf Configuración del servidor
 * @param session Sesion FTP actual
 * @param command Comando que genera el callback, se espera encontrar nombre del archivo a borrar
 * @return uintptr_t 
 */
uintptr_t DELE_cb(serverconf *server_conf, session_info *session, request_info *command)
{
    CHECK_USERNAME(session, command)
    if ( command->command_arg[0] == '\0' ) /* Sin nombre del fichero */
        set_command_response(command, CODE_501_BAD_ARGS);
    else
    {
        char path[XXL_SZ] = "";
        RESOLVE_PATH(session, command, path, 0); /* Recoger fichero a borrar */
        if ( !path_is_file(path) )
            set_command_response(command, CODE_550_NO_DELE, "No es un fichero"); /* Comprobar que es fichero */
        else
        {
            char rm_cmd[XXL_SZ + sizeof("rm -r ")];
            sprintf(rm_cmd, "rm -r %s", path); /* Se utilizara el comando del sistema */ 
            if ( system(rm_cmd) != 0 || access(path, F_OK) != -1 ) /* El fichero debe haberse borrado */
                set_command_response(command, CODE_550_NO_DELE, strerror(errno));
            else
                set_command_response(command, CODE_250_DELE_OK, path_no_root(path));
        }
    }
    return CALLBACK_RET_PROCEED;
}

/**
 * @brief Borra un directorio recursivamente
 * 
 * @param server_conf Configuracion del servidor
 * @param session Sesion actual
 * @param command Comando que genera el callback, se espera encontrar nombre del directorio a borrar
 * @return uintptr_t 
 */
uintptr_t RMDA_cb(serverconf *server_conf, session_info *session, request_info *command)
{
    CHECK_USERNAME(session, command)
    if ( command->command_arg[0] == '\0' ) /* Sin nombre del directorio */
        set_command_response(command, CODE_501_BAD_ARGS);
    else
    {
        char path[XXL_SZ] = "";
        RESOLVE_PATH(session, command, path, 0); /* Recoger directorio a borrar */
        if ( !path_is_dir(path) )
            set_command_response(command, CODE_550_NO_DELE, "No es un directorio"); /* Comprobar que es directorio */
        else
        {
            char rm_cmd[XXL_SZ + sizeof("rm -rfd ")];
            sprintf(rm_cmd, "rm -rfd %s", path); /* Se utilizara el comando del sistema */ 
            if ( system(rm_cmd) != 0 || access(path, F_OK) != -1 ) /* El directorio debe haberse borrado */
                set_command_response(command, CODE_550_NO_DELE, strerror(errno));
            else
                set_command_response(command, CODE_250_DELE_OK, path_no_root(path));
        }
    }
    return CALLBACK_RET_PROCEED;
}

/**
 * @brief Borra un directorio vacio
 * 
 * @param server_conf Configuracion del servidor
 * @param session Sesion actual
 * @param command Comando que genera el callback, se espera encontrar el nombre del directorio a borrar
 * @return uintptr_t 
 */
uintptr_t RMD_cb(serverconf *server_conf, session_info *session, request_info *command)
{
    CHECK_USERNAME(session, command)
    if ( command->command_arg[0] == '\0' ) /* Sin nombre del directorio */
        set_command_response(command, CODE_501_BAD_ARGS);
    else
    {
        char path[XXL_SZ] = "";
        RESOLVE_PATH(session, command, path, 0); /* Recoger directorio a borrar */
        if ( rmdir(path) == -1 )
            set_command_response(command, CODE_550_NO_DELE, strerror(errno));
        else
            set_command_response(command, CODE_250_DELE_OK, path_no_root(path)); /* Devolver path al directorio eliminado */
    }
    return CALLBACK_RET_PROCEED;
}

/**
 * @brief Crea un directorio
 * 
 * @param server_conf Configuracion del servidor
 * @param session Sesion FTP actual
 * @param command Comando que genera el callback, se espera nombre del directorio a crear como argumento
 * @return uintptr_t 
 */
uintptr_t MKD_cb(serverconf *server_conf, session_info *session, request_info *command)
{
    CHECK_USERNAME(session, command)
    if ( command->command_arg[0] == '\0' )
        set_command_response(command, CODE_501_BAD_ARGS);
    else
    {
        char path[XXL_SZ] = "";
        RESOLVE_PATH(session, command, path, 1);
        if ( mkdir(path, 0777) == -1 ) /* Crea un directorio en el path resuelto */
            set_command_response(command, CODE_550_NO_ACCESS);
        else
            set_command_response(command, CODE_257_MKD_OK, path_no_root(path)); /* Devuelve el nombre del path creado */
    }
    return CALLBACK_RET_PROCEED;
}

/**
 * @brief Cambia al directorio padre
 * 
 * @param server_conf Configuracion del servidor
 * @param session Sesion, que contiene el directorio actual
 * @param command Comando que genera el callback
 * @return uintptr_t 
 */
uintptr_t CDUP_cb(serverconf *server_conf, session_info *session, request_info *command)
{
    CHECK_USERNAME(session, command)
    if ( ch_to_parent_dir(session->current_dir) < 0 )
        return CALLBACK_RET_END_CONNECTION; /* Error de memoria */
    set_command_response(command, CODE_250_CHDIR_OK, path_no_root(session->current_dir));   
    return CALLBACK_RET_PROCEED;
}

/**
 * @brief Cambia el directorio actual
 * 
 * @param server_conf Configuracion del servidor
 * @param session Sesion, que contiene el directorio actual
 * @param command Comando que genera el callback. Se espera argumento con el directorio siguiente
 * @return uintptr_t 
 */
uintptr_t CWD_cb(serverconf *server_conf, session_info *session, request_info *command)
{
    CHECK_USERNAME(session, command)
    if ( command->command_arg[0] == '\0' )
        strcpy(command->command_arg, "/");
    switch ( ch_current_dir(session->current_dir, command->command_arg) )
    {
    case -1:
        return CALLBACK_RET_END_CONNECTION; /* Error de memoria */
    case -2:
        set_command_response(command, CODE_550_NO_ACCESS);
        break;
    default:
        set_command_response(command, CODE_250_CHDIR_OK, path_no_root(session->current_dir));   
        break;
    }
    return CALLBACK_RET_PROCEED;
}

/**
 * @brief Imprime el directorio actual
 * 
 * @param server_conf Configuracion del servidor
 * @param session Sesion, que contiene el directorio actual
 * @param command Comando que genera el callback
 * @return uintptr_t 
 */
uintptr_t PWD_cb(serverconf *server_conf, session_info *session, request_info *command)
{
    CHECK_USERNAME(session, command)
    set_command_response(command, CODE_257_PWD_OK, path_no_root(session->current_dir));
    return CALLBACK_RET_PROCEED;
}

/**
 * @brief Termina la sesion actual
 * 
 * @param server_conf Configuracion del servidor
 * @param session Sesion actual
 * @param command Comando que genera el callback
 * @return uintptr_t CALLBACK_RET_END_CONNECTION
 */
uintptr_t QUIT_cb(serverconf *server_conf, session_info *session, request_info *command)
{
    set_command_response(command, CODE_221_GOODBYE_MSG);
    return CALLBACK_RET_END_CONNECTION;
}

/**
 * @brief Termina de renombrar el fichero
 * 
 * @param server_conf Configuracion del servidor
 * @param session Sesion FTP, espera encontrar nombre origen
 * @param command Comando que genera el callback
 * @return uintptr_t CALLBACK_RET_PROCEED
 */
uintptr_t RNTO_cb(serverconf *server_conf, session_info *session, request_info *command)
{
    CHECK_USERNAME(session, command)
    char path[XXL_SZ] = "";
    RESOLVE_PATH(session, command, path, 1)
    char *rnfr = (char *) get_attribute(session, RENAME_FROM_ATTR); /* Recupera fichero origen */
    if ( ((uintptr_t) rnfr) == ATTR_NOT_FOUND )
        set_command_response(command, CODE_503_BAD_SEQUENCE);
    else
    {
        rename(rnfr, path); /* Renombra el fichero */
        set_command_response(command, CODE_25O_FILE_OP_OK);
    }
    return CALLBACK_RET_PROCEED;
}

/**
 * @brief Renombra un fichero
 * 
 * @param server_conf Configuracion del servidor
 * @param session Sesion FTP, añade atributo de fichero origen
 * @param command Comando que genera el callback
 * @return uintptr_t CALLBACK_RET_PROCEED o CALLBACK_RET_END_CONNECTION
 */
uintptr_t RNFR_cb(serverconf *server_conf, session_info *session, request_info *command)
{
    char *path = malloc(XXL_SZ);
    if ( !path )
        return CALLBACK_RET_END_CONNECTION;
    CHECK_USERNAME(session, command)
    RESOLVE_PATH(session, command, path, 0)
    set_attribute(session, RENAME_FROM_ATTR, (uintptr_t) path, 1, 1); /* Atributo de sesion: archivo a renombrar */
    set_command_response(command, CODE_350_RNTO_NEEDED);
    return CALLBACK_RET_PROCEED;
}

/**
 * @brief Tamaño en bytes de un fichero
 * 
 * @param server_conf Configuracion del servidor
 * @param session Sesion FTP
 * @param command Comando enviado
 * @return uintptr_t 
 */
uintptr_t SIZE_cb(serverconf *server_conf, session_info *session, request_info *command)
{
    char path[XXL_SZ];
    CHECK_USERNAME(session, command)
    RESOLVE_PATH(session, command, path, 0)
    size_t s = name_file_size(path); /* Calcular tamaño de fichero */
    if ( s == -1 )
        set_command_response(command, CODE_550_NO_ACCESS);
    else
        set_command_response(command, CODE_213_FILE_SIZE, s);
    return CALLBACK_RET_PROCEED;
}

/**
 * @brief Cambia a modo binario o ascii
 * 
 * @param server_conf Configuracion del servidor
 * @param session Informacion de sesion
 * @param command Comando que genera el callback
 * @return uintptr_t 
 */
uintptr_t TYPE_cb(serverconf *server_conf, session_info *session, request_info *command)
{
    CHECK_USERNAME(session, command)
    if ( !strcmp(command->command_arg, "A") ) /* Modo ascii */
        session->ascii_mode = 1;
    else if ( !strcmp(command->command_arg, "I") ) /* Modo binario */
        session->ascii_mode = 0;
    else
        set_command_response(command, CODE_501_BAD_ARGS);
    set_command_response(command, CODE_200_OP_OK);
    return CALLBACK_RET_PROCEED;
}

/**
 * @brief Indica sistema operativo
 * 
 * @param server_conf Configuracion del servidor
 * @param session Informacion de sesion
 * @param command Comando que genera el callback
 * @return uintptr_t 
 */
uintptr_t SYST_cb(serverconf *server_conf, session_info *session, request_info *command)
{
    set_command_response(command, CODE_215_SYST, operating_system());
    return CALLBACK_RET_PROCEED;
}

/**
 * @brief No hace nada
 * 
 * @param server_conf Configuracion del servidor
 * @param session Informacion de sesion
 * @param command Comando que genera el callback
 * @return uintptr_t 
 */
uintptr_t NOOP_cb(serverconf *server_conf, session_info *session, request_info *command)
{
    CHECK_USERNAME(session, command)    
    set_command_response(command, CODE_200_OP_OK);
    return CALLBACK_RET_PROCEED;
}

/**
 * @brief Cambia modo de transmision (compresion)
 * 
 * @param server_conf Configuracion del servidor
 * @param session Informacion de sesion
 * @param command Comando que genera el callback, se espera argumento S, B, C o Z
 * @return uintptr_t 
 */
uintptr_t MODE_cb(serverconf *server_conf, session_info *session, request_info *command)
{
    CHECK_USERNAME(session, command)
    if ( command->command_arg[0] == '\0' )
        set_command_response(command, CODE_501_BAD_ARGS);
    else if ( !strcmp(command->command_arg, "S") ) /* Modo Stream */
        set_command_response(command, CODE_200_OP_OK);
    else /* Actualmente solo se soporta stream */
        set_command_response(command, CODE_504_UNSUPORTED_PARAM);
    return CALLBACK_RET_PROCEED;
}

/**
 * @brief Cambiar estructura de archivos de servidor
 * 
 * @param server_conf Configuracion del servidor
 * @param session Informacion de sesion
 * @param command Comando que genera el callback, se espera argumento F, R o P
 * @return uintptr_t 
 */
uintptr_t STRU_cb(serverconf *server_conf, session_info *session, request_info *command)
{
    CHECK_USERNAME(session, command)
    if ( command->command_arg[0] == '\0' )
        set_command_response(command, CODE_501_BAD_ARGS);
    else if ( !strcmp(command->command_arg, "F") ) /* Modo Fichero */
        set_command_response(command, CODE_200_OP_OK);
    else /* Actualmente solo se soporta modo fichero */
        set_command_response(command, CODE_504_UNSUPORTED_PARAM);
    return CALLBACK_RET_PROCEED;
}


/* CALLBACKS DE AUTENTICACION */

/**
 * @brief Autentica un usuario mediante su contraseña
 * 
 * @param server_conf Configuracion del servidor
 * @param session Sesion actual,se espera encontrar el atributo username
 * @param command Comando, se espera encontrar una contraseña
 * @return uintptr_t 
 */
uintptr_t PASS_cb(serverconf *server_conf, session_info *session, request_info *command)
{
    char *username = (char *) get_attribute(session, USERNAME_ATTR);
    /* Si no se ha mandado comando user con el nombre de usuario, exigirlo */
    if ( ((uintptr_t) username) == ATTR_NOT_FOUND )
        set_command_response(command, CODE_503_BAD_SEQUENCE);
    else
    {
        /* Autenticar usuario */
        if ( !(validate_pass(command->command_arg) && validate_user(username)) )
            set_command_response(command, CODE_430_INVALID_AUTH);
        else
        {
            /* Contraseña correcta, indicar en la sesion que el usuario se ha logueado */
            set_command_response(command, CODE_230_AUTH_OK);
            session->authenticated = 1;
        }
        explicit_bzero(command->command_arg, strlen(command->command_arg)); /* Limpiar la contraseña en crudo */
    }
    return CALLBACK_RET_PROCEED;
}

/**
 * @brief Recibe un nombre de usuario y lo guarda en sesion
 * 
 * @param server_conf Configuracion del servidor
 * @param session Sesion actual, no se espera ningun atributo
 * @param command Comando ejecutado, se espera un nombre de usuario
 * @return uintptr_t devuelve CALLBACK_RET_END_CONNECTION si fallo de memoria
 */
uintptr_t USER_cb(serverconf *server_conf, session_info *session, request_info *command)
{
    char *username = malloc(FTP_USER_MAX * sizeof(char));
    if ( !username )
        return CALLBACK_RET_END_CONNECTION;
    strncpy(username, command->command_arg, FTP_USER_MAX - 1);
    /* Guardar el atributo de nombre de usuario al que debe llamarse free con caducidad 1 */
    set_attribute(session, USERNAME_ATTR, (uintptr_t) username, 1, 1);
    /* Exigir la contraseña */
    set_command_response(command, CODE_331_PASS);
    return CALLBACK_RET_PROCEED;
}

/*.......*/

/* Debe existir este callback por motivos de compilacion */
uintptr_t ABOR_cb(serverconf *server_conf, session_info *session, request_info *command)
{ return CALLBACK_RET_PROCEED; }