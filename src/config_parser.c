/**
 * @file config_parser.c
 * @author Joaquín Jiménez López de Castro (joaquin.jimenezl@estudiante.uam.es)
 * @brief 
 * @version 1.0
 * @date 07-04-2020
 * 
 */

#define _DEFAULT_SOURCE
#include <confuse.h>
#include "red.h"
#include "utils.h"
#include "config_parser.h"

int get_server_root(serverconf *server_conf, cfg_t *cfg);
int get_ftp_user(serverconf *server_conf, cfg_t *cfg);
int get_max_passive_ports(serverconf *server_conf, cfg_t *cfg);
int get_ftp_host(serverconf *server_conf, cfg_t *cfg);
int get_max_sessions(serverconf *server_conf, cfg_t *cfg);
int get_type(serverconf *server_conf, cfg_t *cfg);
int get_certificate_path(serverconf *server_conf, cfg_t *cfg);
int get_private_key_path(serverconf *server_conf, cfg_t *cfg);

/**
 * @brief Parsea la informacion del fichero server.conf para configurar el servidor al iniciarse
 * 
 * @param server_conf Referencia a a estructura que hay que rellenar con la informacion del fichero server.conf
 * @return int -1 si error, 1 si se ha parseado correctamente
 */
int parse_server_conf(serverconf *server_conf)
{
	cfg_opt_t opts[] = {
		/* El primer parametro indica lo que va a bucar en el .conf.
           El segundo string es un valor por defecto si el fichero esta vacio.
           El ultimo es la variable donde se va a guardar el valor
        */
        CFG_STR(SERVER_ROOT, SERVER_ROOT_DEFAULT, CFGF_NONE),	
        CFG_INT(MAX_PASSIVE_PORTS, MAX_PASSIVE_PORTS_DEFAULT, CFGF_NONE),
        CFG_INT(MAX_SESSIONS, MAX_SESSIONS_DEFAULT, CFGF_NONE),
        CFG_STR(TYPE, TYPE_DEFAULT, CFGF_NONE),
        CFG_STR(FTP_USER, FTP_USER_DEFAULT, CFGF_NONE),
        CFG_STR(FTP_HOST, FTP_HOST_DEFAULT, CFGF_NONE),
        CFG_STR(CERTIFICATE_PATH, CERTIFICATE_PATH_DEFAULT, CFGF_NONE),
        CFG_STR(PRIVATE_KEY_PATH, PRIVATE_KEY_PATH_DEFAULT, CFGF_NONE),
        CFG_END()
    };

    /* Inicializar la configuracion y parsear el fichero */
    cfg_t *cfg;
    cfg = cfg_init(opts, CFGF_NONE);
    if ( cfg_parse(cfg, CONF_FILE) == CFG_PARSE_ERROR )
    	return -1;

    /* Se rellena la estructura con la informacion obtenida del fichero server.conf */
    int res = 1 - 2 * (int) (get_server_root(server_conf, cfg) < 0
                          || get_ftp_user(server_conf, cfg) < 0
                          || get_max_passive_ports(server_conf, cfg) < 0
                          || get_ftp_host(server_conf, cfg) < 0
                          || get_type(server_conf, cfg) < 0
                          || get_private_key_path(server_conf, cfg) < 0
                          || get_certificate_path(server_conf, cfg) < 0
                          || get_max_sessions(server_conf, cfg) < 0);
    cfg_free(cfg);
    return res;
}

/**
 * @brief Recoge y limpia default_type
 * 
 * @param server_conf Estructura de configuracion
 * @param cfg Resultados del parseo
 * @return int menor que 0 si error
 */
int get_type(serverconf *server_conf, cfg_t *cfg)
{
    char *type = cfg_getstr(cfg, TYPE);
    if ( !strcmp(type, "binary") )
        server_conf->default_ascii = 0;
    else if ( !strcmp(type, "ascii") )
        server_conf->default_ascii = 1;
    else
        return -1;
    return 1;
}

/**
 * @brief Recoge y limpia server root
 * 
 * @param server_conf Estructura de configuracion
 * @param cfg Resultados del parseo
 * @return int menor que 0 si error
 */
int get_server_root(serverconf *server_conf, cfg_t *cfg)
{
    char *path;
    char buff[SERVER_ROOT_MAX + 1];
    path = cfg_getstr(cfg, SERVER_ROOT);
    size_t path_len = strlen(path);

    /* CdE: path demasiado largo */
    if ( path_len >= SERVER_ROOT_MAX )
    {
        printf("Server root demasiado largo\n");
        return -1;
    }
    /* Caso especial de ~, reemplazar con variable de entorno HOME */
    if ( path[0] == '~' )
    {
        snprintf(buff, SERVER_ROOT_MAX, "%s%s", getenv("HOME"), ( path_len > 1 ) ? &path[1] : "");
        path = buff;
    }
    /* Para el path a server_root, usamos el path absoluto (man 3 realpath) */
    if ( !realpath(path, server_conf->server_root) )
    {
        printf("Server root incorrecto\n");
        return -1;
    }
    return 1;
}

/**
 * @brief Recoge y limpia certificate path
 * 
 * @param server_conf Configuracion del servidor
 * @param cfg Resultado del parseo
 * @return int menor que 0 si error
 */
int get_certificate_path(serverconf *server_conf, cfg_t *cfg)
{
    char *path = cfg_getstr(cfg, CERTIFICATE_PATH);
    size_t path_len = strlen(path);

    if ( !path_len || path_len >= CERTIFICATE_PATH_MAX )
    {
        printf("Path al certificado %s\n", (path_len) ? "no proporcionado" : "demasiado largo");
        return -1;
    }
    if ( !realpath(path, server_conf->certificate_path) )
    {
        printf("Path al certificado incorrecto\n");
        return -1;
    }
    return 1;
}

/**
 * @brief Recoge y limpia private key path
 * 
 * @param server_conf Configuracion del servidor
 * @param cfg Resultado del parseo
 * @return int menor que 0 si error
 */
int get_private_key_path(serverconf *server_conf, cfg_t *cfg)
{
    char *path = cfg_getstr(cfg, PRIVATE_KEY_PATH);
    size_t path_len = strlen(path);

    if ( !path_len || path_len >= PRIVATE_KEY_PATH_MAX )
    {
        printf("Path al fichero de clave privada %s\n", (path_len) ? "no proporcionado" : "demasiado largo");
        return -1;
    }
    if ( !realpath(path, server_conf->private_key_path) )
    {
        printf("Path al fichero de clave privada incorrecto\n");
        return -1;
    }
    return 1;
}

/**
 * @brief Recoge y limpia ftp user
 * 
 * @param server_conf Estructura de configuracion
 * @param cfg Resultados del parseo
 * @return int menor que 0 si error
 */
int get_ftp_user(serverconf *server_conf, cfg_t *cfg)
{
    char *user;
    user = cfg_getstr(cfg, FTP_USER);
    /* CdE usuario demsiado largo */
    if ( strlen(user) >= FTP_USER_MAX )
    {
        printf("Usuario demasiado largo incorrecto\n");
        return -1;
    }
    strcpy(server_conf->ftp_user, user);
    return 1;
}

/**
 * @brief Recoge y limpia max passive ports
 * 
 * @param server_conf Estructura de configuracion
 * @param cfg Resultados del parseo
 * @return int menor que 0 si error
 */
int get_max_passive_ports(serverconf *server_conf, cfg_t *cfg)
{
    server_conf->max_passive_ports = cfg_getint(cfg, MAX_PASSIVE_PORTS);
    /* CdE: tiene que permitirse al menos un puerto en modo pasivo */
    if ( server_conf->max_passive_ports <= 0 )
        server_conf->max_passive_ports = MAX_PASSIVE_PORTS_DEFAULT;
    /* Semaforo de puertos abiertos */
    sem_init(&(server_conf->free_passive_ports), 0, server_conf->max_passive_ports);
    return 1;
}

/**
 * @brief Recoge y limpia max sessions
 * 
 * @param server_conf Estructura de configuracion
 * @param cfg Resultados del parseo
 * @return int menor que 0 si error
 */
int get_max_sessions(serverconf *server_conf, cfg_t *cfg)
{
    server_conf->max_sessions = cfg_getint(cfg, MAX_SESSIONS);
    /* CdE: tiene que permitirse al menos una sesion */
    if ( server_conf->max_sessions <= 0 )
        server_conf->max_sessions = MAX_SESSIONS_DEFAULT;
    return 1;
}

/**
 * @brief Recoge y limpia ftp host
 * 
 * @param server_conf Estructura de configuracion
 * @param cfg Resultados del parseo
 * @return int menor que 0 si error
 */
int get_ftp_host(serverconf *server_conf, cfg_t *cfg)
{
    char *host = cfg_getstr(cfg, FTP_HOST);
    struct hostent *he = gethostbyname(host); /* Resuelve el nombre */

    if ( !he->h_addr_list[0] )
    {
        printf("El host especificado no tiene direcciones asociadas\n");
        return -1;
    }
    /* Guarda en ascii la direccion ip */
    strcpy(server_conf->ftp_host, inet_ntoa( *( struct in_addr*)( he -> h_addr_list[0])));
    if ( he->h_addr_list[1] != NULL )
        printf("El host especificado tiene varias direcciones asociadas, se cogera la primera: %s\n", server_conf->ftp_host);
    
    return 1;
}