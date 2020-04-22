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

#define MAX_FTP_COMMAND_NAME 4 /*!< Tamaño maximo de comando FTP */
#define MAX_COMMAND_ARG XL_SZ /*!< Tamaño maximo de su argumento */
#define MAX_COMMAND_RESPONSE XL_SZ /*!< Tamaño maximo de resupuesta */
#define FTP_CONTROL_PORT 21 /*!< Puerto FTP de control */
#define FTP_DATA_PORT 20 /*!< Puerto FTP de datos (modo activo) */

/* Enum de strings en C: https://stackoverflow.com/questions/9907160/how-to-convert-enum-names-to-string-in-c */
#define IMPLEMENTED_COMMANDS /*!< Comandos FTP implementados */            \
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
C(USER) /*!< Indicar nombre de usuario */                                  \
C(SYST) /*!< Indicar sistema operativo */                                  \
C(STRU) /*!< Indicar la estructura del servidor */                         \
C(MODE) /*!< Modo de transmision de ficheros */                            \
C(NOOP) /*!< Operacion vacia */                                            \
C(AUTH) /*!< Indica que se usara conexion segura */                        \
C(PBSZ) /*!< Indica tamaño de buffer */                                    \
C(PROT) /*!< Indica nivel de seguridad */                                  \
C(FEAT) /*!< Caracteristicas adicionales del servidor */

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

#define IGNORED_COMMANDS /*!< Comandos FTP reconocidos pero ignorados */\
C(ACCT)C(ADAT)C(ALLO)C(APPE)C(AVBL)C(CCC)C(CONF)C(CSID)C(DSIZ)C(ENC)C(EPRT)C(EPSV)C(HOST)C(LANG)\
C(LPRT)C(LPSV)C(MDTM)C(MFCT)C(MFF)C(MFMT)C(MIC)C(MLSD)C(MLST)C(NLST)C(OPTS)C(REIN)C(REST)\
C(SITE)C(SMNT)C(SPSV)C(STAT)C(STOU)C(THMB)C(XCUP)C(XMKD)C(XPWD)C(XRCP)C(XRMD)C(XRSQ)C(XSEM)C(XSEN)

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

/**
 * @brief Informacion asociada a una peticion y su respuesta en formato
 * 
 */
typedef struct _request_info
{
    imp_commands implemented_command;
    ign_commands ignored_command;
    char command_name[MAX_FTP_COMMAND_NAME + 4];
    char command_arg[MAX_COMMAND_ARG + 1];
    char response[MAX_COMMAND_RESPONSE];
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

#define CODE_150_RETR "150 Enviando archivo %s\r\n" /*!< Enviando archivo */
#define CODE_150_STOR "150 Almacenando archivo %s\r\n" /*!< Comienza a almacenar archivo */
#define CODE_150_LIST "150 Enviando listado de directorio\r\n" /*!< Despliega listado del directorio actual */

#define CODE_200_OP_OK "200 Operacion correcta\r\n" /*!< Mensaje de exito */
#define CODE_211_FEAT "211-Features adicionales:\n PASV\n SIZE\n AUTH TLS\n PROT\n PBSZ\n211 End\r\n" /*!< Funcionalidades de FEAT */
#define CODE_213_FILE_SIZE "213 Tamaño de archivo: %zd Bytes\r\n" /*!< Tamaño de archivo */
#define CODE_214_HELP "214 Lista de comandos implementados: " /*!< lista de comandos implementados*/
#define CODE_215_SYST "215 %s OS\r\n" /*!< Sistema operativo */
#define CODE_220_WELCOME_MSG "220 Bienvenido a mi servidor FTP\r\n" /*!< Mensaje de bienvenida al servidor */
#define CODE_221_GOODBYE_MSG "221 Hasta la vista\r\n" /*!< Despedir al cliente */
#define CODE_226_DATA_TRANSFER "226 Transferencia de datos terminada: %zd Bytes\r\n" /*!< Termina una transferencia de datos */
#define CODE_227_PASV_RES "227 Entering Passive Mode (%s)\r\n" /*!< Indica al cliente el puerto de datos */
#define CODE_230_AUTH_OK "230 Autenticacion correcta\r\n" /*!< Usuario y contraseña correctos */
#define CODE_234_START_NEG "234 Empezar negociacion TLS\r\n" /*!< Dar comienzo a negociacion TLS */
#define CODE_25O_FILE_OP_OK "250 Operacion sobre archivo correcta\r\n" /*!< Operacion realizada sobre archivo correcta */
#define CODE_250_DELE_OK "250 %s borrado correctamente\r\n"/*!< Archivo borrado correctamente */
#define CODE_250_CHDIR_OK "250 Cambiado al directorio %s\r\n" /*!< Cambio de directorio */
#define CODE_257_PWD_OK "257 %s\r\n" /*!< Muestra el directorio actual */
#define CODE_257_MKD_OK "257 %s creado\r\n" /*!< Indica directorio creado correctamente */

#define CODE_331_PASS "331 Introduzca el password\r\n" /*!< Se necesita una contraseña */
#define CODE_350_RNTO_NEEDED "350 Necesario nuevo nombre\r\n" /*!< Pedir nombre al que se renombra el fichero */

#define CODE_421_BAD_TLS_NEG "421 Error en la negociacion TLS\r\n" /*!< Fallo en negociacion TLS */
#define CODE_421_DATA_OPEN "421 Ya hay una conexion de datos activa\r\n" /*!< Tipicamente ante PORT o PASV */
#define CODE_421_BUSY_DATA "421 Hay una transmision de datos en curso, llame a a ABORT o espere a que acabe\r\n" /*!< Transmision en curso */
#define CODE_425_CANNOT_OPEN_DATA "425 No se ha podido abrir conexion de datos: %s\r\n" /* Fallo al abrir conexion de datos */
#define CODE_430_INVALID_AUTH "430 Usuario o password incorrectos\r\n" /*!< Error de autenticacion */
#define CODE_431_INVALID_SEC "431 %s no aceptado, use TLS\r\n" /*!< Error de mecanismo propuesto */
#define CODE_451_DATA_CONN_LOST "451 Error en la transmision de datos\r\n" /*!< Error de transmision de datos */
#define CODE_452_NO_SPACE "452 Espacio insuficiente\r\n" /*!< Sin espacio para transmitir fichero */

#define CODE_500_UNKNOWN_CMD "500 Comando no reconocido\r\n" /*!< Comando no reconocido */
#define CODE_501_BAD_ARGS "501 Error de sintaxis en los argumentos\r\n" /*!< Argumento no reconocido */
#define CODE_502_NOT_IMP_CMD "502 Comando no implementado\r\n" /*!< Comando no implementado */
#define CODE_503_BAD_SEQUENCE "503 Secuencia incorrecta de comandos\r\n" /*!< Recibidos comandos en desorden */
#define CODE_504_UNSUPORTED_PARAM "504 Argumento no implementado\r\n" /*!< Argumentp no soportado */
#define CODE_522_NO_TLS "522 Nivel de seguridad insuficiente\r\n" /*!< No se ha autenticado */
#define CODE_530_NO_LOGIN "530 Usuario no logueado\r\n" /*!< Exige usuario logueado */
#define CODE_534_NO_CERT "534 No se ha dado un certificado\r\n" /*!< Exige certificado al cliente */
#define CODE_536_INSUFFICIENT_SEC "536 Nivel de seguridad no aceptado, solo vale 'P' (private)\r\n" /*!< Exige nivel de seguridad mayor */
#define CODE_550_NO_ACCESS "550 No se puede acceder al archivo\r\n" /*!< Sin acceso a archivo */
#define CODE_550_NO_DELE "550 No se ha podido borrar el archivo: %s\r\n" /*!< Fallo al borrar fichero */
#endif