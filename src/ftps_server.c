/**
 * @file ftps_server.c
 * @author Joaquín Jiménez López de Castro (joaquin.jimenezl@estudiante.uam.es)
 * @brief Implementación de servidor ftps
 * @version 1.0
 * @date 07-04-2020
 * 
 * 
 */

#include "utils.h"
#include "red.h"
#include "authenticate.h"
#include "config_parser.h"

#define MAX_PASSWORD MEDIUM_SZ

serverconf server_conf;

int main(void)
{
    /* Leer la configuracion del servidor */
    if ( parse_server_conf(&server_conf) < 0 )
        errexit("Fallo al procesar fichero de configuracion\n");
    
    /* Decidir metodo de autenticacion del servidor */
    if ( server_conf.ftp_user[0] == '\0' )
    {
        /* Si no se proporciona usuario, usar credenciales de usuario que ejecuta el programa */
        if ( !set_credentials(NULL, NULL) ) 
            errexit("Fallo al establecer credenciales de usuario que ejecuta el programa. Comprobar permisos de root\n");
    }
    else
    {
        char password[MAX_PASSWORD + 1];
        /* Solicitar una contraseña para el usuario proporcionado */
        do
        {
            printf("Introduzca una contraseña para el usuario '%s':\n", server_conf.ftp_user);
        } while ( get_password(&password, MAX_PASSWORD) <= 0 );
        /* Usar credenciales proporcionados */
        if ( !set_credentials(NULL, NULL) ) 
            errexit("Fallo al establecer credenciales de usuario que ejecuta el programa. Comprobar permisos de root\n");
    }

    
    
}