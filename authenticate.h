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
 * @brief Verifica que user es igual al establecido en set_credentials
 * 
 * @param user usuario
 * @return int 1 si exito, 0 si no
 */
int validate_user(char *user);

/**
 * @brief Comprueba que el usuario y contraseña es correcto
 * 
 * @param pass contraseña, debe estar almacenada en /etc/shadow
 * @return int 1 si autenticacion correcta, 0 si no
 */
int validate_pass(char *pass);

/**
 * @brief Establece unos credenciales, con valor por defecto los de el que ejecutaa el programa
 * 
 * @param user Pasar NULL si se quiere usar usuario que ejecuta el programa
 * @param pass Pasar NULL si se quiere usar usuario que ejecuta el programa
 * @return int 1 si exito, 0 si no
 */
int set_credentials(char *user, char *pass);

int drop_root_privileges();

#endif /* AUTHENTICATE_H */