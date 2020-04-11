#!/bin/bash

authbind_path=$(command -v authbind)
if [ -z $authbind_path ]; then
    echo "Este programa requiere que authbind este instalado, quieres instalarlo? Escribe 1 o 2:"
    select respuesta in "Si" "No"; do
        case $respuesta in
            Si ) sudo apt-get install authbind break;;
            No ) exit;;
        esac
    done
fi

# Indicar a authbind que permita que los puertos 20 y 21 se abran por el usuario actual
echo "Se otorgaran permisos de lectura y escritura en los puertos 20 y 21 al usuario '${USER}'
en las ejecuciones con authbind, quieres continuar? Escribe 1 o 2:"
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
echo "Compilando el binario..."
make
echo "Se ha terminado la instalacion, el binario se encuentra en /bin, debera ejecutarlo como '${USER}'"