/**
 * @file ftp.c
 * @author Joaquín Jiménez López de Castro (joaquin.jimenezl@estudiante.uam.es)
 * @brief Contiene informacion sobre el protocolo FTP
 * @version 1.0
 * @date 11-04-2020
 * 
 * @copyright Copyright (c) 2020
 * 
 */

#include "utils.h"
#include "ftp.h"

/**
 * @brief Define un par clave valor
 * Creado para guardar el nombre asociado a un enum y el propio valor del enum
 * 
 */
typedef struct _keypair
{
    char *name;
    int index;
} keypair;

#define C(x) #x,

char *imp_commands_names[] = { IMPLEMENTED_COMMANDS };
keypair imp_commands_names_sorted[IMP_COMMANDS_TOP];
char *ign_commands_names[] = { IGNORED_COMMANDS };
keypair ign_commands_names_sorted[IGN_COMMANDS_TOP];

#undef C

int lists_setup = 0;

/**
 * @brief Inserta un elemento en array
 * 
 * @param idx Tamaño actual del array + 1
 * @param array Array donde se inserta
 * @param elt Elemento a insertar
 */
void insert(int idx, keypair *array, char *elt)
{
    keypair kp;
    kp.index = idx; /* Indice inicial de array fuente es valor de enum */
    kp.name = elt;
    array[idx] = kp; /* Insertar al final */
    for (; idx >= 1; idx--)
    {
        /* Colocar mas grande al final */
        if ( strcmp(kp.name, array[idx - 1].name) > 0 ) return;
        /* Mediante swaps */
        keypair aux = array[idx - 1];
        array[idx - 1] = kp;
        array[idx] = aux;
    }
}

/**
 * @brief Insert Sort
 * 
 * @param source_array Array desordenado
 * @param array_dest Array ordenado resultante
 * @param array_len Tamaño del array
 */
void insert_sort(char *source_array[], keypair *array_dest, int array_len)
{
    for ( int i = 0; i < array_len; i++)
        insert(i, array_dest, source_array[i]);
}

/**
 * @brief Ordena los arrays con los comandos
 * 
 */
void setup_command_lists()
{
    insert_sort(imp_commands_names, imp_commands_names_sorted, IMP_COMMANDS_TOP);
    insert_sort(ign_commands_names, ign_commands_names_sorted, IGN_COMMANDS_TOP);
    lists_setup = 1;
}

/**
 * @brief Buscar en un array por nombre con busqueda binaria
 * 
 * @param name Nombre a buscar
 * @param array Array contenedor
 * @param arr_size Tamaño del array
 * @return int Indice del elemento o -1 si no esta
 */
int search_array(char *name, keypair *array, int arr_size)
{
    if ( !lists_setup )
        setup_command_lists();
    /* Busqueda binaria del comando */
    int bottom = 0, top = arr_size - 1, middle = (top + bottom) / 2, cmp;
    for ( cmp = strcmp(name, array[middle].name); bottom <= top; cmp = strcmp(name, array[middle].name))
    {
        if ( !cmp )
            return array[middle].index;
        else if ( cmp < 0 )
            top = middle - 1;
        else
            bottom = middle + 1;
        middle = ( top + bottom ) / 2;
    }
    return -1;
}

/**
 * @brief Devuelve el valor de enum imp_commands asociado a name
 * 
 * @param name Nombre del comando
 * @return imp_commands Valor de enum o -1 si no esta
 */
imp_commands get_imp_command_number(char *name)
{
    return (imp_commands) search_array(name, imp_commands_names_sorted, IMP_COMMANDS_TOP);
}

/**
 * @brief Devuelve el valor de enum ign_commands asociado a name
 * 
 * @param name Nombre del comando
 * @return imp_commands Valor de enum o -1 si no esta
 */
ign_commands get_ign_command_number(char *name)
{
    return (ign_commands) search_array(name, ign_commands_names_sorted, IGN_COMMANDS_TOP);
}

/**
 * @brief Recoger nombre asociado al enum
 * 
 * @param command Comando FTP
 * @return char* 
 */
char *get_imp_command_name(imp_commands command)
{
    return imp_commands_names[command];
}

/**
 * @brief Recoger nombre asociado al enum
 * 
 * @param command Comando FTP
 * @return char* 
 */
char *get_ign_command_name(ign_commands command)
{
    return ign_commands_names[command];
}

/**
 * @brief Rellena la estructura con la informacion de una peticion
 * 
 * @param ri Almacena datos de la peticion
 * @param buff Buffer con la peticion en crudo (se espera que se haya añadido un 0 de fin de cadena sin embargo)
 */
void parse_ftp_command(request_info *ri, char *buff)
{
    /* Reoger el comando */
    size_t len = strcspn(buff, " \r\n");
    strncpy(ri->command_name, buff, len);
    ri->command_name[len] = '\0';
    /* Recoger el argumento, si lo hay */
    buff = &buff[len + 1];
    len = strcspn(buff, "\r\n");
    strncpy(ri->command_arg, buff, len);
    ri->command_arg[len] = '\0';
    /* Recoger el enum asociado al comando, primero buscar en la lista de implementados */
    ri->implemented_command = get_imp_command_number(ri->command_name);
    /* Mirar a ver si al menos es un comando reconocido */
    if ( ri->implemented_command == -1 )
        ri->ignored_command = get_ign_command_number(ri->command_name);
    else
        ri->ignored_command = -1;
    return;
}

/**
 * @brief Establece una respuesta a un comando
 * 
 * @param ri Estructura de informacion de una peticion 
 * @param response Respuesta a la peticion
 * @param ... Formateado
 * @return int tamaño de respuesta
 */
int set_command_response(request_info *ri, char *response, ...)
{
    va_list param;
    va_start(param, response);
    ri->response_len = vsnprintf(ri->response, MAX_COMMAND_RESPOSE, response, param);
    va_end(param);
    return ri->response_len;
}