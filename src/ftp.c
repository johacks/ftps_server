/**
 * @file ftp.c
 * @author Joaquín Jiménez López de Castro (joaquin.jimenezl@estudiante.uam.es)
 * @brief Contains information about the FTP protocol
 * @version 1.0
 * @date 11-04-2020
 *
 * @copyright Copyright (c) 2020
 *
 */

#include "utils.h"
#include "ftp.h"

/**
 * @brief Defines a key value pair
 * Created to save the name associated to an enum and the value of the enum itself
 *
 */
typedef struct _keypair
{
    char *name; /*!< Field name*/
    int index;  /*!< Index in an array*/
} keypair;

#define C(x) #x, /*!< For each command, its name as a string and a comma*/

char *imp_commands_names[] = {IMPLEMENTED_COMMANDS}; /*!< Array of strings with the names of the implemented commands*/
keypair imp_commands_names_sorted[IMP_COMMANDS_TOP]; /*!< Array with the names of the implemented commands and their enum number*/
char *ign_commands_names[] = {IGNORED_COMMANDS};     /*!< Array of strings with the names of only known commands*/
keypair ign_commands_names_sorted[IGN_COMMANDS_TOP]; /*!< Array with the names of only known commands and their enum number*/

#undef C

static int lists_setup = 0; /*!< Indicates if the arrays have already been sorted*/
/**
 * @brief Inserts an element in array
 *
 * @param idx Current size of the array + 1
 * @param array Array where to insert
 * @param elt Element to insert
 */
void insert(int idx, keypair *array, char *elt)
{
    keypair kp;
    kp.index = idx; /*Initial index of source array is value of enum*/
    kp.name = elt;
    array[idx] = kp; /*insert at end*/
    for (; idx >= 1; idx--)
    {
        /*Put larger at the end*/
        if (strcmp(kp.name, array[idx - 1].name) > 0)
            return;
        /*Mediante swaps*/
        keypair aux = array[idx - 1];
        array[idx - 1] = kp;
        array[idx] = aux;
    }
}

/**
 * @brief Insert Sort
 *
 * @param source_array Unordered Array
 * @param array_dest Resulting ordered array
 * @param array_len Size of the array
 */
void insert_sort(char *source_array[], keypair *array_dest, int array_len)
{
    for (int i = 0; i < array_len; i++)
        insert(i, array_dest, source_array[i]);
}

/**
 * @brief Order the arrays with the commands
 *
 */
void setup_command_lists()
{
    insert_sort(imp_commands_names, imp_commands_names_sorted, IMP_COMMANDS_TOP);
    insert_sort(ign_commands_names, ign_commands_names_sorted, IGN_COMMANDS_TOP);
    lists_setup = 1;
}

/**
 * @brief Search an array by name with binary search
 *
 * @param name Name to search for
 * @param array Array container
 * @param arr_size Size of the array
 * @return int Index of the element or -1 if it is not
 */
int search_array(char *name, keypair *array, int arr_size)
{
    if (!lists_setup)
        setup_command_lists();
    /*Binary search of the command*/
    int bottom = 0, top = arr_size - 1, middle = (top + bottom) / 2, cmp;
    for (cmp = strcmp(name, array[middle].name); bottom <= top; cmp = strcmp(name, array[middle].name))
    {
        if (!cmp)
            return array[middle].index;
        else if (cmp < 0)
            top = middle - 1;
        else
            bottom = middle + 1;
        middle = (top + bottom) / 2;
    }
    return -1;
}

/**
 * @brief Returns the value of enum imp_commands associated with name
 *
 * @param name Command name
 * @return imp_commands Value of enum or -1 if not present
 */
imp_commands get_imp_command_number(char *name)
{
    return (imp_commands)search_array(name, imp_commands_names_sorted, IMP_COMMANDS_TOP);
}

/**
 * @brief Returns the value of enum ign_commands associated with name
 *
 * @param name Command name
 * @return imp_commands Value of enum or -1 if not present
 */
ign_commands get_ign_command_number(char *name)
{
    return (ign_commands)search_array(name, ign_commands_names_sorted, IGN_COMMANDS_TOP);
}

/**
 * @brief Collect name associated to the enum
 *
 * @param command FTP command
 * @return char*
 */
char *get_imp_command_name(imp_commands command)
{
    return imp_commands_names[command];
}

/**
 * @brief Collect name associated to the enum
 *
 * @param command FTP command
 * @return char*
 */
char *get_ign_command_name(ign_commands command)
{
    return ign_commands_names[command];
}

/**
 * @brief Fills the structure with the information of a request
 *
 * @param ri Stores request data
 * @param buff Buffer with raw request (expected end-of-string 0 added though)
 */
void parse_ftp_command(request_info *ri, char *buff)
{
    /*Pick up the command*/
    size_t len = strcspn(buff, " \r\n");
    strncpy(ri->command_name, buff, len);
    ri->command_name[len] = '\0';
    /*Get the argument, if any*/
    buff = &buff[len + 1];
    len = strcspn(buff, "\r\n");
    strncpy(ri->command_arg, buff, len);
    ri->command_arg[len] = '\0';
    /*Get the enum associated with the command, first search the list of implemented*/
    ri->implemented_command = get_imp_command_number(ri->command_name);
    /*Check to see if it's at least one recognized command*/
    if (ri->implemented_command == -1)
        ri->ignored_command = get_ign_command_number(ri->command_name);
    else
        ri->ignored_command = -1;
    return;
}

/**
 * @brief Sets a response to a command
 *
 * @param ri Information structure of a request
 * @param response Response to the request
 * @param ... Formatted
 * @return int response size
 */
int set_command_response(request_info *ri, char *response, ...)
{
    va_list param;
    va_start(param, response);
    ri->response_len = vsnprintf(ri->response, MAX_COMMAND_RESPONSE, response, param);
    va_end(param);
    return ri->response_len;
}