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
#define MAX_COMMAND_RESPOSE XXL_SZ
#define FTP_CONTROL_PORT 21
#define FTP_DATA_PORT 20

/* Enum de strings en C: https://stackoverflow.com/questions/9907160/how-to-convert-enum-names-to-string-in-c */
#define IMPLEMENTED_COMMANDS                                               \
C(ABOR) /*!< Abortar transmision de datos */                               \
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

#define DATA_CALLBACK(cmd) (cmd == LIST || cmd == STOR || cmd == RETR) /*!< Indica si un comando hace uso de conexion de datos */

#define IGNORED_COMMANDS \
C(ACCT)C(ADAT)C(ALLO)C(APPE)C(AUTH)C(AVBL)C(CCC)C(CONF)C(CSID)C(DSIZ)C(ENC)C(EPRT)C(EPSV)C(FEAT)C(HOST)C(LANG)\
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
    char response[MAX_COMMAND_RESPOSE];
    int response_len;
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

/**
 * @brief Establece una respuesta a un comando
 * 
 * @param ri Estructura de informacion de una peticion 
 * @param response Respuesta a la peticion
 * @param ... Formateado
 * @return int tamaño de respuesta
 */
int set_command_response(request_info *ri, char *response, ...);

/* RESPUESTAS DEL SERVIDOR POR EL PUERTO DE CONTROL */

#define CODE_150_RETR "150 Enviando archivo %s\n" /*!< Enviando archivo */
#define CODE_150_STOR "150 Almacenando archivo %s\n" /*!< Comienza a almacenar archivo */
#define CODE_150_LIST "150 Enviando listado del directorio actual\n" /*!< Despliega listado del directorio actual */

#define CODE_200_OP_OK "200 Operacion correcta\n" /*!< Mensaje de exito */
#define CODE_213_FILE_SIZE "213 Tamaño de archivo: %ldB\n" /*!< Tamaño de archivo */
#define CODE_214_HELP "214 Lista de comandos implementados: " /*!< lista de comandos implementados*/
#define CODE_220_WELCOME_MSG "220 Bienvenido a mi servidor FTP\n" /*!< Mensaje de bienvenida al servidor */
#define CODE_221_GOODBYE_MSG "221 Hasta la vista\n" /*!< Despedir al cliente */
#define CODE_226_DATA_TRANSFER "226 Transferencia de datos terminada: %ldB\n" /*!< Termina una transferencia de datos */
#define CODE_227_PASV_RES "227 Puerto de datos en (%s)\n" /*!< Indica al cliente el puerto de datos */
#define CODE_230_AUTH_OK "230 Autenticacion correcta\n" /*!< Usuario y contraseña correctos */
#define CODE_25O_FILE_OP_OK "250 Operacion sobre archivo correcta\n" /*!< Operacion realizada sobre archivo correcta */
#define CODE_250_DELE_OK "250 %s borrado correctamente\n"/*!< Archivo borrado correctamente */
#define CODE_250_CHDIR_OK "250 Cambiado al directorio %s\n" /*!< Cambio de directorio */
#define CODE_257_PWD_OK "257 %s\n" /*!< Muestra el directorio actual */
#define CODE_257_MKD_OK "257 %s creado\n" /*!< Indica directorio creado correctamente */

#define CODE_331_PASS "331 Introduzca la contraseña\n" /*!< Se necesita una contraseña */
#define CODE_350_RNTO_NEEDED "350 Necesario nuevo nombre\n" /*!< Pedir nombre al que se renombra el fichero */

#define CODE_421_DATA_OPEN "421 Ya hay una conexion de datos activa\n" /*!< Tipicamente ante PORT o PASV */
#define CODE_421_BUSY_DATA "421 Hay una transmision de datos en curso, llame a a ABORT o espere a que acabe\n" /*!< Transmision en curso */
#define CODE_425_CANNOT_OPEN_DATA "425 No se ha podido abrir conexion de datos: %s\n" /* Fallo al abrir conexion de datos */
#define CODE_430_INVALID_AUTH "430 Usuario o contraseña incorrectos\n" /*!< Error de autenticacion */

#define CODE_500_UNKNOWN_CMD "500 Comando no reconocido\n" /*!< Comando no reconocido */
#define CODE_501_BAD_ARGS "501 Error de sintaxis en los argumentos\n" /*!< Argumento no reconocido */
#define CODE_502_NOT_IMP_CMD "502 Comando no implementado\n" /*!< Comando no implementado */
#define CODE_503_BAD_SEQUENCE "503 Secuencia incorrecta de comandos\n" /*!< Recibidos comandos en desorden */
#define CODE_530_NO_LOGIN "530 Usuario no logueado\n" /*!< Exige usuario logueado */
#define CODE_550_NO_ACCESS "550 No se puede acceder al archivo\n" /*!< Sin acceso a archivo */
#define CODE_550_NO_DELE "550 No se ha podido borrar el archivo: %s\n" /*!< Fallo al borrar fichero */
#endif