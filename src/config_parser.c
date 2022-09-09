/**
 * @fileconfig_parser.c
 * @author Joaquín Jiménez López de Castro (joaquin.jimenezl@estudiante.uam.es)
 * @brief Implementation of module parsing functions
 * @version 1.0
 * @date 04-07-2020
 *
 */

#define _DEFAULT_SOURCE
#include "network.h"
#include "utils.h"
#include "config_parser.h"
#include "confuse.h"

int get_server_root(serverconf *server_conf, cfg_t *cfg);
int get_ftp_user(serverconf *server_conf, cfg_t *cfg);
int get_max_passive_ports(serverconf *server_conf, cfg_t *cfg);
int get_ftp_host(serverconf *server_conf, cfg_t *cfg);
int get_max_sessions(serverconf *server_conf, cfg_t *cfg);
int get_type(serverconf *server_conf, cfg_t *cfg);
int get_certificate_path(serverconf *server_conf, cfg_t *cfg);
int get_daemon_mode(serverconf *server_conf, cfg_t *cfg);
int get_private_key_path(serverconf *server_conf, cfg_t *cfg);

/**
 * @brief Parse the information from the server.conf file to configure the server at startup
 *
 * @param server_conf Reference to the structure that must be filled with the information from the server.conf file
 * @return int -1 if error, 1 if parsed successfully
 */
int parse_server_conf(serverconf *server_conf)
{
    cfg_opt_t opts[] = {
        /*The first parameter indicates what to look for in the .conf.
        The second string is a default value if the file is empty.
        The last one is the variable where the value is going to be stored.
        */
        CFG_STR(SERVER_ROOT, SERVER_ROOT_DEFAULT, CFGF_NONE),
        CFG_INT(MAX_PASSIVE_PORTS, MAX_PASSIVE_PORTS_DEFAULT, CFGF_NONE),
        CFG_INT(MAX_SESSIONS, MAX_SESSIONS_DEFAULT, CFGF_NONE),
        CFG_INT(DAEMON_MODE, DAEMON_MODE_DEFAULT, CFGF_NONE),
        CFG_STR(TYPE, TYPE_DEFAULT, CFGF_NONE),
        CFG_STR(FTP_USER, FTP_USER_DEFAULT, CFGF_NONE),
        CFG_STR(FTP_HOST, FTP_HOST_DEFAULT, CFGF_NONE),
        CFG_STR(CERTIFICATE_PATH, CERTIFICATE_PATH_DEFAULT, CFGF_NONE),
        CFG_STR(PRIVATE_KEY_PATH, PRIVATE_KEY_PATH_DEFAULT, CFGF_NONE),
        CFG_END()};

    /*Initialize the configuration and parse the file*/
    cfg_t *cfg;
    cfg = cfg_init(opts, CFGF_NONE);
    if (cfg_parse(cfg, CONF_FILE) == CFG_PARSE_ERROR)
        return -1;

    /*The structure is filled with the information obtained from the server.conf file*/
    int res = 1 - 2 * (int)(get_server_root(server_conf, cfg) < 0 || get_ftp_user(server_conf, cfg) < 0 || get_max_passive_ports(server_conf, cfg) < 0 || get_ftp_host(server_conf, cfg) < 0 || get_type(server_conf, cfg) < 0 || get_private_key_path(server_conf, cfg) < 0 || get_certificate_path(server_conf, cfg) < 0 || get_daemon_mode(server_conf, cfg) < 0 || get_max_sessions(server_conf, cfg) < 0);
    cfg_free(cfg);
    return res;
}

/**
 * @brief Collect and clear default_type
 *
 * @param server_conf configuration structure
 * @param cfg Parsing results
 * @return int less than 0 on error
 */
int get_type(serverconf *server_conf, cfg_t *cfg)
{
    char *type = cfg_getstr(cfg, TYPE);
    if (!strcmp(type, "binary"))
        server_conf->default_ascii = 0;
    else if (!strcmp(type, "ascii"))
        server_conf->default_ascii = 1;
    else
        return -1;
    return 1;
}

/**
 * @brief Collect and clean server root
 *
 * @param server_conf configuration structure
 * @param cfg Parsing results
 * @return int less than 0 on error
 */
int get_server_root(serverconf *server_conf, cfg_t *cfg)
{
    char *path;
    char buff[SERVER_ROOT_MAX + 1];
    path = cfg_getstr(cfg, SERVER_ROOT);
    size_t path_len = strlen(path);

    /*CoE: path too long*/
    if (path_len >= SERVER_ROOT_MAX)
    {
        printf("Server root demasiado largo\n");
        return -1;
    }
    /*Special case of ~, replace with environment variable HOME*/
    if (path[0] == '~')
    {
        snprintf(buff, SERVER_ROOT_MAX, "%s%s", getenv("HOME"), (path_len > 1) ? &path[1] : "");
        path = buff;
    }
    /*For the path to server_root, we use the absolute path (man 3 realpath)*/
    if (!realpath(path, server_conf->server_root))
    {
        printf("Server root incorrecto\n");
        return -1;
    }
    return 1;
}

/**
 * @brief Collect and clean certificate path
 *
 * @param server_conf Server configuration
 * @param cfg Parsing result
 * @return int less than 0 on error
 */
int get_certificate_path(serverconf *server_conf, cfg_t *cfg)
{
    char *path = cfg_getstr(cfg, CERTIFICATE_PATH);
    size_t path_len = strlen(path);

    if (!path_len || path_len >= CERTIFICATE_PATH_MAX)
    {
        printf("Path al certificado %s\n", (path_len) ? "no proporcionado" : "demasiado largo");
        return -1;
    }
    if (!realpath(path, server_conf->certificate_path))
    {
        printf("Path al certificado incorrecto\n");
        return -1;
    }
    return 1;
}

/**
 * @brief Collect and clean private key path
 *
 * @param server_conf Server configuration
 * @param cfg Parsing result
 * @return int less than 0 on error
 */
int get_private_key_path(serverconf *server_conf, cfg_t *cfg)
{
    size_t path_len = strlen(cfg_getstr(cfg, PRIVATE_KEY_PATH));
    char *path = alloca(path_len + 1);
    strcpy(path, cfg_getstr(cfg, PRIVATE_KEY_PATH));

    if (!path_len || path_len >= PRIVATE_KEY_PATH_MAX)
    {
        printf("Path al fichero de clave privada %s\n", (path_len) ? "no proporcionado" : "demasiado largo");
        return -1;
    }
    server_conf->private_key_path[0] = '\0';
    if (!realpath(path, server_conf->private_key_path))
    {
        printf("Path al fichero de clave privada incorrecto\n");
        return -1;
    }
    return 1;
}

/**
 * @brief Collect and clean ftp user
 *
 * @param server_conf configuration structure
 * @param cfg Parsing results
 * @return int less than 0 on error
 */
int get_ftp_user(serverconf *server_conf, cfg_t *cfg)
{
    char *user;
    user = cfg_getstr(cfg, FTP_USER);
    /*CdE user too long*/
    if (strlen(user) >= FTP_USER_MAX)
    {
        printf("Usuario demasiado largo incorrecto\n");
        return -1;
    }
    strcpy(server_conf->ftp_user, user);
    return 1;
}

/**
 * @brief Collect and clean max passive ports
 *
 * @param server_conf configuration structure
 * @param cfg Parsing results
 * @return int less than 0 on error
 */
int get_max_passive_ports(serverconf *server_conf, cfg_t *cfg)
{
    server_conf->max_passive_ports = cfg_getint(cfg, MAX_PASSIVE_PORTS);
    /*CoE: at least one port must be allowed in passive mode*/
    if (server_conf->max_passive_ports <= 0)
        server_conf->max_passive_ports = MAX_PASSIVE_PORTS_DEFAULT;
    /*Open ports semaphore*/
    sem_init(&(server_conf->free_passive_ports), 0, server_conf->max_passive_ports);
    return 1;
}

/**
 * @brief Pick up and clean daemon mode
 *
 * @param server_conf configuration structure
 * @param cfg Parsing results
 * @return int less than 0 on error
 */
int get_daemon_mode(serverconf *server_conf, cfg_t *cfg)
{
    server_conf->daemon_mode = cfg_getint(cfg, DAEMON_MODE);
    return 1;
}

/**
 * @brief Collect and clean max sessions
 *
 * @param server_conf configuration structure
 * @param cfg Parsing results
 * @return int less than 0 on error
 */
int get_max_sessions(serverconf *server_conf, cfg_t *cfg)
{
    server_conf->max_sessions = cfg_getint(cfg, MAX_SESSIONS);
    /*CoE: must allow at least one session*/
    if (server_conf->max_sessions <= 0)
        server_conf->max_sessions = MAX_SESSIONS_DEFAULT;
    return 1;
}

/**
 * @brief Collect and clean ftp host
 *
 * @param server_conf configuration structure
 * @param cfg Parsing results
 * @return int less than 0 on error
 */
int get_ftp_host(serverconf *server_conf, cfg_t *cfg)
{
    char *host = cfg_getstr(cfg, FTP_HOST);
    struct hostent *he = gethostbyname(host); /*Resolve the name*/

    if (!he->h_addr_list[0])
    {
        printf("El host especificado no tiene direcciones asociadas\n");
        return -1;
    }
    /*Save the ip address in ascii*/
    strcpy(server_conf->ftp_host, inet_ntoa(*(struct in_addr *)(he->h_addr_list[0])));
    if (he->h_addr_list[1] != NULL)
        printf("El host especificado tiene varias direcciones asociadas, se cogera la primera: %s\n", server_conf->ftp_host);

    return 1;
}