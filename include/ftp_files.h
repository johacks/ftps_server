/**
 * @file ftp_files.h
 * @author Joaquín Jiménez López de Castro (joaquin.jimenezl@estudiante.uam.es)
 * @brief Manipulacion de fichero, paths y conexion de datos
 * @version 1.0
 * @date 15-04-2020
 * 
 * @copyright Copyright (c) 2020
 * 
 */

#ifndef FTP_FILES_H
#define FTP_FILES_H

#include "utils.h"
#include "red.h"
#define DATA_SOCKET_TIMEOUT 5 /*!< Maximo de segundos de timeout en conexion de datos */

/**
 * @brief Define los posibles estados de una conexion FTP de datos
 * 
 */
typedef enum _data_conn_states
{
    DATA_CONN_CLOSED, /*!< No hay un socket abierto actualmente */
    DATA_CONN_AVAILABLE, /*!< Hay un socket listo para transmitir los datos */
    DATA_CONN_BUSY /*!< Hay un socket de datos abierto, pero ya esta transmitiendo datos */
} data_conn_states;

/**
 * @brief Define la informacion de una conexion FTP de datos
 * 
 */
typedef struct _data_conn
{
    int is_passive; /*!< Indica si la conexion es de tipo activo o pasivo */
    int socket_fd; /*!< Descriptor del socket en modo pasivo */
    int conn_fd; /*!< Descriptor de la conexion en modo activo y pasivo */
    TLS *context; /*!< Contexto TLS de conexion de datos */
    char client_ip[sizeof("XXX.XXX.XXX.XXX")]; /*!< Direccion ip del cliente para una conexion en modo activo */
    int client_port; /*!< Puerto del cliente para una conexion en modo activo */
    int abort; /*!< Indica que hay que abortar la transmision actual */
    data_conn_states conn_state; /*!< Estado de la conexion de datos */
    sem_t mutex; /*!< Mutex para el cambio del estado de la conexion */
    sem_t data_conn_sem; /*!< Si a 1, puede continuar el thread de datos */
    sem_t control_conn_sem; /*!< Si a 1, puede continuar el thread de control */
} data_conn;

/**
 * @brief Por comodidad del programador para no tener que indicar el path
 * en cada llamada, se establece el path en una variable estatica
 * @param server_root 
 */
void set_root_path(char *server_root);

/**
 * @brief Limpia un path y coloca el path absoluto real_path, utilizando el directorio actual
 * si path es relativo para poder generarlo
 * 
 * @param current_dir 
 * @param path 
 * @param real_path 
 * @return int 
 */
int get_real_path(char *current_dir, char *path, char *real_path);

/**
 * @brief Devuelve un fichero con el output de ls
 * 
 * @param path Path sin limpiar
 * @param current_dir Directorio actual de sesion FTP
 * @return FILE* Como cualquier otro FILE, sin embargo, debe cerrarse con pclose
 */
FILE *list_directories(char *path, char *current_dir);

/**
 * @brief Abre un fichero, ayudandose de current_dir para localizarlo
 * 
 * @param path path relativo o absoluto al fichero
 * @param current_dir directorio actual, posiblemente necesario
 * @param mode modo de apertura del fichero
 * @return FILE* 
 */
FILE *file_open(char *path, char *current_dir, char *mode);

/**
 * @brief Indica si un path apunta a un directorio
 * 
 * @param path Path al directorio
 * @return int 1 si verdadero
 */
int path_is_dir(char *path);

/**
 * @brief Indica si un path apunta a un fichero
 * 
 * @param path Path al fichero
 * @return int 1 si verdadero
 */
int path_is_file(char *path);

/**
 * @brief Modifica el directorio actual
 * 
 * @param current_dir Directorio actual
 * @param path Path al directorio nuevo
 * @return int 1 si correcto, -1 fallo de memoria, -2 path incorecto
 */
int ch_current_dir(char *current_dir, char *path);

/**
 * @brief Cambiar al directorio padre
 * 
 * @param current_dir Directorio actual
 * @return int 0 si ya en root, 1 si cambio efectivo, -1 fallo de memoria
 */
int ch_to_parent_dir(char *current_dir);

/**
 * @brief Envia el contenido de un buffer a traves de un socket con posibilidad de
 *        que se le aplique un filtro ascii
 * @param socket_fd Descriptor del socket
 * @param buf a enviar
 * @param buf_len cuanto enviar
 * @param ascii_mode Si no 0, transformar saltos de linea a formato universal
 * @return ssize_t si menor que 0, error
 */
ssize_t send_buffer(int socket_fd, char *buf, size_t buf_len, int ascii_mode);

/**
 * @brief Enviar el contenido de f a traves de un socket
 * 
 * @param socket_fd Descriptor del socket
 * @param f Fichero a abrir
 * @param ascii_mode Modo ascii
 * @param abort_transfer Permite cancelar transferencia
 * @return ssize_t leidos
 */
ssize_t send_file(int socket_fd, FILE *f, int ascii_mode, int *abort_transfer);

/**
 * @brief Leer contenido de un socket a un buffer
 * 
 * @param socket_fd Socket origen
 * @param dest Buffer destino
 * @param buf_len Tamaño de buffer
 * @param ascii_mode Modo ascii
 * @return ssize_t leidos
 */
ssize_t read_to_buffer(int socket_fd, char *dest, size_t buf_len, int ascii_mode);

/**
 * @brief Lee el contenido de un socket a un fichero
 * 
 * @param f Fichero destino.
 * @param socket_fd socket origen
 * @param ascii_mode Modo de trasferencia FTP
 * @param abort_transfer Permite cancelar la transferencia
 * @return ssize_t bytes leido 
 */
ssize_t read_to_file(FILE *f, int socket_fd, int ascii_mode, int *abort_transfer);

/**
 * @brief Parsea un port string de formato xxx,xxx,xxx,xxx,ppp,ppp
 * 
 * @param port_string Argumento del comando PORT
 * @param clt_ip Buffer donde se guardara la ip del cliente
 * @param clt_port Donde se guardara el puerto del cliente
 * @return int menor que 0 si error
 */
int parse_port_string(char *port_string, char *clt_ip, int *clt_port);

/**
 * @brief Crea un socket de datos para que se conecte el cliente
 * 
 * @param srv_ip Ip del servidor 
 * @param passive_port_count Semaforo con el numero de conexiones de datos creadas
 * @param socket_fd Socket resultante
 * @return int menor que 0 si error, si no, puerto
 */
int passive_data_socket_fd(char *srv_ip, sem_t *passive_port_count, int *socket_fd);

/**
 * @brief Genera port string para el comando PASV
 * 
 * @param ip Ip del servidor
 * @param port Puerto de datos
 * @param buf Buffer que almacenara el resultado
 * @return char* Resultado
 */
char *make_port_string(char *ip, int port, char *buf);

/**
 * @brief Devuelve un puntero al path sin el root
 * 
 * @param full_path Path completo
 * @return char* Puntero al path sin el root
 */
char *path_no_root(char *full_path);

#endif