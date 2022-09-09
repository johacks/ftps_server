/**
 * @file ftp_files.c
 * @author Joaquín Jiménez López de Castro (joaquin.jimenezl@estudiante.uam.es)
 * @brief Manipulation of paths, files and directories
 * @version 1.0
 * @date 04-14-2020
 *
 * @copyright Copyright (c) 2020
 *
 */

#define _DEFAULT_SOURCE /*!< Access to GNU functions*/
#include "utils.h"
#include "network.h"
#include "ftp.h"
#include "config_parser.h"
#include "ftp_files.h"
#define SEND_BUFFER 1024 * 1024                                                              /*!< send buffer size*/
#define RECV_BUFFER 1024 * 1024                                                              /*!< receive buffer size*/
#define IP_LEN sizeof("xxx.xxx.xxx.xxx")                                                     /*!< Size of ipv4*/
#define LS_CMD "ls -l1 --numeric-uid-gid --hyperlink=never --time-style=iso --color=never '" /*!< Command ls*/
#define IP_FIELD_LEN sizeof("xxx")                                                           /*!< Size of an ipv4 subfield*/
#define VIRTUAL_ROOT "/"                                                                     /*!< Root*/

static char root[SERVER_ROOT_MAX] = "";
static size_t root_size;

/**
 * @brief For the convenience of the programmer so as not to have to indicate the path
 * on each call, the path is set to a static variable
 * @param server_root
 */
void set_root_path(char *server_root)
{
    strncpy(root, server_root, SERVER_ROOT_MAX - 1);
    root_size = strlen(root);
}

/**
 * @brief Clears a path and sets the absolute path real_path, using the current directory
 * if path is relative to be able to generate it
 *
 * @param current_dir
 * @parampath
 * @param real_path
 * @return int
 */
int get_real_path(char *current_dir, char *path, char *real_path)
{
    char *buff = alloca((strlen(current_dir) + strlen(path) + 1) * sizeof(char)); /*'alloca' frees function memory automatically*/
    if (!buff)
        return -1;

    /*If the given path is relative, we will use the current directory*/
    if (path[0] == '\0' || path[0] != '/')
    {
        strcpy(buff, current_dir);
        strcat(buff, "/");
        strcat(buff, path);
    }
    /*If the path is absolute, we concatenate to root*/
    else
    {
        strcpy(buff, root);
        strcat(buff, path);
    }

    int ret = 1;
    if (!realpath(buff, real_path))
    {
        if (errno != ENOENT)
            return -2;
        char *buff2 = alloca(strlen(buff) + 1);
        if (!buff2)
            return -1;
        strcpy(buff2, buff);
        /*If it's because of non-existent file, check to see if at least the directory exists*/
        if (!realpath(dirname(buff), real_path))
            return -2;
        /*If the directory does exist, possibly the path is to a file to be created*/
        strcat(real_path, "/");
        strcat(real_path, basename(buff2));
        ret = 0;
    }

    /*Finally, check that the path is root or a subdirectory of it*/
    if (memcmp(real_path, root, root_size))
        return -2;
    /*After the root string, either the path ends, or there are subdirectories, which do not contain '..' thanks to realpath*/
    if (!(real_path[root_size] == '\0' || real_path[root_size] == '/'))
        return -2;

    /*Correct and ready path in real_path*/
    return ret;
}

/**
 * @brief Returns a file with the output of ls
 *
 * @param path Unclean path
 * @param current_dir Current FTP session directory
 * @return FILE*Like any other FILE, however, it must be closed with pclose
 */
FILE *list_directories(char *path, char *current_dir)
{
    char *buff = alloca((sizeof(LS_CMD) + strlen(current_dir) + strlen(path) + 1) * sizeof(char));
    if (!buff)
        return NULL;
    /*Recoger path*/
    strcpy(buff, LS_CMD);
    if (get_real_path(current_dir, path, &buff[strlen(buff)]) < 0)
        return NULL;
    strcat(buff, "'"); /*Quote from filename*/
                       /*Call to ls, redirecting the output*/
    FILE *output = popen(buff, "r");
    return output;
}

/**
 * @brief Opens a file, using current_dir to locate it
 *
 * @param path relative or absolute path to the file
 * @param current_dir current directory, possibly required
 * @param mode file opening mode
 * @return FILE*
 */
FILE *file_open(char *path, char *current_dir, char *mode)
{
    char *buff = alloca((strlen(current_dir) + strlen(path) + 1) * sizeof(char));
    if (!buff)
        return NULL;
    /*Recoger path*/
    if (get_real_path(current_dir, path, buff) < 0)
        return NULL;

    /*Call to fopen with the path already complete*/
    return fopen(buff, mode);
}

/**
 * @brief Indicates if a path points to a directory
 *
 * @param path Path to directory
 * @return int 1 if true
 */
int path_is_dir(char *path)
{
    struct stat st;
    if (stat(path, &st) < 0)
        return 0;
    return S_ISDIR(st.st_mode);
}

/**
 * @brief Indicates if a path points to a file
 *
 * @param path Path to file
 * @return int 1 if true
 */
int path_is_file(char *path)
{
    struct stat st;
    if (stat(path, &st) < 0)
        return 0;
    return S_ISREG(st.st_mode);
}

/**
 * @brief Modifies the current directory
 *
 * @param current_dir Current directory
 * @param path Path to the new directory
 * @return int 1 if correct, -1 out of memory, -2 wrong path
 */
int ch_current_dir(char *current_dir, char *path)
{
    int ret;
    char *buff = alloca((strlen(current_dir) + strlen(path) + 1) * sizeof(char));
    if (!buff)
        return -1;
    /*Recoger path*/
    if ((ret = get_real_path(current_dir, path, buff)) < 0)
        return ret;

    /*Access permissions and directory exists*/
    if (access(buff, R_OK) != 0)
        return -2;

    /*Must be a directory*/
    if (!path_is_dir(buff))
        return -2;

    /*All ok, change the current directory*/
    strcpy(current_dir, buff);
    return 1;
}

/**
 * @brief Change to parent directory
 *
 * @param current_dir Current directory
 * @return int 0 if already rooted, 1 if change effective, -1 out of memory
 */
int ch_to_parent_dir(char *current_dir)
{
    char *buff = alloca((strlen(current_dir) + 1) * sizeof(char));
    if (!buff)
        return -1;
    /*Get path from the upper directory, if we are already root, we do not change*/
    if (get_real_path(current_dir, "../", buff) < 0)
        return 0;
    strcpy(current_dir, buff);
    return 1;
}

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
ssize_t send_buffer(struct TLSContext *ctx, int socket_fd, char *buf, size_t buf_len, int ascii_mode)
{
    /*Transmission in VST ascii mode*/
    if (ascii_mode)
    {
        char *ascii_buf = alloca(2 * buf_len); /*can grow up to twice*/
        if (!ascii_buf)
            return -1;
        size_t new_buflen = 0;
        for (int i = 0; i < buf_len; i++)
        {
            if (buf[i] == '\n') /*Server line breaks are \n*/
                ascii_buf[new_buflen++] = '\r';
            ascii_buf[new_buflen++] = (buf[i] & 0x7F); /*Most significant bit to 0 always*/
        }
        return ssend(ctx, socket_fd, ascii_buf, new_buflen, MSG_NOSIGNAL); /*Send filtered content*/
    }
    return ssend(ctx, socket_fd, buf, buf_len, MSG_NOSIGNAL); /*send content*/
}

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
ssize_t send_file(struct TLSContext *ctx, int socket_fd, FILE *f, int ascii_mode, int *abort_transfer)
{
    int aux = 0;
    ssize_t sent_b, read_b, total = 0;
    char buf[SEND_BUFFER];
    if (!abort_transfer)
        abort_transfer = &aux;

    /*Send content of f in blocks*/
    while (!(*abort_transfer) && (read_b = fread(buf, sizeof(char), SEND_BUFFER, f)))
    {
        if (*abort_transfer)
            return 0; /*You can change an external flag to cancel the transfer*/
        if ((sent_b = send_buffer(ctx, socket_fd, buf, read_b, ascii_mode)) < 0)
            return -1;
        total += sent_b;
    }
    return total; /*successful transfer*/
}

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
ssize_t read_to_buffer(struct TLSContext *ctx, int socket_fd, char *dest, size_t buf_len, int ascii_mode)
{
    /*Filter what is read from ascii to local (CRLF to LF)*/
    if (ascii_mode)
    {
        char *ascii_buf = alloca(buf_len);
        ssize_t n_read, new_buflen = 0;
        if ((n_read = srecv(ctx, socket_fd, ascii_buf, buf_len, MSG_NOSIGNAL)) <= 0)
            return n_read;
        for (int i = 0; i < n_read; i++)
            if (ascii_buf[i] != '\r')
                dest[new_buflen++] = ascii_buf[i];
        return new_buflen;
    }
    return srecv(ctx, socket_fd, dest, buf_len, MSG_NOSIGNAL);
}

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
ssize_t read_to_file(struct TLSContext *ctx, FILE *f, int socket_fd, int ascii_mode, int *abort_transfer)
{
    int aux = 0;
    ssize_t sent_b, read_b, total = 0;
    char buf[RECV_BUFFER];
    if (!abort_transfer)
        abort_transfer = &aux;

    /*Read from buffer to buffer until finished or interrupted*/
    while (!(*abort_transfer) && ((read_b = read_to_buffer(ctx, socket_fd, buf, RECV_BUFFER, ascii_mode)) > 0))
    {
        if ((sent_b = fwrite(buf, sizeof(char), read_b, f)) != read_b)
            return -1;
        total += read_b;
    }
    return total;
}

/**
 * @brief Parse a port string of format xxx,xxx,xxx,xxx,ppp,ppp
 *
 * @param port_string PORT command argument
 * @param clt_ip Buffer where the client's ip will be stored
 * @param clt_port Where the client port will be stored
 * @return int less than 0 on error
 */
int parse_port_string(char *port_string, char *clt_ip, int *clt_port)
{
    int len, comma_count;
    size_t field_width, command_len = strlen(port_string);
    char *start = port_string;

    /*IP*/
    for (len = 0, comma_count = 4; comma_count; comma_count--, port_string = &port_string[field_width + 1])
    {
        field_width = strcspn(port_string, ",");
        /*correct field*/
        if (field_width >= IP_FIELD_LEN || !field_width || !is_number(port_string, field_width))
            return -1;
        if (comma_count > 1)
            port_string[field_width] = '.';
        len += (field_width + 1);
        if (len >= command_len)
            return -1;
    }
    strncpy(clt_ip, start, len - 1); /*Copy IP omitting the , end*/

    /*PUERTO*/
    *clt_port = 0;
    /*Byte superior*/
    field_width = strcspn(port_string, ",");
    if (!field_width || field_width > sizeof("xxx") || port_string[field_width] != ',' || !is_number(port_string, field_width))
        return -1;
    port_string[field_width] = '\0';
    *clt_port += (atoi(port_string) << 8); /*Multiply by 256*/
    port_string = &port_string[field_width + 1];
    /*Lower byte*/
    field_width = strlen(port_string);
    if (!field_width || field_width > sizeof("xxx") || !is_number(port_string, field_width))
        return -1;
    *clt_port += atoi(port_string);
    return 1;
}

/**
 * @brief Create a data socket for the client to connect to
 *
 * @param srv_ip IP of the server
 * @param passive_port_count Semaphore with the number of data connections created
 * @param socket_fd Resulting socket
 * @return int less than 0 if error, otherwise port
 */
int passive_data_socket_fd(char *srv_ip, sem_t *passive_port_count, int *socket_fd)
{
    /*No data sockets available at this time*/
    if (sem_trywait(passive_port_count) == -1 && errno == EAGAIN)
        return -1;
    if ((*socket_fd = socket_srv("tcp", 10, 0, srv_ip)) < 0)
        return *socket_fd;
    /*Set a timeout*/
    set_socket_timeouts(*socket_fd, DATA_SOCKET_TIMEOUT);
    /*Return the port that has been found*/
    struct sockaddr_in addrinfo;
    socklen_t info_len = sizeof(struct sockaddr);
    getsockname(*socket_fd, (struct sockaddr *)&addrinfo, &info_len);
    return ntohs(addrinfo.sin_port);
}

/**
 * @brief Generates port string for PASV command
 *
 * @param ip IP of the server
 * @param port Data port
 * @param buf Buffer to store the result
 * @return char*Result
 */
char *make_port_string(char *ip, int port, char *buf)
{
    int i;
    strcpy(buf, ip);
    for (i = 0; i < strlen(buf); i++) /*Comma dots in ip*/
        if (buf[i] == '.')
            buf[i] = ',';
    strcat(&buf[i++], ",");      /*And a comma at the end*/
    div_t p1p2 = div(port, 256); /*Get both port numbers and put them at the end*/
    sprintf(&buf[i], "%d,%d", p1p2.quot, p1p2.rem);
    return buf;
}

/**
 * @brief Returns a pointer to the path without root
 *
 * @param full_path Full path
 * @return char*Pointer to path without root
 */
char *path_no_root(char *full_path)
{
    if (full_path[root_size] == '/') /*Asked for a path beyond root*/
        return &full_path[root_size];
    return VIRTUAL_ROOT; /*Asked for a path that was already root*/
}
