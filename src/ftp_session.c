/**
 * @file ftp_session.c
 * @author Joaquín Jiménez López de Castro (joaquin.jimenezl@estudiante.uam.es)
 * @brief Implementa funciones de manejo de sesion ftp
 * @version 1.0
 * @date 13-04-2020
 * 
 * @copyright Copyright (c) 2020
 * 
 */

#include "ftp_session.h"

/**
 * @brief Inicializa una sesion
 * 
 * @param session Sesion FTP
 * @param previous_session Sesion FTP a anterior de la que se heredan los atributos, se liberan sus atributos
 */
void init_session_info(session_info *session, session_info *previous_session)
{
    session->n_attributes = 0;
    if ( !previous_session ) /* Valores iniciales */
    {
        session->authenticated = 0;
        session->context = NULL;
        session->secure = 0;
        session->pbsz_sent = 1;
        strcpy(session->current_dir, "/");
        return;
    }
    /* Heredar atributos de la sesion anterior */
    session->authenticated = previous_session->authenticated;
    session->secure = previous_session->secure;
    session->pbsz_sent = previous_session->pbsz_sent;
    session->context = previous_session->context;
    session->ascii_mode = previous_session->ascii_mode;
    strcpy(session->current_dir, previous_session->current_dir);
    /* Heredar atributos volatiles de la sesion anterior */
    for ( int i = 0; i < previous_session->n_attributes; i++)
    {
        attribute *attr = &(session->attributes[session->n_attributes]), *p_attr = &(previous_session->attributes[i]);
        attr->expire = p_attr->expire - 1;
        /* Si el atributo no expira, lo rellenamos */
        if ( attr->expire >= 0 )
        {
            session->n_attributes++;
            strcpy(attr->name, p_attr->name);
            attr->freeable = p_attr->freeable;
            attr->val = p_attr->val;
        }
        /* Si no, liberamos el valor del atributo, si corresponde */
        else if ( p_attr->freeable )
            free((void *) p_attr->val);
    }
    return;
}

/**
 * @brief Añadir un atributo
 * 
 * @param session Sesion FTP
 * @param name Nombre del atributo
 * @param val Valor del atributo, posiblemente un puntero o entero
 * @param freeable Indica si el atributo puede liberarse con una simple llamada a free
 * @param expiration Indica cuantas peticiones de control aguanta el atributo
 * @return uintptr_t Si el atributo ya estaba y no se puede liberar, devuelve el valor anterior,
 *         (tras sobreescribirlo) si no, devuelve ATTR_NOT_FOUND
 */
uintptr_t set_attribute(session_info *session, char *name, uintptr_t val, char freeable, short expiration)
{
    attribute *current_attr = &(session->attributes[0]);
    int i;
    /* Buscar la posicion a reemplazar */
    for (   i = 0;
            i < session->n_attributes && strncmp(current_attr->name, name, MAX_ATTRIBUTE_NAME);
            current_attr = &(session->attributes[++i]));
    
    current_attr->expire = expiration;
    /* Si el atributo ya estaba, liberarlo si se puede y si no, devolverlo al usuario */
    if ( i < session->n_attributes )
    {
        uintptr_t prev = ATTR_NOT_FOUND;
        if ( current_attr->freeable )
            free((void *) current_attr->val);
        else
            prev = current_attr->val;
        current_attr->val = val;
        current_attr->freeable = freeable;
        return prev;
    }
    /* Nuevo atributo */
    session->n_attributes++;
    current_attr->val = val;
    current_attr->freeable = freeable;
    strcpy(current_attr->name, name);
    return ATTR_NOT_FOUND;
}

/**
 * @brief Recoger un atributo
 * 
 * @param session Sesion FTP
 * @param name Nombre del atributo
 * @return uintptr_t Valor del atributo o ATTR_NOT_FOUND
 */
uintptr_t get_attribute(session_info *session, char *name)
{
    /* Busqueda lineal del atributo */
    for (int i = 0; i < session->n_attributes; i++)
        if ( !strcmp(session->attributes[i].name, name) )
            return session->attributes[i].val;
    return ATTR_NOT_FOUND;
}

/**
 * @brief Libera aquellos atributos que se pueden liberar con free
 * 
 * @param session Sesion FTP
 * @return int Numero de atributos liberados
 */
int free_attributes(session_info *session)
{
    attribute *attr = &(session->attributes[0]);
    int nfreed, i;
    /* Busqueda lineal liberando atributos */
    for ( nfreed = 0, i = 0; i < session->n_attributes; i++, attr = &(session->attributes[i]))
    {
        if ( attr->freeable )
        {
            nfreed++;
            free((void *) attr->val);
        }
    }
    /* Cerrar posible conexion de datos y socket de datos */
    sclose(&(session->data_connection->context), &(session->data_connection->conn_fd));
    sclose(NULL, &(session->data_connection->socket_fd));
    return nfreed;
}