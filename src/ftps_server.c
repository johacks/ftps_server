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
#include "ftp.h"
#include "authenticate.h"
#include "config_parser.h"

#define MAX_PASSWORD MEDIUM_SZ
#define FTP_CONTROL_PORT 21
#define FTP_DATA_PORT 20
#define USING_AUTHBIND "--using-authbind"

void free_resources(int socket_control_fd, int socket_data_fd);
void set_ftp_credentials();

int modo_daemon = 0; /* TODO: Añadir al fichero de configuracion */
serverconf server_conf;

int main(int argc, char *argv[])
{
    /* Comprobacion un poco burda de que la ejecucion debe ser con authbind */
    if (!(argc == 2 && !strcmp(argv[1], USING_AUTHBIND)))
        execlp("authbind", "authbind", argv[0], USING_AUTHBIND, NULL);

    /* Leer la configuracion del servidor */
    if ( parse_server_conf(&server_conf) < 0 )
        errexit("Fallo al procesar fichero de configuracion\n");

    /* Establecer los credenciales del servidor y quitarse permisos de root si fueron dados */
    set_ftp_credentials();

    /* Ahora, habria que crear el socket de control */
    int socket_control_fd = socket_srv("tcp", 10, FTP_CONTROL_PORT, server_conf.ftp_host);
    if ( socket_control_fd < 0 )
        errexit("Fallo al abrir socket de control %s\n", strerror(errno));

    set_log_conf(1, 0, NULL); /* Establecer configuracion de logueo */

    /* Recibir sesiones */
    char buff[XL_SZ + 1];
    struct sockaddr clt_info;
    request_info ri;
    size_t clt_info_size = sizeof(clt_info);

    int clt_fd = accept(socket_control_fd, &clt_info, (socklen_t *) &clt_info_size);
    send(clt_fd, "220 Bienvenido a mi servidor FTP\n", sizeof("220 Bienvenido a mi servidor FTP\n"), 0);
    ssize_t read_b = recv(clt_fd, buff, XL_SZ, 0);
    buff[read_b] = '\0';
    parse_ftp_command(&ri, buff);
    flog(0, "%s\n%s\n%d\n%d\n", ri.command_name, ri.command_arg, ri.implemented_command, ri.ignored_command);
    free_resources(socket_control_fd, -1);
}

/**
 * @brief Establece los credenciales del servidor ftp en base al fichero de configuracion
 * Si se proporciono un nombre de usuario, se pedira una contraseña para dicho usuario
 * Si no se proporciono, se usaran los credenciales del usuario que ejecuta el programa, 
 * por lo que la ejecucion debera ser con permisos de root.
 * Al finalizar, se quitan los permisos de root por motivos de seguridad
 */
void set_ftp_credentials()
{
    /* Decidir metodo de autenticacion del servidor */
    if ( server_conf.ftp_user[0] == '\0' )
    {
        printf("Usuario no especificado en server.conf, se usaran las credenciales del usuario %s\n", getlogin());
        /* Si no se proporciona usuario, usar credenciales de usuario que ejecuta el programa */
        if ( !set_credentials(NULL, NULL) ) 
            errexit("Fallo al establecer credenciales de usuario que ejecuta el programa. Comprobar permisos de root\n");
        /* Quitarnos permisos de superusuario si los teniamos */
        drop_root_privileges();
    }
    else /* Pedir una contraseña para el usuario proporcionado */
    {
        char pass[MAX_PASSWORD + 1], pass_repeat[MAX_PASSWORD + 1];
        char *password; /* Necesario para usar en get_password */
        /* Solicitar una contraseña para el usuario proporcionado */
        do
        {
            password = pass;
            do
                printf("Establezca una contraseña para el usuario '%s':\n", server_conf.ftp_user);
            while ( get_password((char **) &password, MAX_PASSWORD) <= 0 );
            password = pass_repeat;
            do
                printf("Repita la contraseña para el usuario '%s':\n", server_conf.ftp_user);
            while ( get_password((char **) &password, MAX_PASSWORD) <= 0 );
        } while ( strcmp(pass, pass_repeat) && printf("Las contraseñas no coinciden\n") );
        /* Usar credenciales proporcionados */
        if ( !set_credentials(server_conf.ftp_user, pass) )
            errexit("Fallo al establecer credenciales de usuario que ejecuta el programa. Comprobar permisos de root\n");
        /* Limpiar contraseñas */
        memset(pass, '\0', MAX_PASSWORD);
        memset(pass_repeat, '\0', MAX_PASSWORD);
    }
}

void free_resources(int socket_control_fd, int socket_data_fd)
{
    if ( socket_control_fd > 0 ) close(socket_control_fd);
    if ( socket_data_fd > 0 ) close(socket_data_fd);
    return;
}