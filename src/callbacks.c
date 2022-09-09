/**
 * @file callbacks.c
 * @author Joaquín Jiménez López de Castro (joaquin.jimenezl@estudiante.uam.es)
 * @brief Defines the callbacks corresponding to each command
 * @version 1.0
 * @date 12-04-2020
 *
 * @copyright Copyright (c) 2020
 *
 */

#include "network.h"
#include "utils.h"
#include "ftp.h"
#include "callbacks.h"
#include "ftp_files.h"
#include "config_parser.h"
#include "authenticate.h"

/*Define the array of callbacks*/
#define C(x) x##_cb, /*!< Callback function name associated with an implemented command*/
/*array of functions*/
static const callback callbacks[IMP_COMMANDS_TOP] = {IMPLEMENTED_COMMANDS};
#undef C

/*Some widely repeated check macros*/
#define CHECK_USERNAME(s, c)                            \
    {                                                   \
        if (!s->authenticated)                          \
        {                                               \
            set_command_response(c, CODE_530_NO_LOGIN); \
            return CALLBACK_RET_PROCEED;                \
        }                                               \
    } /*!< Check in a callback that the user is logged in*/

#define RESOLVE_PATH(s, c, buf, new)                                              \
    {                                                                             \
        if (get_real_path(s->current_dir, c->command_arg, buf) < ((new) ? 0 : 1)) \
        {                                                                         \
            set_command_response(c, CODE_550_NO_ACCESS);                          \
            return CALLBACK_RET_PROCEED;                                          \
        }                                                                         \
    } /*!< Checks in a callback that the path passed as argument is correct and stores it in buf*/
/**
 * @brief Calls the callback corresponding to a command
 *
 * @param server_conf General server configuration, read only
 * @param session Value of the session after the last command
 * @param command Command that generates the callback, containing a possible argument
 * @return uintptr_t Possible return value, which would be either a pointer or an integer
 */
uintptr_t command_callback(serverconf *server_conf, session_info *session, request_info *command)
{
    return callbacks[command->implemented_command](server_conf, session, command);
}

/*CALLBACKS THAT USE THE DATA CONNECTION*/
/**
 * @brief Defines the data that is passed to a data thread
 *
 */
typedef struct _data_thread_args
{
    pthread_t thread;        /*!< Thread used for data connection*/
    serverconf *server_conf; /*!< Server configuration*/
    session_info *session;   /*!< Sesion FTP*/
    request_info *command;   /*!< Command that generates callback*/
} data_thread_args;

#define DATA_cb(COMMAND)                                                            \
    CALLBACK_RET COMMAND##_cb(CALLBACK_ARGUMENTS)                                   \
    {                                                                               \
        data_thread_args *args = malloc(sizeof(data_thread_args));                  \
        if (!args)                                                                  \
            return CALLBACK_RET_END_CONNECTION;                                     \
        args->server_conf = server_conf;                                            \
        args->session = session;                                                    \
        args->command = command;                                                    \
        if (pthread_create(&(args->thread), NULL, COMMAND##_cb_thread, args) == -1) \
        {                                                                           \
            free(args);                                                             \
            return CALLBACK_RET_END_CONNECTION;                                     \
        }                                                                           \
        pthread_detach(args->thread);                                               \
        return CALLBACK_RET_PROCEED;                                                \
    } /*!< Acts as middleware between the data command callback and the data command thread*/
/*Synchronization between two threads to advance at the same time after an event*/
#define RENDEZVOUS(mut1, mut2) \
    {                          \
        sem_post(&(mut1));     \
        sem_wait(&(mut2));     \
        sem_post(&(mut1));     \
    } /*!< Two processes are synchronized using two mutex*/
/*Release the initial resources of a thread after a premature end*/
#define THREAD_PREMATURE_EXIT(t)                                  \
    {                                                             \
        RENDEZVOUS(t->session->data_connection->data_conn_sem,    \
                   t->session->data_connection->control_conn_sem) \
        sem_post(&(t->session->data_connection->data_conn_sem));  \
        free(t);                                                  \
        return NULL;                                              \
    } /*!< Exits the execution of a thread at the beginning of it, freeing resources*/
/**
 * @brief Create a secure connection in the mode corresponding to passive or active
 *
 * @param server_conf Server configuration
 * @param session FTP session
 * @param command Command that generates the callback, the response will be saved here
 * @return int 1 if all ok, -1 if error
 */
int make_data_conn(serverconf *server_conf, session_info *session, request_info *command)
{
    data_conn *dc = session->data_connection;
    char *expected_public_key = get_client_public_key(session->context);
    if (!session->authenticated)
        set_command_response(command, CODE_530_NO_LOGIN);
    else if (dc->conn_state != DATA_CONN_AVAILABLE || !expected_public_key) /*Absence of a PASV or PORT*/
        set_command_response(command, CODE_503_BAD_SEQUENCE);
    else if (!dc->is_passive)
    {
        /*Connect, checking that the other end uses the same certificate as in the control connection*/
        dc->conn_fd = connect_and_handshake(server_conf->server_ctx, &(dc->context), expected_public_key,
                                            dc->client_port, FTP_DATA_PORT, dc->client_ip, server_conf->ftp_host);
        set_socket_timeouts(dc->conn_fd, DATA_SOCKET_TIMEOUT);
    }
    else /*passive mode*/
        dc->conn_fd = tls_accept_and_handshake(server_conf->server_ctx, &(dc->context), dc->socket_fd, expected_public_key);
    /*Check all went well*/
    if (dc->conn_fd < 0)
        set_command_response(command, CODE_425_CANNOT_OPEN_DATA, strerror(errno));
    else /*All OK when establishing a secure connection*/
    {
        dc->conn_state = DATA_CONN_BUSY;
        return 1;
    }
    return -1;
}

/*This macro checks that there is a data connection available, undoing the semaphores
corresponding and leaving the thread if it is not the case*/
#define CHECK_DATA_PORT(t)                                              \
    {                                                                   \
        if (make_data_conn(t->server_conf, t->session, t->command) < 0) \
            THREAD_PREMATURE_EXIT(t)                                    \
    } /*!< Checks that a data connection can be established correctly and exits if not*/

/**
 * @brief Send a file
 *
 * @param args Thread arguments
 * @return void*NULL
 */
void *RETR_cb_thread(void *args)
{
    data_thread_args *t_args = (data_thread_args *)args;
    CHECK_DATA_PORT(t_args) /*Create data connection*/
                            /*Get the element to give*/
    char path[XXL_SZ] = "";
    if (get_real_path(t_args->session->current_dir, t_args->command->command_arg, path) < 1 || !path_is_file(path))
    {
        set_command_response(t_args->command, CODE_550_NO_ACCESS);
        THREAD_PREMATURE_EXIT(t_args);
    }
    /*Indicate first response to the control thread: 150, sending file*/
    set_command_response(t_args->command, CODE_150_RETR, path_no_root(path));
    /*Follow the marked concurrency protocol*/
    RENDEZVOUS(t_args->session->data_connection->data_conn_sem, t_args->session->data_connection->control_conn_sem)
    /*Start the transfer*/
    FILE *f = fopen(path, "rb");
    if (!f)
        set_command_response(t_args->command, CODE_550_NO_ACCESS);
    else
    {
        ssize_t sent = send_file(t_args->session->data_connection->context, t_args->session->data_connection->conn_fd,
                                 f, t_args->session->ascii_mode, &(t_args->session->data_connection->abort));
        if (sent < 0)
            set_command_response(t_args->command, CODE_550_NO_ACCESS);
        else
            set_command_response(t_args->command, CODE_226_DATA_TRANSFER, sent);
        fclose(f);
    }
    sem_post(&(t_args->session->data_connection->data_conn_sem)); /*Indicate transmission finished*/
    free(t_args);
    return NULL;
}

/**
 * @brief List content of a path
 *
 * @param args Thread arguments
 * @return void*
 */
void *LIST_cb_thread(void *args)
{
    data_thread_args *t_args = (data_thread_args *)args;
    CHECK_DATA_PORT(t_args) /*Create data connection*/
                            /*Get the element to give*/
    char path[XXL_SZ] = "";
    if (get_real_path(t_args->session->current_dir, t_args->command->command_arg, path) < 1)
    {
        set_command_response(t_args->command, CODE_550_NO_ACCESS);
        THREAD_PREMATURE_EXIT(t_args);
    }
    /*Indicate first response to the control thread: 150, sending file*/
    set_command_response(t_args->command, CODE_150_LIST);
    /*Follow the marked concurrency protocol*/
    RENDEZVOUS(t_args->session->data_connection->data_conn_sem, t_args->session->data_connection->control_conn_sem)
    /*Start the transfer*/
    FILE *f = list_directories(t_args->command->command_arg, t_args->session->current_dir);
    if (!f)
        set_command_response(t_args->command, CODE_550_NO_ACCESS);
    else
    {
        ssize_t sent = send_file(t_args->session->data_connection->context, t_args->session->data_connection->conn_fd,
                                 f, t_args->session->ascii_mode, &(t_args->session->data_connection->abort));
        if (sent < 0)
            set_command_response(t_args->command, CODE_550_NO_ACCESS);
        else
            set_command_response(t_args->command, CODE_226_DATA_TRANSFER, sent);
        pclose(f);
    }
    sem_post(&(t_args->session->data_connection->data_conn_sem)); /*Indicate transmission finished*/
    free(t_args);
    return NULL;
}

/**
 * @brief Receive a file from the client
 *
 * @param args Thread arguments
 * @return void*NULL
 */
void *STOR_cb_thread(void *args)
{
    data_thread_args *t_args = (data_thread_args *)args;
    CHECK_DATA_PORT(t_args) /*Create data connection*/
                            /*Get the element to give*/
    char path[XXL_SZ] = "";
    if (get_real_path(t_args->session->current_dir, t_args->command->command_arg, path) < 0)
    {
        set_command_response(t_args->command, CODE_501_BAD_ARGS);
        THREAD_PREMATURE_EXIT(t_args);
    }
    /*Indicate first response to the control thread: 150, sending file*/
    set_command_response(t_args->command, CODE_150_STOR, path_no_root(path));
    /*Follow the marked concurrency protocol*/
    RENDEZVOUS(t_args->session->data_connection->data_conn_sem, t_args->session->data_connection->control_conn_sem)
    /*Start the transfer*/
    FILE *f = fopen(path, "wb");
    if (!f) /*Possible error when opening file*/
        set_command_response(t_args->command, (errno == EACCES) ? CODE_550_NO_ACCESS : CODE_452_NO_SPACE);
    else /*read file*/
    {
        ssize_t sent = read_to_file(t_args->session->data_connection->context, f, t_args->session->data_connection->conn_fd,
                                    t_args->session->ascii_mode, &(t_args->session->data_connection->abort));
        if (sent < 0)
            set_command_response(t_args->command, CODE_451_DATA_CONN_LOST);
        else
            set_command_response(t_args->command, CODE_226_DATA_TRANSFER, sent);
        fclose(f);
    }
    sem_post(&(t_args->session->data_connection->data_conn_sem)); /*Indicate transmission finished*/
    free(t_args);
    return NULL;
}

/*Generates callbacks of functions that need data thread*/
/**
 * @brief Construct middleware function between the RETR callback and a thread to serve it
 */
DATA_cb(RETR)

    /**
     * @brief Construct middleware function between the LIST callback and a thread to serve it
     */
    DATA_cb(LIST)

    /**
     * @brief Construct middleware function between the STOR callback and a thread to serve it
     */
    DATA_cb(STOR) /*!< Function that creates a thread to serve STOR*/
    /*CONTROL CALLBACKS*/
    /**
     * @brief List of commands implemented by the server
     *
     * @param server_conf Server configuration
     * @param session FTP session
     * @param command Command that generates the callback
     * @return uintptr_t
     */
    uintptr_t HELP_cb(serverconf *server_conf, session_info *session, request_info *command)
{
    strcpy(command->response, CODE_214_HELP);
    for (int i = 0; i < IMP_COMMANDS_TOP; i++)
    {
        strcat(command->response, get_imp_command_name((imp_commands)i));
        strcat(command->response, ",");
    }
    command->response_len = strlen(command->response);
    command->response[command->response_len - 1] = '\r'; /*Change last comma to line break*/
    command->response[command->response_len++] = '\n';
    return CALLBACK_RET_PROCEED;
}

/**
 * @brief Offers a port to the client for data transmission
 *
 * @param server_conf Server configuration
 * @param session Current session, information about the socket that is opened will be saved
 * @param command Command that generates the callback
 * @return uintptr_t
 */
uintptr_t PASV_cb(serverconf *server_conf, session_info *session, request_info *command)
{
    CHECK_USERNAME(session, command);
    int port;
    char port_string[sizeof("xxx,xxx,xxx,xxx,ppp,ppp")];

    MUTEX_DO(
        session->data_connection->mutex,                              /*Atomic operation*/
        if (session->data_connection->conn_state != DATA_CONN_CLOSED) /*Open data connection*/
        set_command_response(command, CODE_421_DATA_OPEN);
        /*Open and listen on a passive port*/
        else if ((port = passive_data_socket_fd(server_conf->ftp_host, /*Failed to open socket*/
                                                &(server_conf->free_passive_ports), &(session->data_connection->socket_fd))) == -1)
            set_command_response(command, CODE_425_CANNOT_OPEN_DATA, strerror(errno));
        else /*Generate PASV response string*/
        {
            set_command_response(command, CODE_227_PASV_RES, make_port_string(server_conf->ftp_host, port, port_string));
            session->data_connection->conn_state = DATA_CONN_AVAILABLE;
            session->data_connection->is_passive = 1;
        })
    return CALLBACK_RET_PROCEED;
}

/**
 * @brief Connect to a client port in data transmission mode
 *
 * @param server_conf Server configuration
 * @param session Current session, information about the socket that is opened will be saved
 * @param command Command that generates the callback, the ip and data port of the client are expected
 * @return uintptr_t
 */
uintptr_t PORT_cb(serverconf *server_conf, session_info *session, request_info *command)
{
    CHECK_USERNAME(session, command)
    if (command->command_arg[0] == '\0') /*Expect a Port string*/
    {
        set_command_response(command, CODE_501_BAD_ARGS);
        return CALLBACK_RET_PROCEED;
    }
    MUTEX_DO(
        session->data_connection->mutex, /*Protected operation*/
        if (session->data_connection->conn_state != DATA_CONN_CLOSED)
            set_command_response(command, CODE_421_DATA_OPEN); /*Intentar parsear port string*/
        else if (parse_port_string(command->command_arg, session->data_connection->client_ip, &(session->data_connection->client_port)) < 0)
            set_command_response(command, CODE_501_BAD_ARGS); /*Failed to parse port string*/
        else {
            set_command_response(command, CODE_200_OP_OK); /*Port string successfully parsed*/
            session->data_connection->conn_state = DATA_CONN_AVAILABLE;
            session->data_connection->is_passive = 0;
        })
    return CALLBACK_RET_PROCEED;
}

/**
 * @brief Delete a file from the server
 *
 * @param server_conf Server configuration
 * @param session Current FTP session
 * @param command Command that generates the callback, it is expected to find the name of the file to delete
 * @return uintptr_t
 */
uintptr_t DELE_cb(serverconf *server_conf, session_info *session, request_info *command)
{
    CHECK_USERNAME(session, command)
    if (command->command_arg[0] == '\0') /*No filename*/
        set_command_response(command, CODE_501_BAD_ARGS);
    else
    {
        char path[XXL_SZ] = "";
        RESOLVE_PATH(session, command, path, 0); /*Fetch file to delete*/
        if (!path_is_file(path))
            set_command_response(command, CODE_550_NO_DELE, "No es un fichero"); /*Check that it is a file*/
        else
        {
            char rm_cmd[XXL_SZ + sizeof("rm -r ")];
            sprintf(rm_cmd, "rm -r %s", path);                   /*The system command will be used*/
            if (system(rm_cmd) != 0 || access(path, F_OK) != -1) /*The file must have been deleted*/
                set_command_response(command, CODE_550_NO_DELE, strerror(errno));
            else
                set_command_response(command, CODE_250_DELE_OK, path_no_root(path));
        }
    }
    return CALLBACK_RET_PROCEED;
}

/**
 * @brief Delete a directory recursively
 *
 * @param server_conf Server configuration
 * @param session Current session
 * @param command Command that generates the callback, it is expected to find the name of the directory to delete
 * @return uintptr_t
 */
uintptr_t RMDA_cb(serverconf *server_conf, session_info *session, request_info *command)
{
    CHECK_USERNAME(session, command)
    if (command->command_arg[0] == '\0') /*No directory name*/
        set_command_response(command, CODE_501_BAD_ARGS);
    else
    {
        char path[XXL_SZ] = "";
        RESOLVE_PATH(session, command, path, 0); /*Fetch directory to delete*/
        if (!path_is_dir(path))
            set_command_response(command, CODE_550_NO_DELE, "No es un directorio"); /*Check that it is directory*/
        else
        {
            char rm_cmd[XXL_SZ + sizeof("rm -rfd ")];
            sprintf(rm_cmd, "rm -rfd %s", path);                 /*The system command will be used*/
            if (system(rm_cmd) != 0 || access(path, F_OK) != -1) /*The directory must have been deleted*/
                set_command_response(command, CODE_550_NO_DELE, strerror(errno));
            else
                set_command_response(command, CODE_250_DELE_OK, path_no_root(path));
        }
    }
    return CALLBACK_RET_PROCEED;
}

/**
 * @brief Delete an empty directory
 *
 * @param server_conf Server configuration
 * @param session Current session
 * @param command Command that generates the callback, it is expected to find the name of the directory to delete
 * @return uintptr_t
 */
uintptr_t RMD_cb(serverconf *server_conf, session_info *session, request_info *command)
{
    CHECK_USERNAME(session, command)
    if (command->command_arg[0] == '\0') /*No directory name*/
        set_command_response(command, CODE_501_BAD_ARGS);
    else
    {
        char path[XXL_SZ] = "";
        RESOLVE_PATH(session, command, path, 0); /*Fetch directory to delete*/
        if (rmdir(path) == -1)
            set_command_response(command, CODE_550_NO_DELE, strerror(errno));
        else
            set_command_response(command, CODE_250_DELE_OK, path_no_root(path)); /*Return path to deleted directory*/
    }
    return CALLBACK_RET_PROCEED;
}

/**
 * @brief Create a directory
 *
 * @param server_conf Server configuration
 * @param session Current FTP session
 * @param command Command that generates the callback, expecting the name of the directory to create as an argument
 * @return uintptr_t
 */
uintptr_t MKD_cb(serverconf *server_conf, session_info *session, request_info *command)
{
    CHECK_USERNAME(session, command)
    if (command->command_arg[0] == '\0')
        set_command_response(command, CODE_501_BAD_ARGS);
    else
    {
        char path[XXL_SZ] = "";
        RESOLVE_PATH(session, command, path, 1);
        if (mkdir(path, 0777) == -1) /*Create a directory on the resolved path*/
            set_command_response(command, CODE_550_NO_ACCESS);
        else
            set_command_response(command, CODE_257_MKD_OK, path_no_root(path)); /*Returns the name of the created path*/
    }
    return CALLBACK_RET_PROCEED;
}

/**
 * @brief Change to the parent directory
 *
 * @param server_conf Server configuration
 * @param session Session, which contains the current directory
 * @param command Command that generates the callback
 * @return uintptr_t
 */
uintptr_t CDUP_cb(serverconf *server_conf, session_info *session, request_info *command)
{
    CHECK_USERNAME(session, command)
    if (ch_to_parent_dir(session->current_dir) < 0)
        return CALLBACK_RET_END_CONNECTION; /*memory error*/
    set_command_response(command, CODE_250_CHDIR_OK, path_no_root(session->current_dir));
    return CALLBACK_RET_PROCEED;
}

/**
 * @brief Change the current directory
 *
 * @param server_conf Server configuration
 * @param session Session, which contains the current directory
 * @param command Command that generates the callback. Argument is expected with the following directory
 * @return uintptr_t
 */
uintptr_t CWD_cb(serverconf *server_conf, session_info *session, request_info *command)
{
    CHECK_USERNAME(session, command)
    if (command->command_arg[0] == '\0')
        strcpy(command->command_arg, "/");
    switch (ch_current_dir(session->current_dir, command->command_arg))
    {
    case -1:
        return CALLBACK_RET_END_CONNECTION; /*memory error*/
    case -2:
        set_command_response(command, CODE_550_NO_ACCESS);
        break;
    default:
        set_command_response(command, CODE_250_CHDIR_OK, path_no_root(session->current_dir));
        break;
    }
    return CALLBACK_RET_PROCEED;
}

/**
 * @brief Print the current directory
 *
 * @param server_conf Server configuration
 * @param session Session, which contains the current directory
 * @param command Command that generates the callback
 * @return uintptr_t
 */
uintptr_t PWD_cb(serverconf *server_conf, session_info *session, request_info *command)
{
    CHECK_USERNAME(session, command)
    set_command_response(command, CODE_257_PWD_OK, path_no_root(session->current_dir));
    return CALLBACK_RET_PROCEED;
}

/**
 * @brief Ends the current session
 *
 * @param server_conf Server configuration
 * @param session Current session
 * @param command Command that generates the callback
 * @return uintptr_t CALLBACK_RET_END_CONNECTION
 */
uintptr_t QUIT_cb(serverconf *server_conf, session_info *session, request_info *command)
{
    set_command_response(command, CODE_221_GOODBYE_MSG);
    return CALLBACK_RET_END_CONNECTION;
}

/**
 * @brief Finish renaming the file
 *
 * @param server_conf Server configuration
 * @param session FTP session, expect to find source name
 * @param command Command that generates the callback
 * @return uintptr_t CALLBACK_RET_PROCEED
 */
uintptr_t RNTO_cb(serverconf *server_conf, session_info *session, request_info *command)
{
    CHECK_USERNAME(session, command)
    char path[XXL_SZ] = "";
    RESOLVE_PATH(session, command, path, 1)
    char *rnfr = (char *)get_attribute(session, RENAME_FROM_ATTR); /*Retrieve source file*/
    if (((uintptr_t)rnfr) == ATTR_NOT_FOUND)
        set_command_response(command, CODE_503_BAD_SEQUENCE);
    else
    {
        rename(rnfr, path); /*Rename the file*/
        set_command_response(command, CODE_25O_FILE_OP_OK);
    }
    return CALLBACK_RET_PROCEED;
}

/**
 * @brief Rename a file
 *
 * @param server_conf Server configuration
 * @param session FTP session, add source file attribute
 * @param command Command that generates the callback
 * @return uintptr_t CALLBACK_RET_PROCEED or CALLBACK_RET_END_CONNECTION
 */
uintptr_t RNFR_cb(serverconf *server_conf, session_info *session, request_info *command)
{
    char *path = malloc(XXL_SZ);
    if (!path)
        return CALLBACK_RET_END_CONNECTION;
    CHECK_USERNAME(session, command)
    if (get_real_path(session->current_dir, command->command_arg, path) < 1)
    {
        free(path);
        set_command_response(command, CODE_550_NO_ACCESS);
        return CALLBACK_RET_PROCEED;
    }
    set_attribute(session, RENAME_FROM_ATTR, (uintptr_t)path, 1, 1); /*session attribute: file to rename*/
    set_command_response(command, CODE_350_RNTO_NEEDED);
    return CALLBACK_RET_PROCEED;
}

/**
 * @brief Size in bytes of a file
 *
 * @param server_conf Server configuration
 * @param session FTP session
 * @param command Command sent
 * @return uintptr_t
 */
uintptr_t SIZE_cb(serverconf *server_conf, session_info *session, request_info *command)
{
    char path[XXL_SZ];
    CHECK_USERNAME(session, command)
    RESOLVE_PATH(session, command, path, 0)
    size_t s = name_file_size(path); /*Calculate file size*/
    if (s == -1)
        set_command_response(command, CODE_550_NO_ACCESS);
    else
        set_command_response(command, CODE_213_FILE_SIZE, s);
    return CALLBACK_RET_PROCEED;
}

/**
 * @brief Switch to binary or ascii mode
 *
 * @param server_conf Server configuration
 * @param session Session information
 * @param command Command that generates the callback
 * @return uintptr_t
 */
uintptr_t TYPE_cb(serverconf *server_conf, session_info *session, request_info *command)
{
    CHECK_USERNAME(session, command)
    if (!strcmp(command->command_arg, "A")) /*Ascii mode*/
        session->ascii_mode = 1;
    else if (!strcmp(command->command_arg, "I")) /*Binary mode*/
        session->ascii_mode = 0;
    else
        set_command_response(command, CODE_501_BAD_ARGS);
    set_command_response(command, CODE_200_OP_OK);
    return CALLBACK_RET_PROCEED;
}

/**
 * @brief Indicates operating system
 *
 * @param server_conf Server configuration
 * @param session Session information
 * @param command Command that generates the callback
 * @return uintptr_t
 */
uintptr_t SYST_cb(serverconf *server_conf, session_info *session, request_info *command)
{
    set_command_response(command, CODE_215_SYST, operating_system());
    return CALLBACK_RET_PROCEED;
}

/**
 * @brief Indicates additional features
 *
 * @param server_conf Server configuration
 * @param session Session information
 * @param command Command that generates the callback
 * @return uintptr_t
 */
uintptr_t FEAT_cb(serverconf *server_conf, session_info *session, request_info *command)
{
    set_command_response(command, CODE_211_FEAT);
    return CALLBACK_RET_PROCEED;
}

/**
 * @brief does nothing
 *
 * @param server_conf Server configuration
 * @param session Session information
 * @param command Command that generates the callback
 * @return uintptr_t
 */
uintptr_t NOOP_cb(serverconf *server_conf, session_info *session, request_info *command)
{
    CHECK_USERNAME(session, command)
    set_command_response(command, CODE_200_OP_OK);
    return CALLBACK_RET_PROCEED;
}

/**
 * @brief Change transmission mode (compression)
 *
 * @param server_conf Server configuration
 * @param session Session information
 * @param command Command that generates the callback, argument S, B, C or Z is expected
 * @return uintptr_t
 */
uintptr_t MODE_cb(serverconf *server_conf, session_info *session, request_info *command)
{
    CHECK_USERNAME(session, command)
    if (command->command_arg[0] == '\0')
        set_command_response(command, CODE_501_BAD_ARGS);
    else if (!strcmp(command->command_arg, "S")) /*Modo Stream*/
        set_command_response(command, CODE_200_OP_OK);
    else /*Currently only stream is supported*/
        set_command_response(command, CODE_504_UNSUPORTED_PARAM);
    return CALLBACK_RET_PROCEED;
}

/**
 * @brief Change server file structure
 *
 * @param server_conf Server configuration
 * @param session Session information
 * @param command Command that generates the callback, argument F, R or P is expected
 * @return uintptr_t
 */
uintptr_t STRU_cb(serverconf *server_conf, session_info *session, request_info *command)
{
    CHECK_USERNAME(session, command)
    if (command->command_arg[0] == '\0')
        set_command_response(command, CODE_501_BAD_ARGS);
    else if (!strcmp(command->command_arg, "F")) /*File mode*/
        set_command_response(command, CODE_200_OP_OK);
    else /*Currently only file mode is supported*/
        set_command_response(command, CODE_504_UNSUPORTED_PARAM);
    return CALLBACK_RET_PROCEED;
}

/*SECURITY CALLBACKS*/
/**
 * @brief Authenticates a user using their password
 *
 * @param server_conf Server configuration
 * @param session Current session, expect to find username attribute
 * @param command Command, expect to find a password
 * @return uintptr_t
 */
uintptr_t PASS_cb(serverconf *server_conf, session_info *session, request_info *command)
{
    if (!(session->secure && session->pbsz_sent)) /*Always AUTH first*/
    {
        set_command_response(command, CODE_522_NO_TLS);
        return CALLBACK_RET_PROCEED;
    }
    char *username = (char *)get_attribute(session, USERNAME_ATTR);
    /*If no user command has been sent with the username, require it*/
    if (((uintptr_t)username) == ATTR_NOT_FOUND)
        set_command_response(command, CODE_503_BAD_SEQUENCE);
    else
    {
        /*Authenticate user*/
        if (!(validate_pass(command->command_arg) && validate_user(username)))
            set_command_response(command, CODE_430_INVALID_AUTH);
        else
        {
            /*Correct password, indicate in the session that the user has logged in*/
            set_command_response(command, CODE_230_AUTH_OK);
            session->authenticated = 1;
        }
        explicit_bzero(command->command_arg, strlen(command->command_arg)); /*Clear raw password*/
    }
    return CALLBACK_RET_PROCEED;
}

/**
 * @brief Receive a username and save it in session
 *
 * @param server_conf Server configuration
 * @param session Current session, no attribute expected
 * @param command Command executed, expecting a username
 * @return uintptr_t returns CALLBACK_RET_END_CONNECTION if out of memory
 */
uintptr_t USER_cb(serverconf *server_conf, session_info *session, request_info *command)
{
    if (!(session->secure && session->pbsz_sent)) /*Always AUTH first*/
    {
        set_command_response(command, CODE_522_NO_TLS);
        return CALLBACK_RET_PROCEED;
    }
    char *username = malloc(FTP_USER_MAX * sizeof(char));
    if (!username)
        return CALLBACK_RET_END_CONNECTION;
    strncpy(username, command->command_arg, FTP_USER_MAX - 1);
    /*Save the username attribute which should be called free with expiration 1*/
    set_attribute(session, USERNAME_ATTR, (uintptr_t)username, 1, 1);
    /*Require the password*/
    set_command_response(command, CODE_331_PASS);
    return CALLBACK_RET_PROCEED;
}

/**
 * @brief Activate secure communication
 *
 * @param server_conf Server configuration
 * @param session FTP session
 * @param command Command that generates the callback, the TLS argument is expected
 * @return uintptr_t May return CALLBACK_RET_DONT_SEND
 */
uintptr_t AUTH_cb(serverconf *server_conf, session_info *session, request_info *command)
{
    if (session->secure) /*Do not allow double AUTH*/
    {
        set_command_response(command, CODE_503_BAD_SEQUENCE);
        return CALLBACK_RET_PROCEED;
    }
    if (strcmp(command->command_arg, "TLS")) /*Only accept TLS*/
    {
        set_command_response(command, CODE_431_INVALID_SEC, command->command_arg);
        return CALLBACK_RET_PROCEED;
    }
    char *buf = alloca(XXXL_SZ);
    send(session->clt_fd, CODE_234_START_NEG, sizeof(CODE_234_START_NEG) - 1, MSG_NOSIGNAL);
    session->context = tls_accept(server_conf->server_ctx);
    tls_request_client_certificate(session->context);                          /*We need client certificate*/
    digest_tls(session->context, session->clt_fd, buf, XXXL_SZ, MSG_NOSIGNAL); /*Send certificate request to client*/
    digest_tls(session->context, session->clt_fd, buf, XXXL_SZ, MSG_NOSIGNAL); /*Receive client certificate*/
    if (!tls_context_check_client_certificate(NULL, session->context))         /*Check client certificate*/
    {
        ssend(session->context, session->clt_fd, CODE_421_BAD_TLS_NEG, sizeof(CODE_421_BAD_TLS_NEG) - 1, MSG_NOSIGNAL);
        tls_destroy_context(session->context);
        session->context = NULL;
        return CALLBACK_RET_END_CONNECTION;
    }
    session->secure = 1; /*flag activated*/
    return CALLBACK_RET_DONT_SEND;
}

/**
 * @brief Indicates security level
 *
 * @param server_conf Server configuration
 * @param session FTP session
 * @param command Command that generates the callback, argument P is expected
 * @return uintptr_t
 */
uintptr_t PROT_cb(serverconf *server_conf, session_info *session, request_info *command)
{
    if (!session->secure || !session->pbsz_sent)
        set_command_response(command, CODE_503_BAD_SEQUENCE);
    else if (strcmp(command->command_arg, "P"))
        set_command_response(command, CODE_536_INSUFFICIENT_SEC);
    else
        set_command_response(command, CODE_200_OP_OK);
    return CALLBACK_RET_PROCEED;
}

/**
 * @brief Change buffer size
 *
 * @param server_conf Server configuration
 * @param session FTP session
 * @param command Command that generates the callback, argument 0 is expected
 * @return uintptr_t
 */
uintptr_t PBSZ_cb(serverconf *server_conf, session_info *session, request_info *command)
{
    if (!session->secure)
        set_command_response(command, CODE_503_BAD_SEQUENCE);
    else if (strcmp(command->command_arg, "0"))
        set_command_response(command, CODE_504_UNSUPORTED_PARAM);
    else
    {
        set_command_response(command, CODE_200_OP_OK);
        session->pbsz_sent = 1;
    }
    return CALLBACK_RET_PROCEED;
}

/*.......*/
/**
 * @brief This callback must exist for compilation reasons
 *
 * @param server_conf server configuration
 * @param session FTP session
 * @param command ABOR command
 * @return uintptr_t
 */
uintptr_t ABOR_cb(serverconf *server_conf, session_info *session, request_info *command)
{
    return CALLBACK_RET_PROCEED;
}