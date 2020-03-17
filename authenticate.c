/**
 * @file authenticate.c
 * @author Joaquín Jiménez López de Castro (joaquin.jimenezl@estudiante.uam.es)
 * @brief Servicios de autenticacion
 * @version 1.0
 * @date 17-03-2020
 * 
 * @copyright Copyright (c) 2020
 * 
 */
#define _DEFAULT_SOURCE
#include <unistd.h>
#include <shadow.h>
#include <pwd.h>
#include <crypt.h>
#include <stdio.h>
#include <string.h>
#include "utils.h"

#define MAX_PASS_SIZE MEDIUM_SZ /*!< Tamaño máximo de contraseña */

/**
 * @brief Verifica que user es igual al nombre de usuario que ejecuta el proceso
 * 
 * @param user usuario
 * @return int 1 si exito, 0 si no
 */
int validate_user(char *user)
{
    return strncmp(user, getlogin(), MAX_PASS_SIZE) == 0;
}

/**
 * @brief Comprueba que el usuario y contraseña es correcto. Debe ejecutarse como root
 * 
 * @param user nombre del usuario a autenticar
 * @param pass contraseña, debe estar almacenada en /etc/shadow
 * @return int 1 si autenticacion correcta, 0 si no
 */
int authenticate(char *user, char *pass)
{
    /* Implementacion obtenida de: https://stackoverflow.com/questions/6536994/authentication-of-local-unix-user-using-c */
    struct passwd *pw;
    struct spwd *sp;
    char *encrypted, *correct;

    pw = getpwnam (user); /* Buscar el usuario proporcionado */
    endpwent(); /* Cerrar el stream del fichero de contraseñas */

    if ( !pw ) return 0; /* Usuario no existe */

    sp = getspnam (pw->pw_name); /* Rellenar la estructura spwd con la informacion adecuada */
    endspent(); /* Siguiente entrada fichero */
    if ( sp )
        correct = sp->sp_pwdp; /* Si donde busca getspnam estaba, usamos esa */
    else
        correct = pw->pw_passwd; /* Si no, usamos la que encuentra getpwnam */

    encrypted = crypt (pass, correct); /* Encriptar en formato shadow */
    return strcmp(encrypted, correct) ? 0 : 1; 
}