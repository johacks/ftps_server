/**
 * @file ftp.h
 * @author Joaquín Jiménez López de Castro (joaquin.jimenezl@estudiante.uam.es)
 * @brief Contiene informacion sobre el protoclo ftp
 * @version 1.0
 * @date 11-04-2020
 * 
 * @copyright Copyright (c) 2020
 * 
 */

#ifndef FTP_H
#define FTP_H
#include "utils.h"

#define MAX_FTP_COMMAND_NAME 4
#define MAX_COMMAND_ARG MEDIUM_SZ

/* Enum de strings en C: https://stackoverflow.com/questions/9907160/how-to-convert-enum-names-to-string-in-c */
#define IMPLEMENTED_COMMANDS                                               \
C(CDUP) /*!< Moverse a directorio superior */                              \
C(CWD)  /*!< Cambiar de directorio */                                      \
C(HELP) /*!< Mostrar ayuda */                                              \
C(MKD)  /*!< Crear un directorio */                                        \
C(PASS) /*!< Recibir contraseña del usuario */                             \
C(RNTO) /*!< Indicar que nombre tendra dicho fichero */                    \
C(LIST) /*!< Mostrar archivos del directorio actual */                     \
C(PASV) /*!< Iniciar conexion de datos en modo pasivo */                   \
C(DELE) /*!< Borrar fichero */                                             \
C(PORT) /*!< Indicar al servidor puerto de datos de cliente */             \
C(PWD)  /*!< Indicar directorio actual */                                  \
C(QUIT) /*!< Cerrar conexion */                                            \
C(RETR) /*!< Pedir copia de fichero */                                     \
C(RMD)  /*!< Borrar un directorio */                                       \
C(RMDA) /*!< Borrar el arbol de un directorio */                           \
C(STOR) /*!< Enviar un fichero al servidor */                              \
C(RNFR) /*!< Indicar el nombre del archivo a renombrar */                  \
C(SIZE) /*!< Devolver tamaño de un fichero */                              \
C(TYPE) /*!< Indicar tipo de transmision de datos: ascii o binario */      \
C(USER) /*!< Indicar nombre de usuario */

#define C(x) x,
/**
 * @brief Lista de comandos que implementa el servidor
 * 
 */
typedef enum _imp_commands
{
    IMPLEMENTED_COMMANDS IMP_COMMANDS_TOP
} imp_commands;
#undef C

#define IGNORED_COMMANDS \
C(ABOR)C(ACCT)C(ADAT)C(ALLO)C(APPE)C(AUTH)C(AVBL)C(CCC)C(CONF)C(CSID)C(DSIZ)C(ENC)C(EPRT)C(EPSV)C(FEAT)C(HOST)C(LANG)\
C(LPRT)C(LPSV)C(MDTM)C(MFCT)C(MFF)C(MFMT)C(MIC)C(MLSD)C(MLST)C(MODE)C(NLST)C(NOOP)C(OPTS)C(PBSZ)C(PROT)C(REIN)C(REST)\
C(SITE)C(SMNT)C(SPSV)C(STAT)C(STOU)C(STRU)C(SYST)C(THMB)C(XCUP)C(XMKD)C(XPWD)C(XRCP)C(XRMD)C(XRSQ)C(XSEM)C(XSEN)

#define C(x) x, 

/**
 * @brief Lista de comandos FTP conocidos pero no implementados
 * 
 */
typedef enum _ign_commands
{
    IGNORED_COMMANDS IGN_COMMANDS_TOP
} ign_commands;

#undef C

typedef struct _request_info
{
    imp_commands implemented_command;
    ign_commands ignored_command;
    char command_name[MAX_FTP_COMMAND_NAME + 4];
    char command_arg[MAX_COMMAND_ARG + 1];
} request_info;

/**
 * @brief Devuelve el valor de enum imp_commands asociado a name
 * 
 * @param name Nombre del comando
 * @return imp_commands Valor de enum o -1 si no esta
 */
imp_commands get_imp_command_number(char *name);

/**
 * @brief Devuelve el valor de enum ign_commands asociado a name
 * 
 * @param name Nombre del comando
 * @return imp_commands Valor de enum o -1 si no esta
 */
ign_commands get_ign_command_number(char *name);

/**
 * @brief Recoger nombre asociado al enum
 * 
 * @param command Comando FTP
 * @return char* 
 */
char *get_imp_command_name(imp_commands command);

/**
 * @brief Recoger nombre asociado al enum
 * 
 * @param command Comando FTP
 * @return char* 
 */
char *get_ign_command_name(ign_commands command);

/**
 * @brief Rellena la estructura con la informacion de una peticion
 * 
 * @param ri Almacena datos de la peticion
 * @param buff Buffer con la peticion en crudo (se espera que se haya añadido un 0 de fin de cadena sin embargo)
 */
void parse_ftp_command(request_info *ri, char *buff);

/* RESPUESTAS DEL SERVIDOR POR EL PUERTO DE CONTROL */
#define CODE_220_WELCOME_MSG "220 Bienvenido a mi servidor FTP\n" /*!< Mensaje de bienvenida al servidor */
#define CODE_230_AUTH_OK "230 Autenticacion correcta\n" /*!< Usuario y contraseña correctos */
#define CODE_331_PASS "331 Introduzca la contraseña\n" /*!< Se necesita una contraseña */
#define CODE_430_INVALID_AUTH "430 Usuario o contraseña incorrectos\n" /*!< Error de autenticacion */
#define CODE_501_UNKNOWN_CMD_MSG "501 Error de sintaxis en los argumentos\n" /*!< Argumento no reconocido */
#define CODE_502_NOT_IMP_CMD_MSG "502 Comando no implementado\n" /*!< Comando no implementado */
#define CODE_503_BAD_SEQUENCE "503 Secuencia incorrecta de comandos\n" /*!< Recibidos comandos en desorden */
#endif