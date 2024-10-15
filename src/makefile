# Makefile for Slime project

# Compiler and flags
CC = gcc
CFLAGS = -lpthread

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
	$(CC) $(CFLAGS) $(CLIENT_SRC) -o $(CLIENT_BIN)

# Compile the server with SQLite support
server: $(SERVER_SRC)
	$(CC) $(CFLAGS) $(SERVER_SRC) -o $(SERVER_BIN) $(LIBS_SERVER)

# Rule to create the server directory if it doesn't exist
server_dir:
	mkdir -p server

# Compile both client and server
all: client server

# Clean up generated files
clean:
	rm -f $(CLIENT_BIN) $(SERVER_BIN)

# Phony targets
.PHONY: client server clean all
