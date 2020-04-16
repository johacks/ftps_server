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

uintptr_t HELP_cb(serverconf *server_conf, session_info *session, request_info *command)
{
    return CALLBACK_RET_PROCEED;
}

uintptr_t PASV_cb(serverconf *server_conf, session_info *session, request_info *command)
{
    return CALLBACK_RET_PROCEED;
}

uintptr_t DELE_cb(serverconf *server_conf, session_info *session, request_info *command)
{
    return CALLBACK_RET_PROCEED;
}

uintptr_t PORT_cb(serverconf *server_conf, session_info *session, request_info *command)
{
    return CALLBACK_RET_PROCEED;
}

uintptr_t RETR_cb(serverconf *server_conf, session_info *session, request_info *command)
{
    return CALLBACK_RET_PROCEED;
}

uintptr_t LIST_cb(serverconf *server_conf, session_info *session, request_info *command)
{
    return CALLBACK_RET_PROCEED;
}

uintptr_t STOR_cb(serverconf *server_conf, session_info *session, request_info *command)
{
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
            set_command_response(command, CODE_550_NO_ACCESS); /* Comprobar que es directorio */
        else
        {
            char rm_cmd[XXL_SZ + sizeof("rm -rfd ")];
            sprintf(rm_cmd, "rm -rfd %s", path); /* Se utilizara el comando del sistema */ 
            if ( system(rm_cmd) != 0 || access(path, F_OK) != -1 ) /* El directorio debe haberse borrado */
                set_command_response(command, CODE_550_NO_ACCESS);
            else
                set_command_response(command, CODE_25O_FILE_OP_OK);
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
            set_command_response(command, CODE_550_NO_ACCESS);
        else
            set_command_response(command, CODE_25O_FILE_OP_OK);
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
        if ( mkdir(path, 0) == -1 ) /* Crea un directorio en el path resuelto */
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
        set_command_response(command, CODE_501_BAD_ARGS);
    else
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