/**
 * @file callbacks.h
 * @author Joaquín Jiménez López de Castro (joaquin.jimenezl@estudiante.uam.es);;
 * @brief Definition of callbacks associated with ftp commands
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
#define CB_ARG1 server_conf                                                                  /*!< Argument 1 of a callback: server configuration*/
#define CB_ARG2 session                                                                      /*!< Argument 2 of a callback: session information*/
#define CB_ARG3 command                                                                      /*!< Argument 3 of a callback: command information*/
#define CALLBACK_ARGUMENTS serverconf *CB_ARG1, session_info *CB_ARG2, request_info *CB_ARG3 /*!< Arguments of a callback*/
#define N_CALLBACK_ARGUMENTS 3                                                               /*!< How many arguments does a callback have*/
#define CALLBACK_RET uintptr_t                                                               /*!< Return type of a callback*/
#define CALLBACK_RET_END_CONNECTION -1                                                       /*!< Indicate that the connection to the socket should be closed*/
#define CALLBACK_RET_PROCEED 0                                                               /*!< Indicate that connections are still being accepted*/
#define CALLBACK_RET_DONT_SEND 1                                                             /*!< Tell the higher level not to send the response*/
/**
 * @brief Defines the callback type
 *
 */
typedef CALLBACK_RET (*callback)(CALLBACK_ARGUMENTS);

/*Automatic generation of a callback for each command implemented according to the enum*/
#define C(x) CALLBACK_RET x##_cb(CALLBACK_ARGUMENTS); /*!< For each command in the enumeration, generate prototype*/
IMPLEMENTED_COMMANDS
#undef C

/**
 * @brief Calls the callback corresponding to a command
 *
 * @param server_conf General server configuration, read only
 * @param session Value of the session after the last command
 * @param command Command that generates the callback, containing a possible argument
 * @return uintptr_t Possible return value, which would be either a pointer or an integer
 */
uintptr_t command_callback(serverconf *server_conf, session_info *session, request_info *command);

#endif