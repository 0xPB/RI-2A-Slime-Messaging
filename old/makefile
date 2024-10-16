# Makefile for Slime project

# Compiler and flags
CC = gcc
CFLAGS = -lpthread

# Source files
CLIENT_SRC = src/client.c
SERVER_SRC = src/server.c

# Output binaries
CLIENT_BIN = client.exe
SERVER_BIN = server.exe

# Libraries
LIBS_SERVER = -lsqlite3

# Doxygen
DOXYGEN = doxygen
DOXYFILE = Doxyfile

# Compile the client
client: $(CLIENT_SRC)
	$(CC) $(CFLAGS) $(CLIENT_SRC) -o $(CLIENT_BIN)

# Compile the server with SQLite support
server: $(SERVER_SRC)
	$(CC) $(CFLAGS) $(SERVER_SRC) -o $(SERVER_BIN) $(LIBS_SERVER)

# Rule to create the server directory in the src folder if it doesn't exist
server_dir:
	mkdir -p src/server

# Rule to generate Doxygen documentation
docs:
	$(DOXYGEN) $(DOXYFILE)

# Compile both client and server, and generate documentation
all: client server docs

# Clean up generated files
clean:
	rm -f $(CLIENT_BIN) $(SERVER_BIN)
	rm -rf docs  # Remove the docs directory created by Doxygen

# Phony targets
.PHONY: client server clean all docs server_dir

