/**
 * @file path.c
 * @author Joaquín Jiménez López de Castro (joaquin.jimenezl@estudiante.uam.es)
 * @brief Manipulacion de paths, ficheros y directorios
 * @version 1.0
 * @date 14-04-2020
 * 
 * @copyright Copyright (c) 2020
 * 
 */

#define _DEFAULT_SOURCE
#include "utils.h"
#include "config_parser.h"

static char root[SERVER_ROOT_MAX] = "";
static size_t root_size; 

/**
 * @brief Por comodidad del programador para no tener que indicar el path
 * en cada llamada, se establece el path en una variable estatica
 * @param server_root 
 */
void set_root_path(char *server_root)
{
    strncpy(root, server_root, SERVER_ROOT_MAX - 1);
    root_size = strlen(root);
}

/**
 * @brief Limpia un path y coloca el path absoluto real_path, utilizando el directorio actual
 * si path es relativo para poder generarlo
 * 
 * @param current_dir 
 * @param path 
 * @param real_path 
 * @return int 
 */
int get_real_path(char *current_dir, char *path, char *real_path)
{
    char buff = alloca((strlen(current_dir) + strlen(path) + 1)*sizeof(char)); /* 'Alloca' libera memoria de funcion automaticamente */
    if ( !buff ) return -1;

    /* Si el path dado es relativo, usaremos el directorio actual */
    if ( path[0] == '\0' || path[0] != '/' )
    {
        strcpy(buff, current_dir);
        strcat(buff, "/");
        strcat(buff, path);
    }
    /* Si el path es absoluto, concatenamos al root */
    else
    {
        strcpy(buff, root);
        strcat(buff, path);
    }
    
    /* El siguiente paso es resolver el path */
    if ( !realpath(buff, real_path) ) return -2;
    /* Finalmente, comprobar que el path es root o un subdirectorio de este */
    if ( memcmp(real_path, root, root_size) ) return -2;
    /* Tras la cadena de root, o acaba el path, o hay subdirectorios, que no contienen '..' gracias a realpath */
    if ( !(real_path[root_size] == '\0' || real_path[root_size] =='/') ) return -2;

    /* Path correcto y listo en real_path */
    return real_path;
}

/**
 * @brief Devuelve un fichero con el output de ls
 * 
 * @param path 
 * @param current_dir 
 * @return FILE* 
 */
FILE *list_directories(char *path, char *current_dir)
{
    char buff = alloca((strlen(current_dir) + strlen(path) + 1) * sizeof(char));
    if ( !buff ) return NULL;
    int ret;

    /* Recoger path */
    if ( (ret = get_real_path(current_dir, path, buff)) < 0 ) return ret;

    /* Llamada a ls, redirigiendo el output */
    FILE *output = popen("ls -lbq --numeric-uid-gid --hyperlink=never --color=never", "r");

    return output;
}