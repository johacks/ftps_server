/**
 * @file ftp_files.h
 * @author Joaquín Jiménez López de Castro (joaquin.jimenezl@estudiante.uam.es)
 * @brief Manipulation of file, paths and data connection
 * @version 1.0
 * @date 04-15-2020
 *
 * @copyright Copyright (c) 2020
 *
 */

#ifndef FTP_FILES_H
#define FTP_FILES_H

#include "utils.h"
#include "network.h"
#include "tlse.h"
#define DATA_SOCKET_TIMEOUT 60 /*!< Maximum seconds of timeout in data connection*/
/**
 * @brief Defines the possible states of an FTP data connection
 *
 */
typedef enum _data_conn_states
{
    DATA_CONN_CLOSED,    /*!< There is no socket currently open*/
    DATA_CONN_AVAILABLE, /*!< There is a socket ready to transmit the data*/
    DATA_CONN_BUSY       /*!< There is a data socket open, but it is already transmitting data*/
} data_conn_states;

/**
 * @brief Defines the information of an FTP data connection
 *
 */
typedef struct _data_conn
{
    int is_passive;                            /*!< Indicates if the connection is active or passive*/
    int socket_fd;                             /*!< Socket descriptor in passive mode*/
    int conn_fd;                               /*!< Connection descriptor in active and passive mode*/
    TLS *context;                              /*!< TLS data connection context*/
    char client_ip[sizeof("XXX.XXX.XXX.XXX")]; /*!< IP address of the client for a connection in active mode*/
    int client_port;                           /*!< Client port for a connection in active mode*/
    int abort;                                 /*!< Indicates that the current transmission should be aborted*/
    data_conn_states conn_state;               /*!< Data connection status*/
    sem_t mutex;                               /*!< Mutex for connection state change*/
    sem_t data_conn_sem;                       /*!< If set to 1, you can continue the data thread*/
    sem_t control_conn_sem;                    /*!< If set to 1, you can continue the control thread*/
} data_conn;

/**
 * @brief For the convenience of the programmer so as not to have to indicate the path
 * on each call, the path is set to a static variable
 * @param server_root
 */
void set_root_path(char *server_root);

/**
 * @brief Clears a path and sets the absolute path real_path, using the current directory
 * if path is relative to be able to generate it
 *
 * @param current_dir
 * @parampath
 * @param real_path
 * @return int
 */
int get_real_path(char *current_dir, char *path, char *real_path);

/**
 * @brief Returns a file with the output of ls
 *
 * @param path Unclean path
 * @param current_dir Current FTP session directory
 * @return FILE*Like any other FILE, however, it must be closed with pclose
 */
FILE *list_directories(char *path, char *current_dir);

/**
 * @brief Opens a file, using current_dir to locate it
 *
 * @param path relative or absolute path to the file
 * @param current_dir current directory, possibly required
 * @param mode file opening mode
 * @return FILE*
 */
FILE *file_open(char *path, char *current_dir, char *mode);

/**
 * @brief Indicates if a path points to a directory
 *
 * @param path Path to directory
 * @return int 1 if true
 */
int path_is_dir(char *path);

/**
 * @brief Indicates if a path points to a file
 *
 * @param path Path to file
 * @return int 1 if true
 */
int path_is_file(char *path);

/**
 * @brief Modifies the current directory
 *
 * @param current_dir Current directory
 * @param path Path to the new directory
 * @return int 1 if correct, -1 out of memory, -2 wrong path
 */
int ch_current_dir(char *current_dir, char *path);

/**
 * @brief Change to parent directory
 *
 * @param current_dir Current directory
 * @return int 0 if already rooted, 1 if change effective, -1 out of memory
 */
int ch_to_parent_dir(char *current_dir);

/**
 * @brief Send the contents of a buffer through a socket with possibility of
 * apply an ascii filter
 *
 * @param ctx TLS Context
 * @param socket_fd Socket descriptor
 * @param buf to send
 * @param buf_len how much to send
 * @param ascii_mode If not 0, convert newlines to universal format
 * @return ssize_t if less than 0, error
 */
ssize_t send_buffer(struct TLSContext *ctx, int socket_fd, char *buf, size_t buf_len, int ascii_mode);

/**
 * @brief Send the content of f through a socket
 *
 * @param ctx TLS context
 * @param socket_fd Socket descriptor
 * @param f File to open
 * @param ascii_mode Ascii mode
 * @param abort_transfer Allows you to cancel transfer
 * @return ssize_t
 */
ssize_t send_file(struct TLSContext *ctx, int socket_fd, FILE *f, int ascii_mode, int *abort_transfer);

/**
 * @brief Read content from un socket to un buffer
 *
 * @param ctx TLS context
 * @param socket_fd Socket origin
 * @param dest Buffer destination
 * @param buf_len Buffer size
 * @param ascii_mode Ascii mode
 * @return ssize_t reads
 */
ssize_t read_to_buffer(struct TLSContext *ctx, int socket_fd, char *dest, size_t buf_len, int ascii_mode);

/**
 * @brief Read the contents of a socket to a file
 *
 * @param ctx TLS Context
 * @param f Destination file.
 * @param socket_fd source socket
 * @param ascii_mode FTP transfer mode
 * @param abort_transfer Allows to abort the transfer
 * @return ssize_t bytes read
 */
ssize_t read_to_file(struct TLSContext *ctx, FILE *f, int socket_fd, int ascii_mode, int *abort_transfer);

/**
 * @brief Parse a port string of format xxx,xxx,xxx,xxx,ppp,ppp
 *
 * @param port_string PORT command argument
 * @param clt_ip Buffer where the client's ip will be stored
 * @param clt_port Where the client port will be stored
 * @return int less than 0 on error
 */
int parse_port_string(char *port_string, char *clt_ip, int *clt_port);

/**
 * @brief Create a data socket for the client to connect to
 *
 * @param srv_ip IP of the server
 * @param passive_port_count Semaphore with the number of data connections created
 * @param socket_fd Resulting socket
 * @return int less than 0 if error, otherwise port
 */
int passive_data_socket_fd(char *srv_ip, sem_t *passive_port_count, int *socket_fd);

/**
 * @brief Generates port string for PASV command
 *
 * @param ip IP of the server
 * @param port Data port
 * @param buf Buffer to store the result
 * @return char*Result
 */
char *make_port_string(char *ip, int port, char *buf);

/**
 * @brief Returns a pointer to the path without root
 *
 * @param full_path Full path
 * @return char*Pointer to path without root
 */
char *path_no_root(char *full_path);

#endif