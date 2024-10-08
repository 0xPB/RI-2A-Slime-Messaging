#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <sqlite3.h>

#define BUFFER_SIZE 1024
#define MAX_CLIENTS 10

typedef struct
{
    int socket;
    char username[50];
    char current_channel[50]; // salon actuel
} client_t;

client_t *clients[MAX_CLIENTS];
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;

void clean_input(char *str)
{
    char *pos;
    if ((pos = strchr(str, '\n')) != NULL)
    {
        *pos = '\0'; // Retirer le retour à la ligne
    }
}

int authenticate_user(const char *username, const char *password)
{
    sqlite3 *db;
    sqlite3_stmt *stmt;
    int result = 0;

    if (sqlite3_open("database.db", &db) != SQLITE_OK)
    {
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
        return 0;
    }

    const char *sql = "SELECT id FROM users WHERE username = ? AND password = ?;";
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, 0) != SQLITE_OK)
    {
        fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(db));
        return 0;
    }

    sqlite3_bind_text(stmt, 1, username, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, password, -1, SQLITE_STATIC);

    if (sqlite3_step(stmt) == SQLITE_ROW)
    {
        result = 1; // Authentification réussie
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return result;
}

void send_message_to_channel(const char *channel, const char *message, int sender_socket)
{
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        if (clients[i] && clients[i]->socket != sender_socket && strcmp(clients[i]->current_channel, channel) == 0)
        {
            send(clients[i]->socket, message, strlen(message), 0);
        }
    }
    pthread_mutex_unlock(&clients_mutex);
}

void notify_current_channel(client_t *client)
{
    if (strlen(client->current_channel) > 0)
    {
        char message[BUFFER_SIZE];
        snprintf(message, sizeof(message), "Salon actuel : %s\n", client->current_channel);
        send(client->socket, message, strlen(message), 0);
    }
    else
    {
        send(client->socket, "Vous n'êtes dans aucun salon.\n", 31, 0);
    }
}

void list_channels(int socket) {
    sqlite3 *db;
    sqlite3_stmt *stmt;
    
    if (sqlite3_open("database.db", &db) != SQLITE_OK) {
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
        return;
    }

    const char *sql = "SELECT name FROM salons;";
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, 0) != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return;
    }

    char message[BUFFER_SIZE];
    snprintf(message, sizeof(message), "Liste des salons :\n");

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const char *channel_name = (const char *)sqlite3_column_text(stmt, 0);
        snprintf(message + strlen(message), sizeof(message) - strlen(message), "%s\n", channel_name);
    }

    send(socket, message, strlen(message), 0);  // Envoyer la liste au client

    sqlite3_finalize(stmt);
    sqlite3_close(db);
}


void *handle_client(void *arg)
{
    int client_socket = (intptr_t)arg;
    char buffer[BUFFER_SIZE];
    client_t *client = malloc(sizeof(client_t));
    client->socket = client_socket;
    strcpy(client->current_channel, ""); // Aucun salon par défaut

    // Authentification
    char username[50], password[50];

    // Recevoir le login et le mot de passe
    recv(client->socket, buffer, sizeof(buffer) - 1, 0);
    sscanf(buffer, "%49s %49s", username, password); // Extraire username et password
    clean_input(username);
    clean_input(password);

    if (authenticate_user(username, password))
    {
        strcpy(client->username, username);
        send(client->socket, "Authentication successful\n", 25, 0);

        // Ajoutez le client à la liste
        pthread_mutex_lock(&clients_mutex);
        for (int i = 0; i < MAX_CLIENTS; i++)
        {
            if (clients[i] == NULL)
            {
                clients[i] = client;
                break;
            }
        }
        pthread_mutex_unlock(&clients_mutex);
    }
    else
    {
        send(client->socket, "Authentication failed\n", 21, 0);
        close(client_socket);
        free(client);
        return NULL;
    }

    // Boucle principale du client
    while (1)
    {
        int bytes_received = recv(client->socket, buffer, sizeof(buffer) - 1, 0);
        if (bytes_received <= 0)
        {
            break; // Déconnexion
        }
        buffer[bytes_received] = '\0'; // Terminer la chaîne reçue

        if (strncmp(buffer, "join ", 5) == 0)
        {
            char *channel_name = buffer + 5;
            clean_input(channel_name);
            strcpy(client->current_channel, channel_name);
            char response[BUFFER_SIZE];
            snprintf(response, sizeof(response), "Vous avez rejoint le salon %s\n", channel_name);
            send(client->socket, response, strlen(response), 0);
            send_message_to_channel(channel_name, "A new user has joined the channel.\n", client->socket);
        }
        else if (strcmp(buffer, "leave") == 0)
        {
            if (strlen(client->current_channel) > 0)
            {
                char response[BUFFER_SIZE];
                snprintf(response, sizeof(response), "Vous avez quitté le salon %s\n", client->current_channel);
                send(client->socket, response, strlen(response), 0);
                send_message_to_channel(client->current_channel, "A user has left the channel.\n", client->socket);
                strcpy(client->current_channel, ""); // Réinitialiser le salon actuel
            }
            else
            {
                send(client->socket, "Vous n'êtes dans aucun salon.\n", 31, 0);
            }
        }
        else if (strcmp(buffer, "current") == 0)
        {
            notify_current_channel(client);
        }
        else if (strcmp(buffer, "list") == 0)
        {
            list_channels(client->socket); // Appeler la fonction pour lister les salons
        }

        else if (strcmp(buffer, "disconnect") == 0)
        {
            printf("%s se déconnecte.\n", client->username);
            break; // Sortir de la boucle pour déconnecter le client
        }
        else
        {
            if (strlen(client->current_channel) > 0)
            {
                char message[BUFFER_SIZE];
                snprintf(message, sizeof(message), "%s: %s\n", client->username, buffer);
                send_message_to_channel(client->current_channel, message, client->socket);
            }
            else
            {
                send(client->socket, "Vous n'êtes dans aucun salon.\n", 31, 0);
            }
        }
    }

    // Nettoyer après déconnexion
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        if (clients[i] == client)
        {
            clients[i] = NULL; // Retirer le client de la liste
            break;
        }
    }
    pthread_mutex_unlock(&clients_mutex);

    close(client_socket);
    free(client);
    return NULL;
}

int main()
{
    int server_fd, new_socket;
    struct sockaddr_in server_addr;
    socklen_t client_addr_len = sizeof(server_addr);
    pthread_t tid;

    // Configuration du serveur
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0)
    {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(8080);

    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 5) < 0)
    {
        perror("listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port 8080...\n");

    while ((new_socket = accept(server_fd, (struct sockaddr *)&server_addr, &client_addr_len)))
    {
        pthread_t tid;
        pthread_create(&tid, NULL, handle_client, (void *)(intptr_t)new_socket);
    }

    close(server_fd);
    return 0;
}
