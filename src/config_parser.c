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

/**
 * @brief Parsea la informacion del fichero server.conf para configurar el servidor al iniciarse
 * 
 * @param server_conf Referencia a a estructura que hay que rellenar con la informacion del fichero server.conf
 * @return int -1 si error, 1 si se ha parseado correctamente
 */
int parse_server_conf(serverconf *server_conf)
{
    char *path, user;

    /* CdE: recibe como argumento la estructura */
	if ( !server_conf )
		return -1;

	cfg_opt_t opts[] = {
		/* El primer parametro indica lo que va a bucar en el .conf.
           El segundo string es un valor por defecto si el fichero esta vacio.
           El ultimo es la variable donde se va a guardar el valor
        */
        CFG_STR(SERVER_ROOT, SERVER_ROOT_DEFAULT, CFGF_NONE),	
        CFG_INT(MAX_PASSIVE_PORTS, MAX_PASSIVE_PORTS_DEFAULT, CFGF_NONE),
        CFG_STR(FTP_USER, FTP_USER_DEFAULT, CFGF_NONE),
        CFG_END()
    };

    /* Inicializar la configuracion y parsear el fichero */
    cfg_t *cfg;

    cfg = cfg_init(opts, CFGF_NONE);

    if ( cfg_parse(cfg, CONF_FILE) == CFG_PARSE_ERROR )
    	return -1;

    /* Se rellena la estructura con la informacion obtenida del fichero server.conf */

    /* SERVER_ROOT */
    path = cfg_getstr(cfg, SERVER_ROOT);
    /* CdE: path demasiado largo */
    if ( strlen(path) >= SERVER_ROOT_MAX )
    {
        printf("Server root demasiado largo\n");
        return -1;
    }
    /* Para el path a server_root, usamos el path absoluto (man 3 realpath) */
    if ( !realpath(path, server_conf->server_root) )
    {
        printf("Server root incorrecto\n");
        return -1;
    }

    /* USUARIO FTP */
    user = cfg_getstr(cfg, FTP_USER);
    /* CdE usuario demsiado largo */
    if ( strlen(path) >= FTP_USER_MAX )
    {
        printf("Usuario demasiado largo incorrecto\n");
        return -1;
    }
    strcpy(server_conf->ftp_user, user);

    /* MAXIMO DE PUERTOS EN MODO PASIVO */
    server_conf->max_passive_ports = cfg_getint(cfg, MAX_PASSIVE_PORTS);
    /* CdE: tiene que permitirse al menos un puerto en modo pasivo */
    if (server_conf->max_passive_ports <= 0)
        server_conf->max_passive_ports = MAX_PASSIVE_PORTS_DEFAULT;

    cfg_free(cfg);
    return 1;
}