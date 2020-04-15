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
 * @brief Conectarse en modo activo a un puerto de datos del cliente
 * 
 * @param port_string Argumento del comando PORT
 * @param srv_ip Ip de despliegue del servidor
 * @return int menor que 0 si error
 */
int active_data_socket_fd(char *port_string, char *srv_ip);

/**
 * @brief Crea un socket de datos para que se conecte el cliente
 * 
 * @param srv_ip Ip del servidor 
 * @param passive_port_count Semaforo con el numero de conexiones de datos creadas
 * @param qlen Tamaño maximo de la cola de conexiones pendientes
 * @return int 
 */
int passive_data_socket_fd(char *srv_ip, sem_t *passive_port_count, int qlen);

#endif