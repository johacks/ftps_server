/**
 * @file ftp_session.h
 * @author Joaquín Jiménez López de Castro (joaquin.jimenezl@estudiante.uam.es)
 * @brief Contains the ftp_session structure and operations associated with it. Keep ftp session information
 * @version 1.0
 * @date 04-13-2020
 *
 * @copyright Copyright (c) 2020
 *
 */

#ifndef FTP_SESSION_H
#define FTP_SESSION_H

#include "utils.h"
#include "ftp_files.h"
#define MAX_PATH XL_SZ + 1                                          /*!< Maximum size of the current path*/
#define MAX_ATTRIBUTES SMALL_SZ                                     /*!< Maximum number of dynamic attributes of the session*/
#define MAX_ATTRIBUTE_NAME SMALL_SZ                                 /*!< Maximum size of a dynamic attribute name*/
#define POWER_OF_TWO(x) (((uintptr_t)1) << (x))                     /*!< Square*/
#define ATTR_NOT_FOUND POWER_OF_TWO(8 * sizeof(uintptr_t) - 1) * -1 /*!< Minimum value of uintptr_t*/

/**
 * @brief Defines an attribute by its name and value, possibly
 * will be either a pointer, or a static value, such as an integer
 */
typedef struct _attribute
{
    char name[MAX_ATTRIBUTE_NAME]; /*!< Attribute name*/
    uintptr_t val;                 /*!< Can be either a pointer, or an 8-byte signed integer, as appropriate*/
    char freeable;                 /*!< Indicates if the attribute can be freed*/
    short expire;                  /*!< Indicates for how many requests to control passes before disappearing from the session*/
} attribute;

/**
 * @brief Session information that is maintained between requests to the control port
 *
 */
typedef struct _session_info
{
    data_conn *data_connection;           /*!< Stores information about the current data connection*/
    int ascii_mode;                       /*!< Indicates that the file transmission is done in ascii mode*/
    int authenticated;                    /*!< Indicates if the session user has already been successfully authenticated*/
    int secure;                           /*!< Indicates if the session is in safe mode*/
    int pbsz_sent;                        /*!< Indicates that the pbsz command has already been sent*/
    TLS *context;                         /*!< TLS session context*/
    char *current_pkey;                   /*!< Clave*/
    char current_dir[MAX_PATH];           /*!< Current directory of the session user*/
    int n_attributes;                     /*!< Number of attributes currently in session*/
    attribute attributes[MAX_ATTRIBUTES]; /*!< Session variable attributes*/
    int clt_fd;                           /*!< Client file descriptor (control connection)*/
} session_info;

/**
 * @brief Add an attribute
 *
 * @param session FTP session
 * @param name Name of the attribute
 * @param val Value of the attribute, possibly a pointer or integer
 * @param freeable Indicates whether the attribute can be freed with a simple call to free
 * @param expiration Indicates how many control requests the attribute can hold
 * @return uintptr_t If the attribute was already there and cannot be freed, it returns the previous value,
 * else return ATTR_NOT_FOUND
 */
uintptr_t set_attribute(session_info *session, char *name, uintptr_t val, char freeable, short expiration);

/**
 * @brief Collect an attribute
 *
 * @param session FTP session
 * @param name Name of the attribute
 * @return uintptr_t attribute value or ATTR_NOT_FOUND
 */
uintptr_t get_attribute(session_info *session, char *name);

/**
 * @brief Free those attributes that can be freed with free
 *
 * @param session FTP session
 * @return int Number of released attributes
 */
int free_attributes(session_info *session);

/**
 * @brief Initialize a session
 *
 * @param session FTP session
 * @param previous_session Previous FTP session from which the attributes are inherited
 */
void init_session_info(session_info *session, session_info *previous_session);

/*USED ​​ATTRIBUTES*/
#define USERNAME_ATTR "usr"     /*!< Username of a USER command*/
#define RENAME_FROM_ATTR "rnfr" /*!< Filename to rename*/
#endif