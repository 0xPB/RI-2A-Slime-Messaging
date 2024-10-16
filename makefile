# Makefile for Slime project

# Compiler
CC = gcc

# Source files
CLIENT_SRC = client.c
SERVER_SRC = server.c

# Output binaries
CLIENT_BIN = client.exe
SERVER_BIN = server.exe

# Libraries
LIBS_SERVER = -lsqlite3

# Compile the client
client: $(CLIENT_SRC)
	$(CC) $(CLIENT_SRC) -o $(CLIENT_BIN)

# Compile the server with SQLite support and ensure server directory exists
server: server_dir $(SERVER_SRC)
	$(CC) $(SERVER_SRC) -o $(SERVER_BIN) $(LIBS_SERVER)

# Rule to create the server directory if it doesn't exist
server_dir:
	mkdir -p server

# Compile both client and server
all: server_dir client server docs

# Generate Doxygen documentation
docs:
	doxygen Doxyfile

# Clean up generated files
clean:
	rm -f $(CLIENT_BIN) $(SERVER_BIN)
	rm -rf docs
	rm -rf server

# Phony targets
.PHONY: client server clean all docs server_dir

