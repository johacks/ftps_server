# FTPS server

FTP server with secure TLS connection written in C.

## Server use instructions

### Facility

Attached to this project is an installer, _install.sh_ that performs the following operations:

- Install _authbind_ if it is not installed yet. This program is mandatory for server operation.
- Configure _authbind_ so that the user who runs the installer can access ports 20 and 21 in the execution of the binary.
- Install _doxygen_ and _xDot_ optionally to generate the documentation _Doxygen_ of the project.
- Compile the binary. It will be saved in _bin/ftps_server_
- Generate server certificates. It will be allowed to create at that time using _OpenSSL_ the certificate X.509 and corresponding private password. They will be saved in _certificates/_. Certificates already created can be used, but the private key cannot be protected with a password.
- Optionally, the installer will allow installing an FTPS client (_lftp_) and configure it to be compatible with the server. This will include creating client certificate and key. The resulting configurations will be stored in /etc/lftp.conf, adding at the end of the file, and can be modified later if desired.

### Makefile targets

In the project is a _makefile_ file that allows you to execute the following functionalities:

	~$ make 			    # Generates all executables
	~$ make clean			# Eliminates all executable, dynamically generetable libraries and documentation
	~$ make clear			# Just like clean but do not delete executable
	~$ make doc 			# Generates the documentation in doxygen format
	~$ make runv			# Run with Valgrind, only debug. It only serves if the debug macro has been defined.
	~$ make runv_authbind	# Run the program using valgrind. It works in normal compilation.

### Configuración del Servidor

In the _server.conf_ file are the different server start configuration options.These fields are shown below:

- server_root: Directory where public server files are stored.

- max_passive_ports: Maximum number of ports that can be opened in passive mode.

- max_sessions: Maximum number of concurrent FTP sessions.

- ftp_user: FTP user, leave empty if you want to use the user credentials that launches the server (necessary sudo)

- ftp_host: IP direction of deployment or name of the deployment host

- default_type: Type of default data transmission, possible values 'ASCII' and 'Binary'

- certificate_path: Path to the file with a private key

- private_key_path: Path to the certificate file x.509

- daemon_mode: Indicate if you want to run in demon mode.0 Yes false, any other number if true

### Server execution and test with lftp

At the end of the installation you can already run the program normally with:

	$ bin/ftps_server

Unless in _Server.conf_ a username has not been indicated, in which case it is:

	$ sudo bin/ftps_server

In order to read the /etc /shadow file and access the user's password hash running the program.Once collected, root permissions will be removed.

There are few customers compatible with the authentication architecture of this server, which is why the installer offers the installation of a FTPS (_lftp_) customer compatible with this server and the configuration of the same to encrypt the control and data connection.

Already installed and configured the customer and launched the server, a possible example of use is as follows:

	$ lftp johacks@localhost // It will connect to the user in localhost with username Johacks
	Password: 
	lftp johacks@localhost:~> debug                            
	lftp johacks@localhost:~> source /etc/lftp.conf // Ensure that the configuration file is used
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
