/**
 * @file authenticate.c
 * @author Joaquín Jiménez López de Castro (joaquin.jimenezl@estudiante.uam.es)
 * @brief Servicios de autenticacion
 * @version 1.0
 * @date 17-03-2020
 * 
 * 
 */
#define _DEFAULT_SOURCE /*!< Macro de POSIX para algunas funciones */
#define _GNU_SOURCE /*!< Para otras funciones */
#include "utils.h"
#include "sha256.h"

#define SHA_SIZE 256 /*!< Tamaño de sha */
#define SALT_SZ 87 /*!< Tamaño de sal formateado maximo */
#define MIN_SALT_SZ 2 /*!< Tamaño de una sal no formateada */
#define MAX_USER_SZ MEDIUM_SZ /*!< Tamaño de usuario*/
#define SALT_END_CHAR '$' /*!< Delimitador de sal */

char hashed_pass[SHA_SIZE];
char salt[SALT_SZ];
char server_user[MAX_USER_SZ] = "";
int is_default_pass = 1;

/**
 * @brief Recoge una contraseña sin hacer echo por terminal
 * 
 * @param target donde se guardara el resultado
 * @param n tamaño maximo
 * @return ssize_t bytes leidos
 */
ssize_t get_password(char **target, size_t n)
{
    struct termios old, new;
    int nread;
    size_t tam = n;

    /* Recoger valores actuales de la terminal y guardarlos en old */
    if ( tcgetattr(fileno(stdin), &old) != 0 )
        return -1;
    /* Partiendo de old crear una nueva estructura con el echo desactivado */
    new = old;
    new.c_lflag &= ~ECHO;
    /* Utilizar dicha estructura con el echo desactivado */
    if ( tcsetattr(fileno(stdin), TCSAFLUSH, &new) != 0 )
        return -1;

    /* Leer contraseña */
    nread = getline(target, &tam, stdin);
    nread -= 1; /* Salto de linea */
    (*target)[nread] = '\0';

    /* Recuperar valores antiguos de terminal */
    tcsetattr(fileno (stdin), TCSAFLUSH, &old);

    return nread;
}

/**
 * @brief Verifica que user es igual al nombre de usuario que ejecuta el proceso
 * 
 * @param user usuario
 * @return int 1 si exito, 0 si no
 */
int validate_user(char *user)
{
    return strncmp(user, server_user, MAX_USER_SZ) == 0;
}

/**
 * @brief Tamaño del string de sal de una contraseña
 *        formato: $id$rounds=yyy$salt$encrypted o $id$salt$encrypted
 * @param pass contraseña que sigue el formato especificado
 * @return Tamaño de todo excepto encrypted
 */
size_t get_salt_sz(char *pass)
{
    char *next;

    /* Comprobar si es salt formateada */
    if ( pass[0] != SALT_END_CHAR)
        return MIN_SALT_SZ;
    /* Si no, pasar a siguiente campo */
    next = strchr(&pass[1], SALT_END_CHAR);
    /* Si tiene el campo opcional rounds, pasamos al siguiente campo */
    if ( !memcmp(&next[1], "rounds=", sizeof("rounds=")) )
        next = strchr(&next[1], SALT_END_CHAR);
    /* Buscamos fin del campo salt */
    next = strchr(&next[1], SALT_END_CHAR);

    return ((size_t) (next - pass)) + 1;  
}

/**
 * @brief Establece unos credenciales, con valor por defecto los de quien que ejecuta el programa
 * 
 * @param user Pasar NULL si se quiere usar usuario que ejecuta el programa
 * @param pass Pasar NULL si se quiere usar usuario que ejecuta el programa
 * @return int 1 si exito, 0 si no
 */
int set_credentials(char *user, char *pass)
{
    struct passwd *pw;
    struct spwd *sp;
    char *correct;
    SHA256_CTX sha;

    /* El usuario por defecto es el del que ejecuta el programa */
    if ( !user )
        user = getlogin();
    strcpy(server_user, user);

    /* Version por defecto, coge la contraseña del shadow */
    /* Implementacion obtenida de: https://stackoverflow.com/questions/6536994/authentication-of-local-unix-user-using-c */
    if ( !pass )
    {
        if ( setuid(0) || seteuid(0) ) /* Comprobar que hay permisos */
            return 0;
        pw = getpwnam (user); /* Buscar el usuario proporcionado */
        endpwent(); /* Cerrar el stream del fichero de contraseñas */

        if ( !pw ) 
            return 0; /* Usuario no existe */

        sp = getspnam (pw->pw_name); /* Rellenar la estructura spwd con la informacion adecuada */
        endspent(); /* Siguiente entrada fichero */
        if ( sp )
            correct = sp->sp_pwdp; /* Si donde busca getspnam estaba, usamos esa */
        else
            correct = pw->pw_passwd; /* Si no, usamos la que encuentra getpwnam */

        /* Inevitablemente habra que guardar la sal */
        memcpy(salt, correct, get_salt_sz(correct));
    }
    /* Proveer nosotros una contraseña, solamente la hasheamos a sha_256 */
    else
    {
        is_default_pass = 0;
        correct = pass;
    }

    /* Se guarda una version en sha256 de la contraseña */
    sha256_init(&sha);
    sha256_update(&sha, (BYTE *) correct, strlen(correct));
    sha256_final(&sha, (BYTE *) hashed_pass);

    return 1;
}

/**
 * @brief Comprueba que el usuario y contraseña es correcto. Debe ejecutarse como root
 * 
 * @param pass contraseña, debe estar almacenada en /etc/shadow
 * @return int 1 si autenticacion correcta, 0 si no
 */
int validate_pass(char *pass)
{
    SHA256_CTX sha;
    char hashed[SHA256_BLOCK_SIZE];
    char *shadow_crypted;
    struct crypt_data data;
    data.initialized = 0;

    /* Primero ciframos en formato shadow si era contraseña por defecto */
    if ( is_default_pass )
    {
        shadow_crypted = crypt_r(pass, salt, &data);
        if ( !shadow_crypted ) /* Esto puede ocurrir si no se establecieron credenciales en modo root */
            return 0;
    }
    else
        shadow_crypted = pass;
    
    /* Ahora ciframos lo anterior a sha_256 para compararlo con el sha_256 que se almacena */
    sha256_init(&sha);
    sha256_update(&sha, (BYTE *) shadow_crypted, strlen(shadow_crypted));
    sha256_final(&sha, (BYTE *) hashed);

    /* Comparar los hashes */
    return memcmp(hashed, hashed_pass, SHA256_BLOCK_SIZE) == 0;
}

/**
 * @brief Quita permisos de root
 * Obtenido de: https://stackoverflow.com/questions/3357737/dropping-root-privileges
 * @return int 2 si no es root, 1 si exito, 0 si error
 */
int drop_root_privileges()
{
    gid_t gid;
    uid_t uid;

    /* Si no es root, nada que hacer */
    if ( getuid() != 0 )
        return 2;

    /* Si el programa se ejecuta con sudo, necesitamos obtener sudo_gid a traves de variable de entorno */
    if ((gid = getgid()) == 0) 
    {
        /* Buscamos valor de variable de enterno SUDO_GID */
        const char *sudo_gid = secure_getenv("SUDO_GID");
        if (sudo_gid == NULL) 
            return 0;
        /* Lo convertimos a long long, ya que nos lo pasan como string */
        gid = (gid_t) strtoll(sudo_gid, NULL, 10);
    }
    /* Si no, podemos usar el que nos da getgid() */

    /* Si el programa se ejecuta con sudo, necesitamos obtener sudo_uid a traves de variable de entorno */
    if ((uid = getuid()) == 0) 
    {
        const char *sudo_uid = secure_getenv("SUDO_UID");
        if (sudo_uid == NULL)
            return 0;
        uid = (uid_t) strtoll(sudo_uid, NULL, 10);
    }
    /* Si no, podemos usar el que nos da getuid() */

    /* Procedemos a quitarnos los permisos igualando el gid al obtenido antes */
    if (setgid(gid) != 0)
        return 0;
    if (setuid(uid) != 0)
        return 0;

    /* Comprobar que hemos quitado los privilegios adecuadamente (intentando volvernos root) */
    if (setuid(0) == 0 || seteuid(0) == 0) 
        return 0;

    return 1;
}