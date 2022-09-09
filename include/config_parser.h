/**
 * @fileconfig_parser.h
 * @author Joaquín Jiménez López de Castro (joaquin.jimenezl@estudiante.uam.es)
 * @brief Definition of useful functions for parsing the server.conf file and requests
 * @version 1.0
 * @date 02-16-2020
 *
 */

#ifndef PARSER_H
#define PARSER_H
#include "utils.h"
#include "network.h"

#define CONF_FILE "server.conf" /*!< configuration file*/

#define SERVER_ROOT "server_root" /*!< Base directory field*/
#define SERVER_ROOT_DEFAULT "~/"  /*!< Default value base directory*/
#define SERVER_ROOT_MAX XL_SZ + 1 /*!< Maximum size of root server*/

#define MAX_PASSIVE_PORTS "max_passive_ports" /*!< Field of maximum number of open ports in passive mode*/
#define MAX_PASSIVE_PORTS_DEFAULT 100         /*!< Default value of the maximum number of ports in passive mode*/

#define FTP_USER "ftp_user"       /*!< Username field to assign to the ftp server*/
#define FTP_USER_DEFAULT ""       /*!< Default value*/
#define FTP_USER_MAX SMALL_SZ + 1 /*!< Maximum ftp user size*/

#define FTP_HOST "ftp_host"          /*!< Host field where the FTP server is deployed*/
#define FTP_HOST_DEFAULT "localhost" /*!< Default value of the deployment host*/
#define FTP_HOST_MAX MEDIUM_SZ + 1   /*!< Maximum size name or host ip*/

#define MAX_SESSIONS "max_sessions" /*!< Field for maximum sessions*/
#define MAX_SESSIONS_DEFAULT 100    /*!< Default value for the max_sessions field*/

#define DAEMON_MODE "daemon_mode" /*!< Field for daemon mode*/
#define DAEMON_MODE_DEFAULT 0     /*!< Default value for daemon mode*/

#define TYPE "default_type"  /*!< Field for default type of data transmission*/
#define TYPE_DEFAULT "ascii" /*!< Default value for the field*/

#define CERTIFICATE_PATH "certificate_path" /*!< Path field to the certificate*/
#define CERTIFICATE_PATH_DEFAULT ""         /*!< Default value path to certificate*/
#define CERTIFICATE_PATH_MAX XL_SZ + 1      /*!< Maximum size of path to certificate*/

#define PRIVATE_KEY_PATH "private_key_path" /*!< Path field to private key file*/
#define PRIVATE_KEY_PATH_DEFAULT ""         /*!< Default value base directory*/
#define PRIVATE_KEY_PATH_MAX XL_SZ + 1      /*!< Maximum size of server path to private key file*/
/**
 * @brief Contains general information about the server, which includes the information parsed in server.conf
 *
 */
typedef struct _serverconf
{
    char server_root[SERVER_ROOT_MAX];           /*!< Root of the path where the files are searched*/
    int max_passive_ports;                       /*!< Maximum number of ports that can be opened in passive mode*/
    sem_t free_passive_ports;                    /*!< How many more data ports can be opened*/
    char ftp_user[FTP_USER_MAX];                 /*!< Username associated with the ftp server*/
    char ftp_host[FTP_HOST_MAX];                 /*!< Host where the server will be deployed*/
    int max_sessions;                            /*!< Maximum concurrent FTP sessions*/
    int default_ascii;                           /*!< Indicates if the default mode is ascii or not*/
    TLS *server_ctx;                             /*!< Contexto TLS general*/
    char certificate_path[CERTIFICATE_PATH_MAX]; /*!< Path to the x.509 certificate*/
    char private_key_path[PRIVATE_KEY_PATH_MAX]; /*!< Path to file with private key*/
    int daemon_mode;                             /*!< Indicates whether to run in daemon mode*/
} serverconf;

/**
 * @brief Parse the information from the server.conf file to configure the server at startup
 *
 * @param server_conf Reference to the structure that must be filled with the information from the server.conf file
 * @return int -1 if error, 1 if parsed successfully
 */
int parse_server_conf(serverconf *server_conf);

#endif /*PARSER_H*/