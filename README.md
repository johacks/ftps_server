# Servidor FTPS

Joaquín Jiménez López de Castro. Redes 2. Grupo 2312. joaquin.jimenezl@estudiante.uam.es

Versión extendida del README.md: WIKI.md (adjunto)

## Instrucciones de Uso del Servidor

### Instalación

Adjunto a este proyecto se encuentra un instalador, _install.sh_ que realiza las siguientes operaciones:

- Instalar _authbind_ si no está instalado todavía. Este programa es obligatorio para el funcionamiento del servidor.
- Configurar _authbind_ para que el usuario que ejecuta el instalador pueda acceder a los puertos 20 y 21 en la ejecución del binario.
- Instalar _doxygen_ y _xdot_ opcionalmente para generar la documentación _doxygen_ del proyecto.
- Compilar el binario. Se guardará en _bin/ftps_server_
- Generar certificados del servidor. Se permitirán crear en ese momento usando _openssl_ el certificado X.509 y clave privada correspondiente del servidor. Se guardarán en _certificates/_. Se pueden utilizar certificados ya creados, pero la clave privada no puede estar protegida con una contraseña.
- Opcionalmente, el instalador permitirá instalar un cliente ftps (_lftp_) y configurarlo para que sea compatible con el servidor. Esto incluirá crear certificado y clave privada del cliente. Las configuraciones resultantes se almacenarán en /etc/lftp.conf, añadiéndose al final del fichero, y podrán modificarse más adelante si se desea. 

### Targets Makefile

En el proyecto se encuentra un fichero de _Makefile_ que permite ejecutar las siguientes funcionalidades: 

	~$ make 			    # Genera todos los ejecutables
	~$ make clean			# Elimina todos los ejcutables, librerías generables dinámicamente y documentación
	~$ make clear			# Igual que clean pero no borra ejecutables
	~$ make doc 			# Genera la documentación en formato doxygen en el doc a partir del fichero Doxyfile proporcionado
	~$ make runv			# Correr con valgrind, solo debug. Solo sirve si se ha definido la macro DEBUG en compilación.
	~$ make runv_authbind	# Correr el programa usando valgrind. Funciona en compilación normal.

### Configuración del Servidor

En el fichero _server.conf_ se encuentran las distintas opciones de configuración de inicio del servidor. A continuación se muestran dichos campos:

- server_root: Directorio donde se almacenan los ficheros públicos del servidor.

- max_passive_ports: Número maximo de puertos que se pueden abrir en modo pasivo.

- max_sessions: Número maximo de sesiones FTP concurrentes.

- ftp_user: Usuario FTP, dejar vacio si se quieren usar los credenciales del usuario que lanza el servidor (necesario sudo) 

- ftp_host: Direccion IP de despliegue o nombre del host de despliegue

- default_type: Tipo de transmision de datos por defecto, valores posibles 'ascii' y 'binary' 

- certificate_path: Path al fichero con clave privada

- private_key_path: Path al fichero de certificado x.509

- daemon_mode: Indica si se quiere ejecutar en modo demonio. 0 si falso, cualquier otro numero si verdadero

### Ejecución del Servidor y Test con lftp

Al finalizar la instalación (ver 4.1) ya se puede correr el programa normalmente con:

	$ bin/ftps_server

A no ser que en _server.conf_ no se haya indicado un nombre de usuario, en cuyo caso es:

	$ sudo bin/ftps_server

Para así poder leer el fichero /etc/shadow y acceder al hash de la contraseña del usuario que ejecuta el programa. Una vez recogido, se droperan los permisos de root.

Hay pocos clientes compatibles con la arquitectura de autenticación de este servidor, es por ello que el instalador ofrece la instalación de un cliente ftps (_lftp_) compatible con este servidor y la configuración del mismo para que cifre la conexión de control y de datos.

Ya instalado y configurado el cliente y lanzado el servidor, un posible ejemplo de uso es el siguiente:

	$ lftp johacks@localhost // Se conectará al servidor desplegado en localhost con nombre de usuario johacks
	Clave: 
	lftp johacks@localhost:~> debug                            
	lftp johacks@localhost:~> ls
	---- Conectándose a localhost (127.0.0.1) port 21
	<--- 220 Bienvenido a mi servidor FTP
	---> FEAT
	<--- 211-Features adicionales:  
	<---  PASV
	<---  SIZE
	<---  AUTH TLS
	<---  PROT
	<---  PBSZ
	<--- 211 End
	---> AUTH TLS
	<--- 234 Empezar negociacion TLS
	---> USER johacks
	Certificate: C=ES,ST=Spain,L=Madrid,O=UAM,OU=EPS,CN=SERVIDORFTP,EMAIL=joaquin.jimenezl@estudiante.uam.es
	Issued by: C=ES,ST=Spain,L=Madrid,O=UAM,OU=EPS,CN=SERVIDORFTP,EMAIL=joaquin.jimenezl@estudiante.uam.es
	Trusted
	WARNING: Certificate verification: hostname checking disabled
	<--- 331 Introduzca el password
	---> PASS XXXX
	<--- 230 Autenticacion correcta
	---> PWD
	<--- 257 /
	---> PBSZ 0
	<--- 200 Operacion correcta
	---> PROT P
	<--- 200 Operacion correcta
	---> CCC
	<--- 502 Comando no implementado
	---> PASV
	<--- 227 Entering Passive Mode (127,0,0,1,146,181)
	---- Conectando socket de datos a (127.0.0.1) puerto 37557
	---- Conexión de datos establecida
	---> LIST
	Certificate: C=ES,ST=Spain,L=Madrid,O=UAM,OU=EPS,CN=SERVIDORFTP,EMAIL=joaquin.jimenezl@estudiante.uam.es
	Issued by: C=ES,ST=Spain,L=Madrid,O=UAM,OU=EPS,CN=SERVIDORFTP,EMAIL=joaquin.jimenezl@estudiante.uam.es
	Trusted
	WARNING: Certificate verification: hostname checking disabled
	<--- 150 Enviando listado de directorio
	total 56                          
	drwxr-xr-x 2 1000 1000 4096 04-30 13:32 certificados_ftp
	drwxr-xr-x 2 1000 1000 4096 04-26 11:20 Ctests
	drwxr-xr-x 2 1000 1000 4096 04-30 17:43 Descargas
	drwxr-xr-x 2 1000 1000 4096 04-24 01:23 Documentos
	drwxr-xr-x 2 1000 1000 4096 04-26 11:30 Escritorio
	drwxr-xr-x 2 1000 1000 4096 04-26 12:26 Imágenes
	drwxr-xr-x 2 1000 1000 4096 04-24 01:23 Música
	drwxr-xr-x 2 1000 1000 4096 04-24 01:23 Plantillas
	-rw------- 1 1000 1000 1704 04-30 12:28 private_key.pem
	drwxr-xr-x 2 1000 1000 4096 04-24 01:23 Público
	drwxr-xr-x 5 1000 1000 4096 04-30 18:53 REDES2
	drwxr-xr-x 4 1000 1000 4096 04-29 16:05 snap
	drwxrwxr-x 2 1000 1000 4096 04-30 18:17 test
	drwxr-xr-x 2 1000 1000 4096 04-24 01:23 Vídeos
	---- Got EOF on data connection
	---- Cerrando socket de datos
	<--- 226 Transferencia de datos terminada: 733 Bytes
	---- Cerrando conexión ociosa
	---> QUIT
	<--- 221 Hasta la vista
	---- Cerrando socket de control
	lftp johacks@localhost:~> quote HELP
	---- Conectándose a localhost (127.0.0.1) port 21
	<--- 220 Bienvenido a mi servidor FTP
	---> FEAT
	<--- 211-Features adicionales:    
	<---  PASV
	<---  SIZE
	<---  AUTH TLS
	<---  PROT
	<---  PBSZ
	<--- 211 End
	---> AUTH TLS
	<--- 234 Empezar negociacion TLS
	---> USER johacks
	Certificate: C=ES,ST=Spain,L=Madrid,O=UAM,OU=EPS,CN=SERVIDORFTP,EMAIL=joaquin.jimenezl@estudiante.uam.es
	Issued by: C=ES,ST=Spain,L=Madrid,O=UAM,OU=EPS,CN=SERVIDORFTP,EMAIL=joaquin.jimenezl@estudiante.uam.es
	Trusted
	WARNING: Certificate verification: hostname checking disabled
	<--- 331 Introduzca el password
	---> PASS XXXX
	<--- 230 Autenticacion correcta
	---> PWD
	<--- 257 /
	---> PBSZ 0
	<--- 200 Operacion correcta
	---> PROT P
	<--- 200 Operacion correcta
	---> CCC
	<--- 502 Comando no implementado
	---> TYPE I
	<--- 200 Operacion correcta
	---> HELP
	214 Lista de comandos implementados: ABOR,CDUP,CWD,HELP,MKD,PASS,RNTO,LIST,PASV,DELE,PORT,PWD,QUIT,RETR,RMD,RMDA,STOR,RNFR,SIZE,TYPE,USER,SYST,STRU,MODE,NOOP,AUTH,PBSZ,PROT,FEAT
	lftp johacks@localhost:~> quote PWD
	---> PWD
	257 /
	lftp johacks@localhost:~> cd REDES2
	---- CWD path to be sent is `~/REDES2'
	---> CWD REDES2
	<--- 250 Cambiado al directorio /REDES2
	cd ok, dir actual=~/REDES2
	lftp johacks@localhost:~/REDES2> quote PWD
	---> PWD
	257 /REDES2
	lftp johacks@localhost:~/REDES2> ls
	---> TYPE A
	<--- 200 Operacion correcta
	---> PASV
	<--- 227 Entering Passive Mode (127,0,0,1,170,23)
	---- Conectando socket de datos a (127.0.0.1) puerto 43543
	---- Conexión de datos establecida
	---> LIST
	Certificate: C=ES,ST=Spain,L=Madrid,O=UAM,OU=EPS,CN=SERVIDORFTP,EMAIL=joaquin.jimenezl@estudiante.uam.es
	Issued by: C=ES,ST=Spain,L=Madrid,O=UAM,OU=EPS,CN=SERVIDORFTP,EMAIL=joaquin.jimenezl@estudiante.uam.es
	Trusted
	WARNING: Certificate verification: hostname checking disabled
	<--- 150 Enviando listado de directorio
	---- Got EOF on data connection
	---- Cerrando socket de datos
	total 236
	drwxrwxr-x 13 1000 1000   4096 04-30 19:42 ftps_server
	drwxr-xr-x  8 1000 1000   4096 04-29 23:37 practica3
	-rw-------  1 1000 1000 227891 04-26 12:03 PrC!ctica 3 - Multimedia.pdf
	drwxr-xr-x  5 1000 1000   4096 04-27 19:30 Tema 3 - Multimedia
	<--- 226 Transferencia de datos terminada: 287 Bytes
	lftp johacks@localhost:~/REDES2> quote SIZE practica3
	---> TYPE I
	<--- 200 Operacion correcta
	---> SIZE practica3
	213 Tamaño de archivo: 4096 Bytes
	lftp johacks@localhost:~/REDES2> mkdir directorio
	---> MKD directorio
	<--- 257 /REDES2/directorio creado
	mkdir ok, `directorio' creado
	lftp johacks@localhost:~/REDES2> quit
	---> QUIT
	<--- 221 Hasta la vista
	---- Cerrando socket de control
	$ 