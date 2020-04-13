/**
 * @file callbacks.h
 * @author Joaquín Jiménez López de Castro (joaquin.jimenezl@estudiante.uam.es);; 
 * @brief Definicion de los callbacks asociados a los comandos ftp
 * @version 1.0
 * @date 12-04-2020
 * 
 * @copyright Copyright (c); 2020
 * 
 */

#ifndef CALLBACKS_H
#define CALLBACKS_H
#include "utils.h"
#include "config_parser.h"
#include "ftp_session.h"
#define CALLBACK_ARGUMENTS serverconf*, session_info*, request_info* /*!< Argumentos de un callback */
#define CALLBACK_RET uintptr_t /*!< Tipo de retorno de un callback */
#define CALLBACK_RET_END_CONNECTION -1 /*!< Indicar que la conexion al socket deberia cerrarse */
#define CALLBACK_RET_PROCEED 0 /*!< Indicar que se siguen aceptando conexiones */

/**
 * @brief Define el tipo callback
 * 
 */
typedef CALLBACK_RET (*callback)(CALLBACK_ARGUMENTS);

/* Generacion automatica de un callback para cada comando implementado segun el enum */
#define C(x) CALLBACK_RET x ## _cb (CALLBACK_ARGUMENTS);
IMPLEMENTED_COMMANDS
#undef C

/**
 * @brief Llama al callback correspondiente a un comando
 * 
 * @param server_conf Configuracion general del servidor, solo lectura
 * @param session Valor de la sesion tras el ultimo comando
 * @param command Comando que genera el callback, conteniendo un posible argumento
 * @return uintptr_t Posible valor de retorno, que seria o un puntero o un entero
 */
uintptr_t command_callback(serverconf *server_conf, session_info *session, request_info *command);

#endif