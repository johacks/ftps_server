/**
 * @file ftp.h
 * @author Joaquín Jiménez López de Castro (joaquin.jimenezl@estudiante.uam.es)
 * @brief Contains information about the ftp protocol
 * @version 1.0
 * @date 11-04-2020
 *
 * @copyright Copyright (c) 2020
 *
 */

#ifndef FTP_H
#define FTP_H
#include "utils.h"

#define MAX_FTP_COMMAND_NAME 4     /*!< Maximum size of FTP command*/
#define MAX_COMMAND_ARG XL_SZ      /*!< Maximum size of its argument*/
#define MAX_COMMAND_RESPONSE XL_SZ /*!< Maximum response size*/
#define FTP_CONTROL_PORT 21        /*!< control FTP port*/
#define FTP_DATA_PORT 20           /*!< FTP data port (active mode)*/
/*Enum of strings in C: https://stackoverflow.com/questions/9907160/how-to-convert-enum-names-to-string-in-c*/
#define IMPLEMENTED_COMMANDS /*!< FTP commands implemented*/                         \
    C(ABOR)                  /*!< Abort data transmission*/                          \
    C(CDUP)                  /*!< Move to parent directory*/                         \
    C(CWD)                   /*!< Change directory*/                                 \
    C(HELP)                  /*!< show help*/                                        \
    C(MKD)                   /*!< Create a directory*/                               \
    C(PASS)                  /*!< Receive password from user*/                       \
    C(RNTO)                  /*!< Indicate what name the file will have*/            \
    C(LIST)                  /*!< Show files in current directory*/                  \
    C(PASV)                  /*!< Start data connection in passive mode*/            \
    C(DELE)                  /*!< Delete file*/                                      \
    C(PORT)                  /*!< Tell the server client data port*/                 \
    C(PWD)                   /*!< Indicate current directory*/                       \
    C(QUIT)                  /*!< Close connection*/                                 \
    C(RETR)                  /*!< Request file copy*/                                \
    C(RMD)                   /*!< Delete a directory*/                               \
    C(RMDA)                  /*!< Delete a directory tree*/                          \
    C(STOR)                  /*!< Send a file to the server*/                        \
    C(RNFR)                  /*!< Specify the name of the file to rename*/           \
    C(SIZE)                  /*!< Return size of a file*/                            \
    C(TYPE)                  /*!< Indicate data transmission type: ascii or binary*/ \
    C(USER)                  /*!< Specify username*/                                 \
    C(SYST)                  /*!< Indicate operating system*/                        \
    C(STRU)                  /*!< Indicate the structure of the server*/             \
    C(MODE)                  /*!< File transmission mode*/                           \
    C(NOOP)                  /*!< Empty operation*/                                  \
    C(AUTH)                  /*!< Indicates that secure connection will be used*/    \
    C(PBSZ)                  /*!< Indicates buffer size*/                            \
    C(PROT)                  /*!< Indicates security level*/                         \
    C(FEAT)                  /*!< Additional server features*/

#define C(x) x, /*!< For each command, command name followed by a comma*/
/**
 * @brief List of commands that the server implements
 *
 */
typedef enum _imp_commands
{
    IMPLEMENTED_COMMANDS IMP_COMMANDS_TOP
} imp_commands;
#undef C

#define DATA_CALLBACK(cmd) (cmd == LIST || cmd == STOR || cmd == RETR) /*!< Indicates if a command makes use of a data connection*/

#define IGNORED_COMMANDS                                                                                      \
    C(ACCT)                                                                                                   \
    C(ADAT) C(ALLO) C(APPE) C(AVBL) C(CCC) C(CONF) C(CSID) C(DSIZ) C(ENC) C(EPRT) C(EPSV) C(HOST) C(LANG)     \
        C(LPRT) C(LPSV) C(MDTM) C(MFCT) C(MFF) C(MFMT) C(MIC) C(MLSD) C(MLST) C(NLST) C(OPTS) C(REIN) C(REST) \
            C(SITE) C(SMNT) C(SPSV) C(STAT) C(STOU) C(THMB) C(XCUP) C(XMKD) C(XPWD) C(XRCP) C(XRMD) C(XRSQ) C(XSEM) C(XSEN) /*!< FTP commands recognized but ignored*/
#define C(x) x,                                                                                                             /*!< For each command, command name followed by a comma*/
/**
 * @brief List of known but not implemented FTP commands
 *
 */
typedef enum _ign_commands
{
    IGNORED_COMMANDS IGN_COMMANDS_TOP
} ign_commands;

#undef C

/**
 * @brief Information associated with a request and its response
 *
 */
typedef struct _request_info
{
    imp_commands implemented_command;            /*!< Indicates the number of the command if it is implemented*/
    ign_commands ignored_command;                /*!< Indicates the number of the command if it is only known*/
    char command_name[MAX_FTP_COMMAND_NAME + 4]; /*!< Command name*/
    char command_arg[MAX_COMMAND_ARG + 1];       /*!< Argument that came with the command*/
    char response[MAX_COMMAND_RESPONSE];         /*!< Response to be sent to the command*/
    int response_len;                            /*!< Size of said response*/
} request_info;

/**
 * @brief Returns the value of enum imp_commands associated with name
 *
 * @param name Command name
 * @return imp_commands Value of enum or -1 if not present
 */
imp_commands get_imp_command_number(char *name);

/**
 * @brief Returns the value of enum ign_commands associated with name
 *
 * @param name Command name
 * @return imp_commands Value of enum or -1 if not present
 */
ign_commands get_ign_command_number(char *name);

/**
 * @brief Collect name associated to the enum
 *
 * @param command FTP command
 * @return char*
 */
char *get_imp_command_name(imp_commands command);

/**
 * @brief Collect name associated to the enum
 *
 * @param command FTP command
 * @return char*
 */
char *get_ign_command_name(ign_commands command);

/**
 * @brief Fills the structure with the information of a request
 *
 * @param ri Stores request data
 * @param buff Buffer with raw request (expected end-of-string 0 added though)
 */
void parse_ftp_command(request_info *ri, char *buff);

/**
 * @brief Sets a response to a command
 *
 * @param ri Information structure of a request
 * @param response Response to the request
 * @param ... Formatted
 * @return int response size
 */
int set_command_response(request_info *ri, char *response, ...);

/*SERVER RESPONSES THROUGH THE CONTROL PORT*/

#define CODE_150_RETR "150 Enviando archivo %s\r\n"            /*!< Sending file*/
#define CODE_150_STOR "150 Almacenando archivo %s\r\n"         /*!< Start storing file*/
#define CODE_150_LIST "150 Enviando listado de directorio\r\n" /*!< Display current directory listing*/

#define CODE_200_OP_OK "200 Operacion correcta\r\n"                                                               /*!< Success message*/
#define CODE_211_FEAT "211-Features adicionales:\r\n PASV\r\n SIZE\r\n AUTH TLS\r\n PROT\r\n PBSZ\r\n211 End\r\n" /*!< FEAT Features*/
#define CODE_213_FILE_SIZE "213 Tamaño de archivo: %zd Bytes\r\n"                                                 /*!< File size*/
#define CODE_214_HELP "214 Lista de comandos implementados: "                                                     /*!< list of implemented commands*/
#define CODE_215_SYST "215 %s OS\r\n"                                                                             /*! <Operating system*/
#define CODE_220_WELCOME_MSG "220 Bienvenido a mi servidor FTP\r\n"                                               /*!< Server welcome message*/
#define CODE_221_GOODBYE_MSG "221 Hasta la vista\r\n"                                                             /*!< Fire the client*/
#define CODE_226_DATA_TRANSFER "226 Transferencia de datos terminada: %zd Bytes\r\n"                              /*!< Terminates a data transfer*/
#define CODE_227_PASV_RES "227 Entering Passive Mode (%s)\r\n"                                                    /*!< Tells the client the data port*/
#define CODE_230_AUTH_OK "230 Autenticacion correcta\r\n"                                                         /*!< Correct username and password*/
#define CODE_234_START_NEG "234 Empezar negociacion TLS\r\n"                                                      /*!< Start TLS negotiation*/
#define CODE_25O_FILE_OP_OK "250 Operacion sobre archivo correcta\r\n"                                            /*!< Operation performed on correct file*/
#define CODE_250_DELE_OK "250 %s borrado correctamente\r\n"                                                       /*!< File deleted successfully*/
#define CODE_250_CHDIR_OK "250 Cambiado al directorio %s\r\n"                                                     /*!< Change directory*/
#define CODE_257_PWD_OK "257 %s\r\n"                                                                              /*!< show the current directory*/
#define CODE_257_MKD_OK "257 %s creado\r\n"                                                                       /*!< Indicates directory created successfully*/

#define CODE_331_PASS "331 Introduzca el password\r\n"        /*!< Password is required*/
#define CODE_350_RNTO_NEEDED "350 Necesario nuevo nombre\r\n" /*!< Request name to which the file is renamed*/

#define CODE_421_BAD_TLS_NEG "421 Error en la negociacion TLS\r\n"                                               /*!< Failure in TLS negotiation*/
#define CODE_421_DATA_OPEN "421 Ya hay una conexion de datos activa\r\n"                                         /*! <Typically PORT or PASV ante*/
#define CODE_421_BUSY_DATA "421 Hay una transmision de datos en curso, llame a a ABORT o espere a que acabe\r\n" /*!< Transmission in progress*/
#define CODE_425_CANNOT_OPEN_DATA "425 No se ha podido abrir conexion de datos: %s\r\n"                          /*!< Failed to open data connection*/
#define CODE_430_INVALID_AUTH "430 Usuario o password incorrectos\r\n"                                           /*!< Authentication error*/
#define CODE_431_INVALID_SEC "431 %s no aceptado, use TLS\r\n"                                                   /*!< Proposed mechanism error*/
#define CODE_451_DATA_CONN_LOST "451 Error en la transmision de datos\r\n"                                       /*!< Data transmission error*/
#define CODE_452_NO_SPACE "452 Espacio insuficiente\r\n"                                                         /*!< No space to transmit file*/

#define CODE_500_UNKNOWN_CMD "500 Comando no reconocido\r\n"                                        /*!< Unrecognized command*/
#define CODE_501_BAD_ARGS "501 Error de sintaxis en los argumentos\r\n"                             /*!< Unrecognized argument*/
#define CODE_502_NOT_IMP_CMD "502 Comando no implementado\r\n"                                      /*!< Command not implemented*/
#define CODE_503_BAD_SEQUENCE "503 Secuencia incorrecta de comandos\r\n"                            /*!< Commands received out of order*/
#define CODE_504_UNSUPORTED_PARAM "504 Argumento no implementado\r\n"                               /*!< Argument not supported*/
#define CODE_522_NO_TLS "522 Nivel de seguridad insuficiente\r\n"                                   /*!< Not authenticated*/
#define CODE_530_NO_LOGIN "530 Usuario no logueado\r\n"                                             /*!< Require logged in user*/
#define CODE_534_NO_CERT "534 No se ha dado un certificado\r\n"                                     /*!< Require client certificate*/
#define CODE_536_INSUFFICIENT_SEC "536 Nivel de seguridad no aceptado, solo vale 'P' (private)\r\n" /*!< Requires higher security level*/
#define CODE_550_NO_ACCESS "550 No se puede acceder al archivo\r\n"                                 /*!< No file access*/
#define CODE_550_NO_DELE "550 No se ha podido borrar el archivo: %s\r\n"                            /*!< Failed to delete file*/
#endif