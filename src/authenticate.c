/**
 * @file authenticate.c
 * @author Joaquín Jiménez López de Castro (joaquin.jimenezl@estudiante.uam.es)
 * @brief Authentication services
 * @version 1.0
 * @date 03-17-2020
 *
 *
 */
#define _DEFAULT_SOURCE /*!< POSIX macro for some functions*/
#define _GNU_SOURCE     /*!< for other functions*/
#include "utils.h"
#include "sha256.h"
#include "authenticate.h"

#define SHA_SIZE 256          /*!< sha size*/
#define SALT_SZ 87            /*!< Maximum formatted salt size*/
#define MIN_SALT_SZ 2         /*!< Size of an unformatted salt*/
#define MAX_USER_SZ MEDIUM_SZ /*!< User size*/
#define SALT_END_CHAR '$'     /*!< Salt delimiter*/

char hashed_pass[SHA_SIZE];         /*!< Password hash. If shadow was opened, it is a hash of a hash*/
char salt[SALT_SZ];                 /*!< Salt of the password*/
char server_user[MAX_USER_SZ] = ""; /*!< Username for server access*/
int is_default_pass = 1;            /*!< 1 if credentials of the user executing the program are used*/
/**
 * @brief Get a password without doing echo by terminal
 *
 * @param target where the result will be saved
 * @param n maximum size
 * @return ssize_t bytes read
 */
ssize_t get_password(char **target, size_t n)
{
    struct termios old, new;
    int nread;
    size_t tam = n;

    /*Fetch current values ​​from terminal and store them in old*/
    if (tcgetattr(fileno(stdin), &old) != 0)
        return -1;
    /*Starting from old create a new structure with echo disabled*/
    new = old;
    new.c_lflag &= ~ECHO;
    /*Use that structure with echo disabled*/
    if (tcsetattr(fileno(stdin), TCSAFLUSH, &new) != 0)
        return -1;

    /*read password*/
    nread = getline(target, &tam, stdin);
    nread -= 1; /*line break*/
    (*target)[nread] = '\0';

    /*Retrieve old values ​​from terminal*/
    tcsetattr(fileno(stdin), TCSAFLUSH, &old);

    return nread;
}

/**
 * @brief Verifies that user is equal to the username running the process
 *
 * @param user user
 * @return int 1 if successful, 0 if not
 */
int validate_user(char *user)
{
    return strncmp(user, server_user, MAX_USER_SZ) == 0;
}

/**
 * @brief Size of the salt string of a password
 * format: $id$rounds=yyy$salt$encrypted or $id$salt$encrypted
 * @param pass password that follows the specified format
 * @return Size of everything except encrypted
 */
size_t get_salt_sz(char *pass)
{
    char *next;

    /*Check if it is salt formatted*/
    if (pass[0] != SALT_END_CHAR)
        return MIN_SALT_SZ;
    /*If not, go to next field*/
    next = strchr(&pass[1], SALT_END_CHAR);
    /*If it has the optional field rounds, we go to the next field*/
    if (!memcmp(&next[1], "rounds=", sizeof("rounds=")))
        next = strchr(&next[1], SALT_END_CHAR);
    /*Find the end of the salt field*/
    next = strchr(&next[1], SALT_END_CHAR);

    return ((size_t)(next - pass)) + 1;
}

/**
 * @brief It establishes some credentials, with default value those of the one who executes the program
 *
 * @param user Pass NULL if you want to use the user that runs the program
 * @param pass Pass NULL if you want to use the user that runs the program
 * @return int 1 if successful, 0 if not
 */
int set_credentials(char *user, char *pass)
{
    struct passwd *pw;
    struct spwd *sp;
    char *correct;
    SHA256_CTX sha;

    /*The default user is the one executing the program*/
    if (!user)
        user = get_username();
    strcpy(server_user, user);

    /*Default version, get the shadow password*/
    /*Implementation obtained from: https://stackoverflow.com/questions/6536994/authentication-of-local-unix-user-using-c*/
    if (!pass)
    {
        if (setuid(0) || seteuid(0)) /*Check for permissions*/
            return 0;
        pw = getpwnam(user); /*Find the provided user*/
        endpwent();          /*Close the password file stream*/

        if (!pw)
            return 0; /*User does not exist*/

        sp = getspnam(pw->pw_name); /*Fill the spwd structure with the appropriate information*/
        endspent();                 /*Next file entry*/
        if (sp)
            correct = sp->sp_pwdp; /*If where searches for getspnam was, we use that*/
        else
            correct = pw->pw_passwd; /*If not, use the one found by getpwnam*/

        /*Inevitably we will have to save the salt*/
        memcpy(salt, correct, get_salt_sz(correct));
    }
    /*Provide us with a password, we just hash it to sha_256*/
    else
    {
        is_default_pass = 0;
        correct = pass;
    }

    /*Save a sha256 version of the password*/
    sha256_start(&sha);
    sha256_update(&sha, (BYTE *)correct, strlen(correct));
    sha256_final(&sha, (BYTE *)hashed_pass);

    return 1;
}

/**
 * @brief Check that the username and password are correct. Must run as root
 *
 * @param pass password, must be stored in /etc/shadow
 * @return int 1 if successful authentication, 0 if not
 */
int validate_pass(char *pass)
{
    SHA256_CTX sha;
    char hashed[SHA256_BLOCK_SIZE];
    char *shadow_crypted;
    struct crypt_data data;
    data.initialized = 0;

    /*First we encrypt in shadow format if it was the default password*/
    if (is_default_pass)
    {
        shadow_crypted = crypt_r(pass, salt, &data);
        if (!shadow_crypted) /*This can happen if root mode credentials were not set*/
            return 0;
    }
    else
        shadow_crypted = pass;

    /*Now we encrypt the above to sha_256 to compare it with the sha_256 that is stored*/
    sha256_start(&sha);
    sha256_update(&sha, (BYTE *)shadow_crypted, strlen(shadow_crypted));
    sha256_final(&sha, (BYTE *)hashed);

    /*Compare the hashes*/
    return memcmp(hashed, hashed_pass, SHA256_BLOCK_SIZE) == 0;
}

/**
 * @brief Remove root permissions
 * Obtained from: https://stackoverflow.com/questions/3357737/dropping-root-privileges
 * @return int 2 if not root, 1 on success, 0 on error
 */
int drop_root_privileges()
{
    gid_t gid;
    uid_t uid;

    /*If not root, nothing to do*/
    if (getuid() != 0)
        return 2;

    /*If the program is run with sudo, we need to get sudo_gid via environment variable*/
    if ((gid = getgid()) == 0)
    {
        /*We look for value of environment variable SUDO _GID*/
        const char *sudo_gid = secure_getenv("SUDO_GID");
        if (sudo_gid == NULL)
            return 0;
        /*We convert it to long long, since it is passed to us as a string*/
        gid = (gid_t)strtoll(sudo_gid, NULL, 10);
    }
    /*If not, we can use the one given by getgid()*/

    /*If the program is run with sudo, we need to get sudo_uid via environment variable*/
    if ((uid = getuid()) == 0)
    {
        const char *sudo_uid = secure_getenv("SUDO_UID");
        if (sudo_uid == NULL)
            return 0;
        uid = (uid_t)strtoll(sudo_uid, NULL, 10);
    }
    /*If not, we can use the one given by getuid()*/

    /*We proceed to remove the permissions by matching the gid to the one obtained before*/
    if (setgid(gid) != 0)
        return 0;
    if (setuid(uid) != 0)
        return 0;

    /*Check that we have removed the privileges properly (trying to become root)*/
    if (setuid(0) == 0 || seteuid(0) == 0)
        return 0;

    return 1;
}

char get_username_buff[MAX_USER_SZ]; /*!< Read only, stores the user executing the program*/
/**
 * @brief Name of the user running the program
 *
 * @return char*Read only
 */
char *get_username()
{
    if (!getlogin())
        errexit("No se ha podido detectar el nombre del usuario que ejecuta el programa, por favor configurelo en 'server.conf'\n");
    strcpy(get_username_buff, getlogin());
    return get_username_buff;
}