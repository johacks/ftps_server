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

#define CONF_FILE "server.conf" /*!< Fichero de configuracion */
#define SERVER_ROOT "server_root" /*!< Directorio base */
#define SERVER_ROOT_DEFAULT "~/" /*!< Valor por defecto directorio base */
#define MAX_PASSIVE_PORTS "max_passive_ports" /*!< Numero maximo de puertos abiertos en modo pasivo */
#define MAX_PASSIVE_PORTS_DEFAULT 100 /*!< Valor por defecto del maximo de puertos en modo pasivo */
#define FTP_USER "ftp_user" /*!< Nombre de usuario a asginar al servidor ftp */
#define FTP_USER_DEFAULT "" /*!< Valor por defecto */
#define SERVER_ROOT_MAX MEDIUM_SZ + 1 /*!< Tamaño maximo de server root */
#define FTP_USER_MAX SMALL_SZ + 1 /* Tamaño maximo de usuario ftp */

typedef struct _serverconf
{
    char server_root[SERVER_ROOT_MAX]; /*!< Root del path donde se buscan los ficheros */
    int max_passive_ports; /*!< Numero maximo de puertos que se pueden abrir en modo pasivo */
    char ftp_user[FTP_USER_MAX]; /*!< Nombre de usuario asociado al servidor ftp */

} serverconf;

/**
 * @brief Parsea la informacion del fichero server.conf para configurar el servidor al iniciarse
 * 
 * @param server_conf Referencia a a estructura que hay que rellenar con la informacion del fichero server.conf
 * @return int -1 si error, 1 si se ha parseado correctamente
 */
int parse_server_conf(serverconf *server_conf);

#endif /* PARSER_H*/