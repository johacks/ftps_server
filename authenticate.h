/**
 * @file authenticate.h
 * @author Joaquín Jiménez López de Castro (joaquin.jimenezl@estudiante.uam.es)
 * @brief 
 * @version 1.0
 * @date 17-03-2020
 * 
 * @copyright Copyright (c) 2020
 * 
 */

#ifndef AUTHENTICATE_H
#define AUTHENTICATE_H

/**
 * @brief Verifica que user es igual al nombre de usuario que ejecuta el proceso
 * 
 * @param user usuario
 * @return int 1 si exito, 0 si no
 */
int validate_user(char *user);

/**
 * @brief Comprueba que el usuario y contraseña es correcto
 * 
 * @param user nombre del usuario a autenticar
 * @param pass contraseña, debe estar almacenada en /etc/shadow
 * @return int 1 si autenticacion correcta, 0 si no
 */
int authenticate(char *user, char *pass);

#endif /* AUTHENTICATE_H */