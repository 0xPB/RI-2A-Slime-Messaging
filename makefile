# Makefile for Slime project

# Compiler
CC = gcc

# Source files
CLIENT_SRC = src/client.c
SERVER_SRC = src/server.c

# Output binaries (in src folder)
CLIENT_BIN = src/client.exe
SERVER_BIN = src/server.exe

# Libraries
LIBS_SERVER = -lsqlite3

# Rule to create the server directory in the src folder if it doesn't exist
server_dir:
	mkdir -p src/server

# Compile the client
client: server_dir $(CLIENT_SRC)
	$(CC) $(CLIENT_SRC) -o $(CLIENT_BIN)

# Compile the server with SQLite support
server: server_dir $(SERVER_SRC)
	$(CC) $(SERVER_SRC) -o $(SERVER_BIN) $(LIBS_SERVER)

# Rule to generate Doxygen documentation (force execution)
docs:
	doxygen Doxyfile

# Compile both client and server
all: client server docs

# Clean up generated files
clean:
	rm -f $(CLIENT_BIN) $(SERVER_BIN)
	rm -rf docs
	rm -rf src/server

# Phony targets
.PHONY: client server clean all server_dir docs
