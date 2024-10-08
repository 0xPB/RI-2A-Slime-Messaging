# Compilateur
CC = gcc

# Options de compilation
CFLAGS = -Wall -Wextra -g

# Drapeaux de l'édition des liens pour le client et le serveur
LDFLAGS_CLIENT = -lpthread
LDFLAGS_SERVER = -lsqlite3 -lpthread

# Cibles
CLIENT_EXEC = client
SERVER_EXEC = server

# Fichiers sources
CLIENT_SRC = client.c
SERVER_SRC = server.c

# Règle par défaut : compile client et server
all: $(CLIENT_EXEC) $(SERVER_EXEC)

# Compilation du client
$(CLIENT_EXEC): $(CLIENT_SRC)
	$(CC) $(CLIENT_SRC) -o $(CLIENT_EXEC) $(LDFLAGS_CLIENT) $(CFLAGS)

# Compilation du serveur
$(SERVER_EXEC): $(SERVER_SRC)
	$(CC) $(SERVER_SRC) -o $(SERVER_EXEC) $(LDFLAGS_SERVER) $(CFLAGS)

# Nettoyage des fichiers exécutables
clean:
	rm -f $(CLIENT_EXEC) $(SERVER_EXEC)

# Nettoyage complet, incluant les fichiers temporaires de l'éditeur
distclean: clean
	rm -f *~ *.bak
