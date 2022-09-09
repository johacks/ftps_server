/**
 * @file authenticate.h
 * @author Joaquín Jiménez López de Castro (joaquin.jimenezl@estudiante.uam.es)
 * @brief Prototypes of functions related to user authentication
 * @version 1.0
 * @date 03-17-2020
 *
 */

#ifndef AUTHENTICATE_H
#define AUTHENTICATE_H

/**
 * @brief Get a password without doing echo by terminal
 *
 * @param target where the result will be saved
 * @param n maximum size
 * @return ssize_t bytes read
 */
ssize_t get_password(char **target, size_t n);

/**
 * @brief Verifies that user is equal to the one set in set_credentials
 *
 * @param user user
 * @return int 1 if successful, 0 if not
 */
int validate_user(char *user);

/**
 * @brief Check that the username and password is correct
 *
 * @param pass password, must be stored in /etc/shadow
 * @return int 1 if successful authentication, 0 if not
 */
int validate_pass(char *pass);

/**
 * @brief Sets some credentials, with default value those of the one executing the program
 *
 * @param user Pass NULL if you want to use the user that runs the program
 * @param pass Pass NULL if you want to use the user that runs the program
 * @return int 1 if successful, 0 if not
 */
int set_credentials(char *user, char *pass);

/**
 * @brief Remove root permissions
 * Obtained from: https://stackoverflow.com/questions/3357737/dropping-root-privileges
 * @return int 2 if not root, 1 on success, 0 on error
 */
int drop_root_privileges();

/**
 * @brief Name of the user running the program
 *
 * @return char*Read only
 */
char *get_username();

#endif /*AUTHENTICATE_H*/