/**
 * @file ftp_session.h
 * @author Joaquín Jiménez López de Castro (joaquin.jimenezl@estudiante.uam.es)
 * @brief Contiene la estructura ftp_session y operaciones asociadas a esta
 * @version 1.0
 * @date 13-04-2020
 * 
 * @copyright Copyright (c) 2020
 * 
 */

#ifndef FTP_SESSION_H
#define FTP_SESSION_H

#include "utils.h"
#include "ftp_files.h"
#define MAX_PATH MEDIUM_SZ + 1
#define MAX_ATTRIBUTES SMALL_SZ
#define MAX_ATTRIBUTE_NAME SMALL_SZ
#define POWER_OF_TWO(x) (((uintptr_t) 1) << (x)) /*!< Elevar al cuadrado */
#define ATTR_NOT_FOUND POWER_OF_TWO(8*sizeof(uintptr_t)-1)*-1 /*!< Valor minimo de uintptr_t */ 

/**
 * @brief Define un atributo mediante su nombre y valor, que posiblemente
 * sera o un puntero, o un valor estatico, como un entero
 */
typedef struct _attribute
{
    char name[MAX_ATTRIBUTE_NAME]; /*!< Nombre del atributo */
    uintptr_t val; /*!< Puede ser o un puntero, o un entero con signo de 8 bytes, segun convenga */
    char freeable; /*!< Indica si el atributo puede liberarse */
    short expire; /*!< Indica por cuantas peticiones a control pasa antes de desaparecer de la sesion */
} attribute;

/**
 * @brief Informacion de sesion que se mantiene entre peticiones al puerto de control
 * 
 */
typedef struct _session_info
{
    data_conn *data_connection; /*!< Almacena informacion de la conexion de datos actual */
    int passive_mode; /*!< Indica que se hacen las transmisiones en modo pasivo */
    int ascii_mode; /*!< Indica que se hace la transmision de ficheros en modo ascii */
    int authenticated; /*!< Indica si el usuario de la sesion ya ha sido autenticado correctamente */
    char current_dir[MAX_PATH]; /*!< Directorio actual del usuario de la sesion */
    int n_attributes; /*!< Cantidad de atributos actualmente en sesion */
    attribute attributes[MAX_ATTRIBUTES]; /*!< Atributos variables de la sesion */
    int clt_fd; /*!< Descriptor de fichero del cliente (conexion de control) */
} session_info;

/**
 * @brief Añadir un atributo
 * 
 * @param session Sesion FTP
 * @param name Nombre del atributo
 * @param val Valor del atributo, posiblemente un puntero o entero
 * @param freeable Indica si el atributo puede liberarse con una simple llamada a free
 * @param expiration Indica cuantas peticiones de control aguanta el atributo
 * @return uintptr_t Si el atributo ya estaba y no se puede liberar, devuelve el valor anterior,
 *         si no, devuelve ATTR_NOT_FOUND
 */
uintptr_t set_attribute(session_info *session, char *name, uintptr_t val, char freeable, short expiration);

/**
 * @brief Recoger un atributo
 * 
 * @param session Sesion FTP
 * @param name Nombre del atributo
 * @return uintptr_t Valor del atributo o ATTR_NOT_FOUND
 */
uintptr_t get_attribute(session_info *session, char *name);

/**
 * @brief Libera aquellos atributos que se pueden liberar con free
 * 
 * @param session Sesion FTP
 * @return int Numero de atributos liberados
 */
int free_attributes(session_info *session);

/**
 * @brief Inicializa una sesion
 * 
 * @param session Sesion FTP
 * @param previous_session Sesion FTP a anterior de la que se heredan los atributos
 */
void init_session_info(session_info *session, session_info *previous_session);

/* ATRIBUTOS UTILIZADOS */
#define USERNAME_ATTR "usr" /*!< Nombre de usuario de un comando USER */
#define RENAME_FROM_ATTR "rnfr" /*!< Nombre de fichero a renombrar */
#endif