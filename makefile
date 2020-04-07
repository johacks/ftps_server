########################################################
# VARIABLES

# Directorios
O=obj/
L=lib/
D=doc/
S=src/
H=include/
SL=srclib/
DIRECTORIES=$(O) $(L) $(D) $(S) $(H) $(SL)

# Compilacion y ejecucion con valgrind
CC=gcc
VGLOGS=vglogs/
VFLAGS= --leak-check=full --show-leak-kinds=all --log-file=$(VGLOGS)log%p.txt --trace-children=yes --track-origins=yes
CFLAGS= -g -Wall -I $(L) -I $(H) -I $(SL)
EXE=ftps_server

# Librerias

# Externas
PRS_LIB=$(L)libconfuse.a
SHA_LIB_O=$(O)sha256.o
SHA_LIB=$(L)sha256.a
EXT_LIB=$(PRS_LIB) $(SHA_LIB)

# Internas
INT_LIB_O=$(O)red.o $(O)authenticate.o $(O)utils.o
INT_LIB=$(L)lib_server.a

# Uso de librerias
LNK_LIB=-pthread -lrt -L./lib -Lconfuse
LIB=$(INT_LIB) $(LNK_LIB) $(EXT_LIB)

########################################################

all: directories $(INT_LIB) $(EXT_LIB) $(EXE)

# LIBRERIA INTERNA

$(INT_LIB): $(INT_LIB_O)
	ar rcs $@ $^
	rm -rf $(INT_LIB_O)

$(O)red.o: $(S)red.c $(H)red.h
	$(CC) $(CFLAGS) -c $< -o $@ $(LNK_LIB)

$(O)authenticate.o: $(S)authenticate.c $(H)authenticate.h
	$(CC) $(CFLAGS) -c $< -o $@ $(LNK_LIB)

$(O)utils.o: $(S)utils.c $(H)utils.h
	$(CC) $(CFLAGS) -c $< -o $@ $(LNK_LIB)

# LIBRERIA EXTERNA

# Libreria sha

$(SHA_LIB): $(SHA_LIB_O)
	ar rcs $@ $^
	rm -rf $(SHA_LIB_O)

$(O)sha256.o: $(SL)sha256.c
	$(CC) $(CFLAGS) -c $< -o $@

# Libreria libconfuse siempre precompilada, sin target

# BINARIOS

# Programa principal

ftps_server: $(S)ftps_server.c $(INT_LIB) $(EXT_LIB)
	$(CC) $(CFLAGS) $< -o $@ $(LIB)

# TARGETS DE UTILIDAD

directories:
	mkdir -p $(DIRECTORIES)

doc: Doxyfile
	doxygen Doxyfile

clean:
	rm -rf $(EXE) $(INT_LIB) $(VGLOGS)* $(D)

clear:
	rm -rf $(INT_LIB) $(VGLOGS)* $(D)

# Concatena todos los logs de valgrind en uno
pretty_vglog:
	cat $(VGLOGS)* > $(VGLOGS)execlog.txt

runv:
	valgrind $(VFLAGS) ftps_server
