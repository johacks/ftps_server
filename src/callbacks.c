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
#include "config_parser.h"
#include "authenticate.h"

/* Define el array de callbacks */
#define C(x) x ## _cb, /*!< Nombre de funcion de callback asociado a un comando implementado */
/* Array de funciones */
static const callback callbacks[IMP_COMMANDS_TOP] = { IMPLEMENTED_COMMANDS };
#undef C

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

uintptr_t CDUP_cb(serverconf *server_conf, session_info *session, request_info *command)
{
    return CALLBACK_RET_PROCEED;
}

uintptr_t CWD_cb(serverconf *server_conf, session_info *session, request_info *command)
{
    return CALLBACK_RET_PROCEED;
}

uintptr_t HELP_cb(serverconf *server_conf, session_info *session, request_info *command)
{
    return CALLBACK_RET_PROCEED;
}

uintptr_t MKD_cb(serverconf *server_conf, session_info *session, request_info *command)
{
    return CALLBACK_RET_PROCEED;
}


uintptr_t RNTO_cb(serverconf *server_conf, session_info *session, request_info *command)
{
    return CALLBACK_RET_PROCEED;
}

uintptr_t LIST_cb(serverconf *server_conf, session_info *session, request_info *command)
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

uintptr_t PWD_cb(serverconf *server_conf, session_info *session, request_info *command)
{
    return CALLBACK_RET_PROCEED;
}

uintptr_t QUIT_cb(serverconf *server_conf, session_info *session, request_info *command)
{
    return CALLBACK_RET_PROCEED;
}

uintptr_t RETR_cb(serverconf *server_conf, session_info *session, request_info *command)
{
    return CALLBACK_RET_PROCEED;
}

uintptr_t RMD_cb(serverconf *server_conf, session_info *session, request_info *command)
{
    return CALLBACK_RET_PROCEED;
}

uintptr_t RMDA_cb(serverconf *server_conf, session_info *session, request_info *command)
{
    return CALLBACK_RET_PROCEED;
}

uintptr_t STOR_cb(serverconf *server_conf, session_info *session, request_info *command)
{
    return CALLBACK_RET_PROCEED;
}

uintptr_t RNFR_cb(serverconf *server_conf, session_info *session, request_info *command)
{
    return CALLBACK_RET_PROCEED;
}

uintptr_t SIZE_cb(serverconf *server_conf, session_info *session, request_info *command)
{
    return CALLBACK_RET_PROCEED;
}

uintptr_t TYPE_cb(serverconf *server_conf, session_info *session, request_info *command)
{
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
        send(session->clt_fd, CODE_503_BAD_SEQUENCE, sizeof(CODE_503_BAD_SEQUENCE), 0);
    else
    {
        /* Autenticar usuario */
        if ( !(validate_pass(command->command_arg) && validate_user(username)) )
            send(session->clt_fd, CODE_430_INVALID_AUTH, sizeof(CODE_430_INVALID_AUTH), 0);
        else
        {
            /* Contraseña correcta, indicar en la sesion que el usuario se ha logueado */
            send(session->clt_fd, CODE_230_AUTH_OK, sizeof(CODE_230_AUTH_OK), 0);
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
    set_attribute(session, "username", (uintptr_t) username, 1, 1);
    /* Exigir la contraseña */
    send(session->clt_fd, CODE_331_PASS, sizeof(CODE_331_PASS), 0);
    return CALLBACK_RET_PROCEED;
}