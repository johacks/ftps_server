/**
 * @file ftp_session.c
 * @author Joaquín Jiménez López de Castro (joaquin.jimenezl@estudiante.uam.es)
 * @brief Implements ftp session handling functions
 * @version 1.0
 * @date 04-13-2020
 *
 * @copyright Copyright (c) 2020
 *
 */

#include "ftp_session.h"

/**
 * @brief Initialize a session
 *
 * @param session FTP session
 * @param previous_session Previous FTP session from which the attributes are inherited, its attributes are released
 */
void init_session_info(session_info *session, session_info *previous_session)
{
    session->n_attributes = 0;
    if (!previous_session) /*Starting initials*/
    {
        session->authenticated = 0;
        session->context = NULL;
        session->secure = 0;
        session->pbsz_sent = 1;
        strcpy(session->current_dir, "/");
        return;
    }
    /*Inherit attributes from the previous session*/
    session->authenticated = previous_session->authenticated;
    session->secure = previous_session->secure;
    session->pbsz_sent = previous_session->pbsz_sent;
    session->context = previous_session->context;
    session->ascii_mode = previous_session->ascii_mode;
    strcpy(session->current_dir, previous_session->current_dir);
    /*Inherit volatile attributes from the previous session*/
    for (int i = 0; i < previous_session->n_attributes; i++)
    {
        attribute *attr = &(session->attributes[session->n_attributes]), *p_attr = &(previous_session->attributes[i]);
        attr->expire = p_attr->expire - 1;
        /*If the attribute does not expire, we fill it*/
        if (attr->expire >= 0)
        {
            session->n_attributes++;
            strcpy(attr->name, p_attr->name);
            attr->freeable = p_attr->freeable;
            attr->val = p_attr->val;
        }
        /*If not, we release the value of the attribute, if applicable*/
        else if (p_attr->freeable)
            free((void *)p_attr->val);
    }
    return;
}

/**
 * @brief Add an attribute
 *
 * @param session FTP session
 * @param name Name of the attribute
 * @param val Value of the attribute, possibly a pointer or integer
 * @param freeable Indicates whether the attribute can be freed with a simple call to free
 * @param expiration Indicates how many control requests the attribute can hold
 * @return uintptr_t If the attribute was already there and cannot be freed, it returns the previous value,
 *(after overriding) else return ATTR_NOT_FOUND
 */
uintptr_t set_attribute(session_info *session, char *name, uintptr_t val, char freeable, short expiration)
{
    attribute *current_attr = &(session->attributes[0]);
    int i;
    /*Find the position to replace*/
    for (i = 0;
         i < session->n_attributes && strncmp(current_attr->name, name, MAX_ATTRIBUTE_NAME);
         current_attr = &(session->attributes[++i]))
        ;

    current_attr->expire = expiration;
    /*If the attribute was already there, release it if possible and if not, return it to the user*/
    if (i < session->n_attributes)
    {
        uintptr_t prev = ATTR_NOT_FOUND;
        if (current_attr->freeable)
            free((void *)current_attr->val);
        else
            prev = current_attr->val;
        current_attr->val = val;
        current_attr->freeable = freeable;
        return prev;
    }
    /*New attribute*/
    session->n_attributes++;
    current_attr->val = val;
    current_attr->freeable = freeable;
    strcpy(current_attr->name, name);
    return ATTR_NOT_FOUND;
}

/**
 * @brief Collect an attribute
 *
 * @param session FTP session
 * @param name Name of the attribute
 * @return uintptr_t attribute value or ATTR_NOT_FOUND
 */
uintptr_t get_attribute(session_info *session, char *name)
{
    /*Linear search of the attribute*/
    for (int i = 0; i < session->n_attributes; i++)
        if (!strcmp(session->attributes[i].name, name))
            return session->attributes[i].val;
    return ATTR_NOT_FOUND;
}

/**
 * @brief Free those attributes that can be freed with free
 *
 * @param session FTP session
 * @return int Number of released attributes
 */
int free_attributes(session_info *session)
{
    attribute *attr = &(session->attributes[0]);
    int nfreed, i;
    /*Linear search freeing attributes*/
    for (nfreed = 0, i = 0; i < session->n_attributes; i++, attr = &(session->attributes[i]))
    {
        if (attr->freeable)
        {
            nfreed++;
            free((void *)attr->val);
        }
    }
    /*Close possible data connection and data socket*/
    sclose(&(session->data_connection->context), &(session->data_connection->conn_fd));
    sclose(NULL, &(session->data_connection->socket_fd));
    return nfreed;
}