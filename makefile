#########################################################
# VARIABLES
# Directories
O=obj/
L=lib/
D=doc/
S=src/
H=include/
SL=srclib/
B=bin/
V=valgrind/
DIRECTORIES=$(O) $(L) $(D) $(S) $(H) $(SL) $(B)

# Compilation and execution with valgrind
CC=gcc
VGLOGS=vglogs/
VFLAGS= --leak-check=full --show-leak-kinds=all --trace-children=yes --track-origins=yes --suppressions=$(V)suppression_file --trace-children-skip=*authbind/helper,*ls*,/bin/sh*
CFLAGS= -g -Wall -I $(L) -I $(H) -I $(SL)
EXE=$(B)ftps_server

# Bookstores
# external
PRS_LIB=$(L)libconfuse.a
SHA_LIB_O=$(O)sha256.o
SHA_LIB=$(L)sha256.a
TLS_LIB_O=$(O)tlse.o $(O)curve25519.o
TLS_LIB=$(L)libtls.a
EXT_LIB=$(PRS_LIB) $(SHA_LIB) $(TLS_LIB)

# internal
INT_LIB_O=$(O)network.o $(O)authenticate.o $(O)utils.o $(O)config_parser.o $(O)ftp.o $(O)callbacks.o $(O)ftp_session.o $(O)ftp_files.o
INT_LIB=$(L)lib_server.a

# Use of libraries
LNK_LIB=-pthread -lcrypt -lrt -L./lib
LIB=$(INT_LIB) $(LNK_LIB) $(EXT_LIB)

########################################################

all: directories $(INT_LIB) $(EXT_LIB) $(EXE)

# INTERNAL LIBRARY

$(INT_LIB): $(INT_LIB_O)
	ar rcs $@ $^
	rm -rf $(INT_LIB_O)

$(O)network.o: $(S)network.c $(H)network.h
	$(CC) $(CFLAGS) -c $< -o $@ $(LNK_LIB)

$(O)authenticate.o: $(S)authenticate.c $(H)authenticate.h
	$(CC) $(CFLAGS) -c $< -o $@ $(LNK_LIB)

$(O)utils.o: $(S)utils.c $(H)utils.h
	$(CC) $(CFLAGS) -c $< -o $@ $(LNK_LIB)

$(O)config_parser.o: $(S)config_parser.c $(H)config_parser.h
	$(CC) $(CFLAGS) -c $< -o $@ $(LNK_LIB)

$(O)ftp.o: $(S)ftp.c $(H)ftp.h
	$(CC) $(CFLAGS) -c $< -o $@ $(LNK_LIB)

$(O)callbacks.o: $(S)callbacks.c $(H)callbacks.h
	$(CC) $(CFLAGS) -c $< -o $@ $(LNK_LIB)

$(O)ftp_session.o: $(S)ftp_session.c $(H)ftp_session.h
	$(CC) $(CFLAGS) -c $< -o $@ $(LNK_LIB)

$(O)ftp_files.o: $(S)ftp_files.c $(H)ftp_files.h
	$(CC) $(CFLAGS) -c $< -o $@ $(LNK_LIB)


# EXTERNAL LIBRARY
# Sha bookcase

$(SHA_LIB): $(SHA_LIB_O)
	ar rcs $@ $^

$(O)sha256.o: $(SL)sha256.c
	$(CC) $(CFLAGS) -c $< -o $@

# TLS library

$(TLS_LIB): $(TLS_LIB_O)
	ar rcs $@ $^

$(O)curve25519.o: $(SL)curve25519.c
	$(CC) $(CFLAGS) -c $< -o $@

$(O)tlse.o: $(SL)tlse.c
	$(CC) $(CFLAGS) -c $< -o $@

# libconfuse library always precompiled, no target
# BINARIES
# Main program, needs access to ports

$(B)ftps_server: $(S)ftps_server.c $(INT_LIB) $(EXT_LIB)
	$(CC) $(CFLAGS) $< -o $@ $(LIB)

# UTILITY TARGETS

directories:
	mkdir -p $(DIRECTORIES)

doc: 
	doxygen Doxyfile

clean:
	rm -rf $(EXE) $(INT_LIB) $(VGLOGS)* $(D)

clear:
	rm -rf $(INT_LIB) $(VGLOGS)* $(D)

runv:
	valgrind $(VFLAGS) $(B)ftps_server

runv_authbind:
	authbind --deep valgrind $(VFLAGS) $(B)ftps_server --using-authbind