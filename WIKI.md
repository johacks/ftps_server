# Memoria Trabajo Tutelado

Joaquín Jiménez López de Castro. Redes 2. Grupo 2312. joaquin.jimenezl@estudiante.uam.es

## 1. Introducción

Para este trabajo tutelado se ha desarrollado un servidor FTPS en C. El objetivo de dicho proyecto era adquirir conocimientos más avanzados de las funciones de C de sistema operativo y adquirir práctica con la implementación y seguridad en protocolos de doble banda (conexión de control y conexión de datos). En esta memoria se detallarán las decisiones de implementación así como los problemas encontrados y sus soluciones. Finalmente se darán unas conclusiones personales sobre la realización del proyecto y se darán unas instrucciones para la instalación y para la prueba.

## 2. Desarrollo Técnico

### 2.1 Estructura y contenido del proyecto

La organización del código y documentación es similar a la de la práctica 1 de REDES II. A continuación se muestra una vista global de los ficheros más relevantes del proyecto:

_/_ : directorio base del proyecto.
+  1.1. _bin/_: almacena el binario/s producido tras la compilación.
	+ 1.1.1. _ftps_server_: binario generado tras la compilación.
+  1.2. _certificates/_: generado por el instalador, directorio por defecto para colocar certificados, puede cambiarse.
	+ 1.2.1. _certificate.pem_: certificado que genera el instalador para el servidor, puede cambiarse
	+ 1.2.2 _private_key.pem_: clave privada que genera el instalador para el servidor, puede cambiarse, pero no hay soporte para  contraseña
+  1.3. _doc/_: almacena documentación generado doxygen.
	+ 1.3.1. _html/_: fichero html
		+ 1.3.1.1. _index.html_: punto de entrada a la documentación doxygen. Puede abrirse con un navegador
+  1.4. _include/_: almacena ficheros de cabecera C (.h).
	+ 1.4.1 _authenticate.h_
	+ 1.4.2 _callbacks.h_
	+ 1.4.3 _config_parser.h_
	+ 1.4.4 _ftp_files.h_
	+ 1.4.5 _ftp_session.h_
	+ 1.4.6 _ftp.h_
	+ 1.4.7 _red.h_
	+ 1.4.8 _utils.h_
+  1.5. _lib/_: almacena ficheros de librería, tanto externas como internas (.a).
	+ 1.5.1 _libconfuse.a_
	+ 1.5.2 _lib_server.a_
	+ 1.5.3 _libtls.a_
	+ 1.5.4 _sha256.a_
+  1.6. _obj/_: almacena el código objeto (.o) para algunos compilados, típicamente los de librería.
+  1.7. _src/_: código fuente de los archivos propios desarrollados (.c).
	+ 1.7.1 _authenticate.c_
	+ 1.7.2 _callbacks.c_
	+ 1.7.3 _config_parser.c_
	+ 1.7.4 _ftp_files.c_
	+ 1.7.5 _ftp_session.c_
	+ 1.7.6 _ftp.c_
	+ 1.7.7 _ftps_server.c_
	+ 1.7.8 _red.c_
	+ 1.7.9 _utils.c_
+  1.8. _srclib/_: código fuente de las librerías externas (.c, .h).
+  1.9. _valgrind/_: almacena ficheros relacionados a valgrind.
	+ 1.9.1.  _supression_file_: fichero de supresión de falsos leaks de memoria en creación de threads con glibc.
+  1.10. _Doxyfile_: fichero de configuración doxygen para generar la documentación.
+  1.11. _install.sh_: script bash para instalar y configurar componentes necesarias para el servidor.
+  1.12. _makefile_: contiene objetivos para compilación, debugging y generación de documentación.
+  1.13. _README.md_: resumen de esta wiki.
+  1.14. _server.conf_: fichero de configuración de parámetros del servidor.
+  1.15. _WIKI.md_: esta wiki.

### 2.2 Librerías utilizadas

- Librerías externas:
	- <u>Librería para sha256 (usado en autenticación)</u>:
		- Código fuente:
			- _srclib/sha256.c_
			- _srclib/sha256.h_
		- Librería generada en compilación:
			- _lib/sha256.a_
		- Obtenido de:
			- https://github.com/B-Con/crypto-algorithms/

	- <u>Librería para parseo de ficheros.conf</u>:
		- Código fuente:
			- _srclib/confuse.h_
		- Librería generada en compilación:
			- _lib/libconfuse.a_: se proporciona ya compilada.
		- Obtenido de:
			- https://github.com/martinh/libconfuse

	- <u>Librería para cifrado TLS 1.2</u>:
		- Código fuente:
			- _srclib/libtomcrypt.c_
			- _srclib/curve25519.c_
			- _srclib/tlse.c_
			- _srclib/tlse.h_
		- Librería generada en compilación:
			- _lib/libtls.a_
		- Obtenido de:
			- https://github.com/eduardsui/tlse

- Librerías internas:
	- _lib_server.a_: resultado de compilación del resto de ficheros fuente

### 2.3 Decisiones de Diseño de tipo de servidor

No se ha prestado especial atención a obtener un servidor FTP robusto que pueda atender a grandísimas cantidades de clientes, como es típico en un servidor HTTP por ejemplo. Sin embargo, tampoco se quería un servidor iterativo, para poder ofrecer la posibildad de tener varias sesiones simultáneas. Finalmente, se ha optado por un servidor concurrente que crea un thread por cada sesión nueva, ya que los threads se crean más rápido que los procesos hijo. Se puede configurar el número máximo de hilos en server.conf, que se controlan con un semáforo en implementación.

### 2.4 Decisiones de Diseño sobre la Funcionalidad a Implementar de FTP

Se han implementado los siguientes comandos FTP:

- ABOR: Abortar transmision de datos.
- CDUP: Moverse a directorio superior.
- CWD: Cambiar de directorio.
- HELP: Mostrar ayuda (lista de comandos implementados).
- MKD: Crear un directorio.
- USER: Indicar nombre de usuario para autenticarse.
- PASS: Recibir contraseña del usuario.
- RNFR: Indicar el nombre de un fichero que se va a renombrar.
- RNTO: Indicar el nombre al que se renombra un fichero seleccionado.
- LIST: Mostrar archivos del directorio actual o del directorio pasado como argumento.
- PASV: Iniciar conexión de datos en modo pasivo. Solicita al servidor un puerto de datos al que conectarse.
- PORT: Iniciar conexión de datos en modo activo. Solicita al servidor que se conecte al puerto indicado.
- DELE: Borrar fichero pasado como argumento.
- PWD: Solicitar directorio actual.
- QUIT: Cerrar sesión FTP.
- RETR: Pedir copia de un fichero remoto al servidor.
- RMD: Borrar un directorio.
- RMDA: Borrar el árbol de un directorio.
- STOR: Enviar un fichero de datos al servidor.
- SIZE: Devolver tamaño de un fichero.
- TYPE: Indicar tipo de transmisión de ficheros: ascii o binario.
- SYST: Indicar sistema operativo del servidor.
- STRU: Indicar la estructura del servidor, solo acepta 'F' (modo fichero).
- MODE: Modo de transmision de datos, solo acepta 'S' (modo stream).
- NOOP: Operacion vacía.
- AUTH: Indica modo de cifrado, solo acepta 'TLS'.
- PBSZ: Indica tamaño de buffer. Solo acepta '0'.
- PROT: Indica nivel de protección de conexión. Solo acepta 'P' (Private).
- FEAT: Indicar caracteristicas adicionales del servidor.

Implementando estas funcionalidades, se satisface [RFC 959](https://tools.ietf.org/html/rfc959) y [RFC 4217](https://tools.ietf.org/html/rfc4217). 

### 2.5 Decisiones de Diseño del flujo de datos

El flujo general de los datos es el siguiente:

1. Se recibe una petición de una nueva conexión de control.
2. Se crea un thread para atender a dicha petición.
3. Por cada comando recibido, se parsea y se identifica su valor de enum asociado y posible argumento.
4. Se usa el valor de enum asociado para indexar un callback de un array de callbacks.
5. Se llama al callback, todos los callbacks siempre reciben: información global del servidor, información de sesión FTP e información del comando recibido
6. Si el callback no hace uso de conexión de datos, ejecuta la funcionalidad y almacena la respuesta en un buffer.
7. Si el callback hace uso de conexión de datos, crea un thread para el callback y entra la conexión de control en un loop especializado que rechaza nuevas peticiones de control por parte del cliente (excepto ABOR). Al final se almacena la respuesta igualmente en un buffer.
8. La conexión de control envía la respuesta del buffer.

De 3. a 8. se repiten hasta que el servidor reciba SIGINT o SIGTERM o el cliente envíe QUIT. De 2. a 8. se repiten hasta que el servidor reciba SIGINT o reciba SIGTERM.

### 2.6 Decisiones de Diseño de gestión de usuarios

El servidor FTP implementado solo soporta un usuario y contraseña por ejecución del mismo. Hay dos formas de especificar las credenciales:

- Credenciales del usuario que ejecuta el programa: si en _server.conf_ no se especifica un nombre de usuario, se utilizarán los credenciales del usuario que ejecuta el programa. Será necesaria la ejecución como root del programa si se quiere utilizar esta versión.

- Especificar credenciales: si en _server.conf_ se especifica un usuario, el programa pedirá al inicio una contraseña para dicho usuario, conformándose unas credenciales únicas para esa ejecución. 

### 2.7 SEGURIDAD

#### 2.7.1 Permisos de root

Es poco deseable que un proceso tenga permisos de root constantemente, sobre todo si espera recibir peticiones de clientespotencialmente malignos, ya que si alguno logra tomar el control del proceso, lo hará con permisos de root, lo que le dará muchasmás facilidades para su ataque (podrá hacer prácticamente lo que quiere con el servidor).

Hay dos situaciones en las que el servidor implementado necesitaría tener permisos de root: si se están usando los credenciales del usuario que ejecuta el programa (lo que implica abrir /etc/shadow que tiene permisos de lectura solo para root) y para la apertura de puertos privilegiados 20 y 21 que usa el protocolo.

Para solucionar lo de /etc/shadow la solución es relativamente simple, se ejecuta el programa al iniciar el servidor con sudo, se lee el hash de la contraseña, se almacena un hash del hash sha256 y se dropean los permisos de root.

Aunque la solución de arriba podría aplicarse para la apertura del puerto 21, que solo se abre una vez, por definición del protocolo, la conexión de datos en modo activo debe abrirse por cada nueva petición de transmisión de datos, lo que requiere permisos de root, y si el usuario ha dropeado los permisos de root, tendría que volver a recuperarlos por cada intento de transmisión de datos en modo activo, y esto no es razonable.

La solución adoptada para el acceso a los puertos es la siguiente: el instalador (_install.sh_), instala el comando _authbind_, y lo configura para que solamente el usuario que ejecuta el instalador pueda acceder a dichos puertos sin permiso de root en los programas que se ejecuten con _authbind_. Es decir, una ejecución como la siguiente por el usuario _monty_:

	$ authbind ls 

Pemitiría que, si se ha configurado _authbind_ para que _monty_ acceda a los puertos 20 y 21, el comando ls en esa ejecución pueda abrir sockets y hacer binds en los puertos 20 y 21.

En este caso, el instalador _install.sh_ instala y configura _authbind_ para que el usuario que ejecuta el script de instalación pueda acceder a los puertos 20 y 21 en los programas que ejecute con _authbind_. Una vez hecho esto, se llama normalmente al programa y el propio binario se llama a sí mismo con authbind, poniendo un argumento especial para no entrar en recursión infinita:

	$ bin/ftps_server --hace un exec()--> authbind bin/ftps_server --using-authbind (no se llama a sí mismo otra vez por el flag)

#### 2.7.2 DDOS

Se hace un leve control de los ataques DDOS al poder controlarse desde el fichero de configuración el número máximo de sesiones simultáneas del servidor.

#### 2.7.3 TLS y port stealing

El servidor obliga a utilizar un certificado y una clave privada, que se pueden generar automáticamente en la instalación. La clave privada no puede estar cifrada con contraseña porque desafortunadamente, la librería TLS utilizada no lo soporta.

Excepto por algunos comandos contados de información general del servidor (HELP, FEAT,...), ningún comando se puede ejecutar en el servidor hasta que el usuario se haya autenticado con USER y PASS; y el servidor implementado, tampoco permite mandar dichos comandos de autenticación hasta que se haya establecido una comunicación cifrada por TLS.

A diferencia de muchos otros servidores FTPS, este servidor también exigirá al cliente un certificado en el _handshake_ de la conexión de control, que no verificará con ningún Certificate Authority, ya que el usuario ya se autentica con sus credenciales con USER y PASS. La razón pues para exigir un certificado al cliente en el _handshake_ de la conexión de control, es que este servidor exige al cliente un cifrado TLS de la conexión de datos también, en la que también se le pide su certificado, de manera que si el certificado que presenta el cliente en la conexión de datos no coincide con el de la conexión de control, se rechaza dicha conexión. Esto no previene el factor de DDOS de que cualquier cliente maligno pueda colarse en la conexión de datos de otro cliente con su clave pública, pero al menos garantiza que dicho cliente maligno no podrá leer los datos recibidos, ya que no dispone de la clave privada del cliente al que suplanta. Faltaría por poner en esta implementación un filtrado por IP, que no sería _fail safe_, pero dificultaría este tipo de ataques.

#### 2.7.4 Inyección de path

Ayudándose la función _realpath_ de POSIX y algunos otros mecanismos, se ha intentado en la medida de lo posible evitar que un cliente pueda acceder a un fichero fuera del root definido para el servidor en _server.conf_ (por ejemplo, poniendo '..' en un path).

## 3.Conclusiones

### 3.1 Conclusiones Personales

La realización de este trabajo ha llevado bastante tiempo. Esto se ha debido a las siguientes dificultades/errores:

- El lenguaje de desarrollo C obliga a tener en cuenta _memory leaks_, estar atento a que cada _string_ tiene su 0 de fin de cadena correspondiente, etc. En el desarrollo de este proyecto se ha cogido mucho contacto con funciones de POSIX y de GNU que han facilitado el desarrollo, por ejemplo, la función alloca, que permite reservar memoria utilizando el stack de una función, con lo que esta se libera automáticamente al salir de la función. La única ventaja/desventaja de trabajar en C ha sido la personalización de la compilación de los callbacks utilizando macros, aunque a veces es difícul distinguir cuándo usarlas y cuándo no.

- Poca comprensión de FTP. Debido al hábito de buscar todas las dudas en _stack overflow_, el desarrollo fue torpe y lleno de dudas durante las primeras semanas, ya que al ser este protocolo viejillo y no tan utilizado como HTTP, no todas las dudas tienen una respuesta inmediata en internet. Finalmente, la opción que parecía que iba a llevar más tiempo era la que hacía perder menos el tiempo: RTFM (read the <span style="color: white;">fucking</span> manual), o dicho de otra manera, leer el RFC.

- FTP es demasiado laxo!. Desafortunadamente para mí, RFC deja muchas cosas en el aire, con lo que se han pasado horas y horas buscando un cliente (_lftp_, ver anexo) que cifrara la conexión de control y de datos y mandando un certificado TLS del cliente en el _handshake_; y eso que la implementación aquí presente es válida según los RFC más relevantes de FTPS (959 y 4217).

### 3.2 Conclusiones Técnicas

La realización de este proyecto ha dado como resultado un servidor FTPS, que cifra la conexión de control y de datos y que permite ejecutar las funcionalidades más importantes de FTP, además de otras extra (como SIZE). El ejecutable no tiene memory leaks reales según valgrind y transmite los ficheros de datos con suficiente velocidad, pese al cifrado exigido. Sin embargo, el cifrado de datos se hace pesado para el comando LIST, que transmite pocos bytes, pero requiere que se haga un _handshake_ por ejecución.

Realizando este proyecto, se llega a la conclusión de que mientras que con los protocolos de doble banda se gana seguridad en el procesamiento de los datos, aumenta enormemente la complejidad del uso de TLS, ya que hay muchas variantes: cliente cifra solo en control, cliente cifra solo en datos, cliente envía certificado, cliente no envía certificado, en vez de certificado se usa identificador de sesión... Por lo que tal vez vale más la pena trabajar o como en HTTPS, o trabajar con un protocolo preparado para TLS desde el principio, no como FTP, al que se le añadió TLS en un RFC tras varios años de su diseño. 

## 4. Anexo: Instrucciones de Uso del Servidor

### 4.1 Instalación

Adjunto a este proyecto se encuentra un instalador, _install.sh_ que realiza las siguientes operaciones:

- Instalar _authbind_ si no está instalado todavía. Este programa es obligatorio para el funcionamiento del servidor.
- Configurar _authbind_ para que el usuario que ejecuta el instalador pueda acceder a los puertos 20 y 21 en la ejecución del binario.
- Instalar _doxygen_ y _xdot_ opcionalmente para generar la documentación _doxygen_ del proyecto.
- Compilar el binario. Se guardará en _bin/ftps_server_
- Generar certificados del servidor. Se permitirán crear en ese momento usando _openssl_ el certificado X.509 y clave privada correspondiente del servidor. Se guardarán en _certificates/_. Se pueden utilizar certificados ya creados, pero la clave privada no puede estar protegida con una contraseña.
- Opcionalmente, el instalador permitirá instalar un cliente ftps (_lftp_) y configurarlo para que sea compatible con el servidor. Esto incluirá crear certificado y clave privada del cliente. Las configuraciones resultantes se almacenarán en /etc/lftp.conf, añadiéndose al final del fichero, y podrán modificarse más adelante si se desea. 

### 4.2 Targets Makefile

En el proyecto se encuentra un fichero de _Makefile_ que permite ejecutar las siguientes funcionalidades: 

	~$ make 				# Genera todos los ejecutables
	~$ make clean			# Elimina todos los ejcutables, librerías generables dinámicamente y documentación
	~$ make clear			# Igual que clean pero no borra ejecutables
	~$ make doc 			# Genera la documentación en formato doxygen en el doc a partir del fichero Doxyfile proporcionado
	~$ make runv			# Correr con valgrind, solo debug. Solo sirve si se ha definido la macro DEBUG en compilación.
	~$ make runv_authbind	# Correr el programa usando valgrind. Funciona en compilación normal.

### 4.3 Configuración del Servidor

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

### 4.4 Ejecución del Servidor y Test con lftp

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