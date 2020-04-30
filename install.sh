#!/bin/bash

# Instalación de authbind
authbind_path=$(command -v authbind)
if [ -z $authbind_path ]; then
    echo "Este programa requiere que authbind este instalado, quieres instalarlo? Escribe 1 o 2:"
    select respuesta in "Si" "No"; do
        case $respuesta in
            Si ) sudo apt-get install authbind; break;;
            No ) exit;;
        esac
    done
fi

# Instalación de doxygen
doxygen_path=$(command -v doxygen)
xdot_path=$(command -v xdot)
if [ -z $doxygen_path ] || [ -z $xdot_path ]; then
    echo "Este programa requiere que doxygen y xdot esten instalados para generar documentación, quieres instalarlos? Escribe 1 o 2:"
    select respuesta in "Si" "No"; do
        case $respuesta in
            Si ) sudo apt-get install doxygen; sudo apt-get install xdot ;break;;
            No ) break;;
        esac
    done
fi

# Indicar a authbind que permita que los puertos 20 y 21 se abran por el usuario actual
echo "Se otorgarán permisos de lectura y escritura en los puertos 20 y 21 al usuario '${USER}' en las
ejecuciones con authbind para no tener que tener permisos permanentes de root en el acceso a puertos,
quieres continuar? Escribe 1 o 2:"
select respuesta in "Si" "No"; do
    case $respuesta in
        Si ) break;;
        No ) exit;;
    esac
done

puerto20="/etc/authbind/byport/20"
puerto21="/etc/authbind/byport/21"

sudo touch ${puerto20}
sudo chown ${USER}:${USER} ${puerto20}
sudo chmod 755 ${puerto20}

sudo touch ${puerto21}
sudo chown ${USER}:${USER} ${puerto21}
sudo chmod 755 ${puerto21}

echo "Ahora el usuario '${USER}' podra correr el servidor FTP correctamente"

# Compilar programa
echo "Compilando el binario..."
make
echo "Se ha terminado la compilación, el binario se encuentra en /bin, debera ejecutarlo como '${USER}'"

# Generar certificados del servidor
echo "El servidor necesita un certificado X.509 y su clave privada correspondiente .pem, que no puede llevar contraseña, quieres crearlos ahora?
Escribe 1 o 2:"
select respuesta in "Si" "No"; do
    case $respuesta in
        Si ) respuesta="1"; break;;
        No ) respuesta=""; break;;
    esac
done
ca_path=''
if [ $respuesta ]; then
    openssl_path=$(command -v openssl)
    if [ -z $openssl_path ]; then
        echo "Es necesario tener openssl para generar los certificados, quieres instalarlo? Escribe 1 o 2:"
        select respuesta in "Si" "No"; do
            case $respuesta in
                Si ) sudo apt-get install openssl ;break;;
                No ) break;;
            esac
        done
    fi
    mkdir -p certificates/
    cd certificates
    openssl req -newkey rsa:2048 -nodes -keyout private_key.pem -x509 -days 365 -out certificate.pem
    sudo chmod 760 certificate.pem private_key.pem 
    ca_path=$(realpath certificate.pem)
    cd ..
fi

# Instalación de un cliente compatible 
lftp_path=$(command -v lftp)
if [ -z $lftp_path ]; then
    echo "Debido a las altas restricciones de autenticación en la conexión de datos de este servidor, se recomienda usar 
          el cliente lftp, que permite cifrar conexión de datos además de en la conexión de control, quieres instalarlo? Escribe 1 o 2:"
    select respuesta in "Si" "No"; do
        case $respuesta in
            Si ) sudo apt-get install lftp; break;;
            No ) exit;;
        esac
    done
fi

# Configuración del cliente
lftp_path=$(command -v lftp)
if [ $lftp_path ]; then
    echo "Quieres configurar lftp para el correcto funcionamiento del servidor? Escribe 1 o 2:"
    select respuesta in "Si" "No"; do
        case $respuesta in
            Si ) break;;
            No ) exit;;
        esac
    done
fi

if [ -z $ca_path ]; then
    echo "No has llegado a crear un certificado para el servidor, introduce el path al fichero .pem si ya lo has creado de antemano: "
    read ca_path
    ca_path=$(realpath $ca_path)
fi

# Generar certificados del cliente
echo "El cliente necesita un certificado X.509 y su clave privada correspondiente .pem, en que directorio quieres guardarlos? 
Introduce el path al directorio: "
curr_path=$(pwd)
read certificate_path
certificate_path=$(realpath $certificate_path)
mkdir -p $certificate_path
cd $certificate_path
openssl req -newkey rsa:2048 -nodes -keyout private_key.pem -x509 -days 365 -out certificate.pem
sudo chmod 760 certificate.pem private_key.pem 
if [ "$?" -ne "0" ]; then
    echo "Fallo al generar certificados"
    exit
fi 
cd $curr_path
p_key_path="${certificate_path}/private_key.pem"
cert_path="${certificate_path}/certificate.pem"

# Terminar de configurar el cliente
client_config="\nset ftp:ssl-allow on\nset ftp:ssl-auth \"TLS\"\nset ftp:ssl-data-use-keys on\nset ftp:ssl-force on\nset ftp:ssl-protect-data on\nset ftp:ssl-use-ccc on\nset ftp:use-size on\nset ftp:use-stat off\nset ftp:initial-prot \"P\"\nset ssl:check-hostname off\nset ssl:key-file ${p_key_path}\nset ssl:cert-file ${cert_path}\nset ssl:ca-file ${ca_path}\n"

echo "Se usará la siguiente configuración del cliente, puede modificarla en /etc/lftp.conf:"
echo -e $client_config | sudo tee -a /etc/lftp.conf