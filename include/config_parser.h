/**
 * @file parser.h
 * @author Joaquín Jiménez López de Castro (joaquin.jimenezl@estudiante.uam.es)
 * @brief Definicion de funciones útiles para el parseo del fichero server.conf y de requests
 * @version 1.0
 * @date 16-02-2020
 * 
 */

#ifndef PARSER_H
#define PARSER_H
#include "utils.h"
#include "red.h"

#define CONF_FILE "server.conf" /*!< Fichero de configuracion */

#define SERVER_ROOT "server_root" /*!< Campo de directorio base */
#define SERVER_ROOT_DEFAULT "~/" /*!< Valor por defecto directorio base */
#define SERVER_ROOT_MAX XL_SZ + 1 /*!< Tamaño maximo de server root */

#define MAX_PASSIVE_PORTS "max_passive_ports" /*!< Campo de numero maximo de puertos abiertos en modo pasivo */
#define MAX_PASSIVE_PORTS_DEFAULT 100 /*!< Valor por defecto del maximo de puertos en modo pasivo */

#define FTP_USER "ftp_user" /*!< Campo de nombre de usuario a asginar al servidor ftp */
#define FTP_USER_DEFAULT "" /*!< Valor por defecto */
#define FTP_USER_MAX SMALL_SZ + 1 /*!< Tamaño maximo de usuario ftp */

#define FTP_HOST "ftp_host" /*!< Campo de host donde se despliega el servidor FTP */
#define FTP_HOST_DEFAULT "localhost" /*!< Valor por defecto del host de despliegue */ 
#define FTP_HOST_MAX MEDIUM_SZ + 1 /*!< Tamaño maximo nombre o ip de host */

#define MAX_SESSIONS "max_sessions" /*!< Campo para maximo de sesions */
#define MAX_SESSIONS_DEFAULT 100 /*!< Valor por defecto para el campo max_sessions */

#define TYPE "default_type" /*!< Campo para tipo por defecto de transmision de datos */
#define TYPE_DEFAULT "ascii" /*!< Valor por defecto para el campo */

#define CERTIFICATE_PATH "certificate_path" /*!< Campo de path al certificado */
#define CERTIFICATE_PATH_DEFAULT "" /*!< Valor por defecto path al certificado */
#define CERTIFICATE_PATH_MAX XL_SZ + 1 /*!< Tamaño maximo de path al certificado */

#define PRIVATE_KEY_PATH "private_key_path" /*!< Campo de path al fichero de clave privada */
#define PRIVATE_KEY_PATH_DEFAULT "" /*!< Valor por defecto directorio base */
#define PRIVATE_KEY_PATH_MAX XL_SZ + 1 /*!< Tamaño maximo de server path al fichero de clave privada */

typedef struct _serverconf
{
    char server_root[SERVER_ROOT_MAX]; /*!< Root del path donde se buscan los ficheros */
    int max_passive_ports; /*!< Numero maximo de puertos que se pueden abrir en modo pasivo */
    sem_t free_passive_ports; /*!< Cuantos mas puertos de datos se pueden abrir */
    char ftp_user[FTP_USER_MAX]; /*!< Nombre de usuario asociado al servidor ftp */
    char ftp_host[FTP_HOST_MAX]; /*!< Host donde se desplegara el servidor */
    int max_sessions; /*!< Maximo de sesiones FTP concurrentes */
    int default_ascii; /*!< Indica si el modo por defecto es ascii o no */
    TLS *server_ctx; /*!< Contexto TLS general */
    char certificate_path[CERTIFICATE_PATH_MAX]; /*!< Path al certificado x.509 */
    char private_key_path[PRIVATE_KEY_PATH_MAX]; /*!< Path al fichero con clave privada  */
} serverconf;

/**
 * @brief Parsea la informacion del fichero server.conf para configurar el servidor al iniciarse
 * 
 * @param server_conf Referencia a a estructura que hay que rellenar con la informacion del fichero server.conf
 * @return int -1 si error, 1 si se ha parseado correctamente
 */
int parse_server_conf(serverconf *server_conf);

#endif /* PARSER_H*/