#include "server.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <poll.h>
#include <sqlite3.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>


#define BUFFER_SIZE 1024
#define MAX_CLIENTS 10

client_t *clients[MAX_CLIENTS];

int is_admin(const char *username)
{
    sqlite3 *db;
    sqlite3_stmt *stmt;
    int result = 0;

    if (sqlite3_open("database.db", &db) != SQLITE_OK)
    {
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
        return 0;
    }

    const char *sql = "SELECT role FROM users WHERE username = ?;";
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, 0) != SQLITE_OK)
    {
        fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return 0;
    }

    sqlite3_bind_text(stmt, 1, username, -1, SQLITE_STATIC);

    if (sqlite3_step(stmt) == SQLITE_ROW)
    {
        const char *role = (const char *)sqlite3_column_text(stmt, 0);
        if (strcmp(role, "admin") == 0)
        {
            result = 1;
        }
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return result;
}

void clean_input(char *str)
{
    char *pos;
    if ((pos = strchr(str, '\n')) != NULL || (pos = strchr(str, '\r')) != NULL)
    {
        *pos = '\0';
    }
}

void clear_server_directory()
{
    system("rm -rf server/*");
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
        sqlite3_close(db);
        return 0;
    }

    sqlite3_bind_text(stmt, 1, username, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, password, -1, SQLITE_STATIC);

    printf("Authenticating user: %s, password: %s\n", username, password);

    if (sqlite3_step(stmt) == SQLITE_ROW)
    {
        result = 1;
    }
    else
    {
        printf("Authentication failed for user: %s\n", username);
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return result;
}

void store_file_in_salon(const char *salon_name, const char *filename)
{
    char directory_path[256];
    snprintf(directory_path, sizeof(directory_path), "server/%s", salon_name);

    struct stat st = {0};
    if (stat(directory_path, &st) == -1)
    {
        mkdir(directory_path, 0700);
    }

    char destination_path[256];
    snprintf(destination_path, sizeof(destination_path), "%s/%s", directory_path, filename);

    char command[512];
    snprintf(command, sizeof(command), "cp %s %s", filename, destination_path);
    system(command);
}

void store_message_in_db(const char *channel, const char *username, const char *message)
{
    sqlite3 *db;
    sqlite3_stmt *stmt;

    if (sqlite3_open("database.db", &db) != SQLITE_OK)
    {
        fprintf(stderr, "Erreur lors de l'ouverture de la base de données : %s\n", sqlite3_errmsg(db));
        return;
    }
    const char *sql = "INSERT INTO messages (salon_id, username, message) VALUES ((SELECT id FROM salons WHERE name = ?), ?, ?);";
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, 0) != SQLITE_OK)
    {
        fprintf(stderr, "Erreur lors de la préparation de la requête SQL : %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return;
    }

    sqlite3_bind_text(stmt, 1, channel, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, username, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, message, -1, SQLITE_STATIC);

    if (sqlite3_step(stmt) != SQLITE_DONE)
    {
        fprintf(stderr, "Erreur lors de l'insertion du message : %s\n", sqlite3_errmsg(db));
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);
}

void clear_messages_in_db()
{
    sqlite3 *db;

    if (sqlite3_open("database.db", &db) != SQLITE_OK)
    {
        fprintf(stderr, "Erreur lors de l'ouverture de la base de données : %s\n", sqlite3_errmsg(db));
        return;
    }

    const char *sql = "DELETE FROM messages;";
    char *err_msg = 0;

    if (sqlite3_exec(db, sql, 0, 0, &err_msg) != SQLITE_OK)
    {
        fprintf(stderr, "Erreur lors de la suppression des messages : %s\n", err_msg);
        sqlite3_free(err_msg);
    }
    else
    {
        printf("Tous les messages ont été supprimés de la base de données.\n");
    }

    sqlite3_close(db);
}

void delete_salon_directory(const char *salon_name)
{
    char directory_path[256];
    snprintf(directory_path, sizeof(directory_path), "server/%s", salon_name);

    char command[512];
    snprintf(command, sizeof(command), "rm -rf %s", directory_path);
    system(command);
}

void create_salon_directory(const char *salon_name)
{
    char directory_path[256];
    snprintf(directory_path, sizeof(directory_path), "server/%s", salon_name);

    struct stat st = {0};
    if (stat(directory_path, &st) == -1)
    {
        mkdir(directory_path, 0700);
        printf("Dossier créé pour le salon : %s\n", salon_name);
    }
}

int channel_exists(const char *channel_name)
{
    sqlite3 *db;
    sqlite3_stmt *stmt;
    int exists = 0;

    if (sqlite3_open("database.db", &db) != SQLITE_OK)
    {
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
        return 0;
    }

    const char *sql = "SELECT 1 FROM salons WHERE name = ?;";
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, 0) != SQLITE_OK)
    {
        fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return 0;
    }

    sqlite3_bind_text(stmt, 1, channel_name, -1, SQLITE_STATIC);

    if (sqlite3_step(stmt) == SQLITE_ROW)
    {
        exists = 1;
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return exists;
}

void create_channel(client_t *client, const char *channel_name)
{
    if (!is_admin(client->username))
    {
        send(client->socket, "Vous devez être un administrateur pour créer un salon.\n", 55, 0);
        return;
    }

    if (channel_exists(channel_name))
    {
        send(client->socket, "Ce salon existe déjà.\n", 23, 0);
        return;
    }

    sqlite3 *db;
    sqlite3_stmt *stmt;

    if (sqlite3_open("database.db", &db) != SQLITE_OK)
    {
        send(client->socket, "Erreur d'ouverture de la base de données.\n", 41, 0);
        return;
    }

    const char *sql = "INSERT INTO salons (name) VALUES (?);";
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, 0) != SQLITE_OK)
    {
        send(client->socket, "Erreur lors de la préparation de la requête SQL.\n", 48, 0);
        sqlite3_close(db);
        return;
    }

    clean_input((char *)channel_name);
    sqlite3_bind_text(stmt, 1, channel_name, -1, SQLITE_STATIC);

    if (sqlite3_step(stmt) == SQLITE_DONE)
    {
        send(client->socket, "Salon créé avec succès.\n", 25, 0);
        printf("Création du channel %s par %s\n", channel_name, client->username);
        create_salon_directory(channel_name);
    }
    else
    {
        send(client->socket, "Erreur lors de la création du salon.\n", 37, 0);
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);
}

void list_users_in_channel(client_t *client)
{
    char message[BUFFER_SIZE];
    snprintf(message, sizeof(message), "Utilisateurs connectés dans le salon %s:\n", client->current_channel);

    int found_user = 0;
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        if (clients[i] && strcmp(clients[i]->current_channel, client->current_channel) == 0)
        {
            if (strcmp(clients[i]->username, client->username) != 0)
            {
                snprintf(message + strlen(message), sizeof(message) - strlen(message), "%s\n", clients[i]->username);
                found_user = 1;
            }
        }
    }

    if (!found_user)
    {
        snprintf(message + strlen(message), sizeof(message) - strlen(message), "Aucun autre utilisateur connecté.\n");
    }

    if (send(client->socket, message, strlen(message), 0) < 0)
    {
        perror("Erreur lors de l'envoi de la liste des utilisateurs");
    }
}

void send_message_to_channel(const char *channel, const char *message, int sender_socket)
{
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        if (clients[i] && clients[i]->socket != sender_socket && strcmp(clients[i]->current_channel, channel) == 0)
        {
            if (send(clients[i]->socket, message, strlen(message), 0) < 0)
            {
                perror("Erreur lors de l'envoi du message au client");
            }
        }
    }

    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        if (clients[i] && clients[i]->socket == sender_socket)
        {
            store_message_in_db(channel, clients[i]->username, message);
            break;
        }
    }
}

void delete_channel(client_t *client, const char *channel_name)
{

    if (!is_admin(client->username))
    {
        send(client->socket, "Vous devez être un administrateur pour supprimer un salon.\n", 58, 0);
        return;
    }

    sqlite3 *db;
    sqlite3_stmt *stmt;
    char sql[BUFFER_SIZE];

    if (sqlite3_open("database.db", &db) != SQLITE_OK)
    {
        send(client->socket, "Erreur d'ouverture de la base de données.\n", 41, 0);
        return;
    }

    snprintf(sql, sizeof(sql), "DELETE FROM messages WHERE salon_id = (SELECT id FROM salons WHERE name = ?);");
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, 0) != SQLITE_OK)
    {
        send(client->socket, "Erreur lors de la préparation de la requête SQL pour supprimer les messages.\n", 75, 0);
        sqlite3_close(db);
        return;
    }
    sqlite3_bind_text(stmt, 1, channel_name, -1, SQLITE_STATIC);

    if (sqlite3_step(stmt) != SQLITE_DONE)
    {
        send(client->socket, "Erreur lors de la suppression des messages du salon.\n", 54, 0);
        sqlite3_finalize(stmt);
        sqlite3_close(db);
        return;
    }
    sqlite3_finalize(stmt);

 
    char message[BUFFER_SIZE];
    snprintf(message, sizeof(message), "Le salon %s a été supprimé par %s.\n", channel_name, client->username);
    send_message_to_channel(channel_name, message, client->socket); 


    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        if (clients[i] && strcmp(clients[i]->current_channel, channel_name) == 0)
        {
            send(clients[i]->socket, "Vous avez été déconnecté car le salon a été supprimé.\n", 60, 0);
            strcpy(clients[i]->current_channel, "\0");
            clients[i] = NULL;
        }
    }

    delete_salon_directory(channel_name);

    snprintf(sql, sizeof(sql), "DELETE FROM salons WHERE name = ?;");
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, 0) != SQLITE_OK)
    {
        send(client->socket, "Erreur lors de la préparation de la requête SQL pour supprimer le salon.\n", 72, 0);
        sqlite3_close(db);
        return;
    }
    sqlite3_bind_text(stmt, 1, channel_name, -1, SQLITE_STATIC);

    if (sqlite3_step(stmt) == SQLITE_DONE)
    {
        send(client->socket, "Salon supprimé avec succès.\n", 29, 0);
        printf("Suppression du channel %s par %s\n", channel_name, client->username);
    }
    else
    {
        send(client->socket, "Erreur lors de la suppression du salon.\n", 40, 0);
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);
}

void list_channels(int socket)
{
    sqlite3 *db;
    sqlite3_stmt *stmt;

    if (sqlite3_open("database.db", &db) != SQLITE_OK)
    {
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
        return;
    }

    const char *sql = "SELECT name FROM salons;";
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, 0) != SQLITE_OK)
    {
        fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return;
    }

    char message[BUFFER_SIZE];
    snprintf(message, sizeof(message), "Liste des salons :\n");

    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        const char *channel_name = (const char *)sqlite3_column_text(stmt, 0);
        snprintf(message + strlen(message), sizeof(message) - strlen(message), "%s\n", channel_name);
    }

    send(socket, message, strlen(message), 0);

    sqlite3_finalize(stmt);
    sqlite3_close(db);
}

void handle_list_admin(int admin_socket)
{
    char message[BUFFER_SIZE];
    snprintf(message, sizeof(message), "Liste des utilisateurs connectés et leurs salons :\n");

    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        if (clients[i])
        {
            snprintf(message + strlen(message), sizeof(message) - strlen(message),
                     "Utilisateur : %s, Salon : %s\n",
                     clients[i]->username,
                     strlen(clients[i]->current_channel) > 0 ? clients[i]->current_channel : "Aucun");
        }
    }

    if (send(admin_socket, message, strlen(message), 0) < 0)
    {
        perror("Erreur lors de l'envoi de la liste des utilisateurs");
    }
}

void notify_current_channel(client_t *client)
{
    if (client != NULL && strlen(client->current_channel) > 0)
    {
        printf("Salon actuel du client %s = %s\n", client->username, client->current_channel);
        char message[BUFFER_SIZE];
        snprintf(message, sizeof(message), "Salon actuel : %s\n", client->current_channel);
        if (send(client->socket, message, strlen(message), 0) < 0)
        {
            perror("Erreur lors de l'envoi du message de salon actuel");
        }
    }
    else
    {
        printf("Client %s n'a rejoint aucun salon.\n", client->username);
        if (send(client->socket, "Vous n'êtes dans aucun salon.\n", 31, 0) < 0)
        {
            perror("Erreur lors de l'envoi du message d'absence de salon");
        }
    }
}

void initialize_salon_directories()
{
    sqlite3 *db;
    sqlite3_stmt *stmt;

    if (sqlite3_open("database.db", &db) != SQLITE_OK)
    {
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
        return;
    }

    const char *sql = "SELECT name FROM salons;";
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, 0) != SQLITE_OK)
    {
        fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return;
    }

    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        const char *salon_name = (const char *)sqlite3_column_text(stmt, 0);
        create_salon_directory(salon_name);
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);
}

void send_file_to_client(int client_socket, const char *salon_name, const char *filename)
{
    char file_path[256];
    snprintf(file_path, sizeof(file_path), "server/%s/%s", salon_name, filename);

    FILE *file = fopen(file_path, "rb");
    if (file == NULL)
    {
        perror("Erreur lors de l'ouverture du fichier");
        send(client_socket, "Erreur : fichier introuvable.\n", 30, 0);
        return;
    }
    fseek(file, 0L, SEEK_END);
    long file_size = ftell(file);
    rewind(file);
    char size_str[20];
    snprintf(size_str, sizeof(size_str), "%ld", file_size);
    send(client_socket, size_str, strlen(size_str), 0);
    char buffer[BUFFER_SIZE];
    recv(client_socket, buffer, sizeof(buffer), 0);
    char byte;
    while (fread(&byte, 1, 1, file) == 1)
    {
        if (send(client_socket, &byte, 1, 0) < 0)
        {
            perror("Erreur lors de l'envoi du fichier");
            break;
        }
    }

    fclose(file);
    printf("Fichier '%s' envoyé au client.\n", filename);
}

void receive_file_from_client(int client_socket, const char *salon_name, const char *filename)
{
    char file_path[256];
    snprintf(file_path, sizeof(file_path), "server/%s/%s", salon_name, filename);
    char buffer[BUFFER_SIZE];
    int bytes_received = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
    if (bytes_received <= 0)
    {
        perror("Erreur lors de la réception de la taille du fichier");
        return;
    }
    buffer[bytes_received] = '\0';
    long file_size = atol(buffer);
    send(client_socket, "OK", 2, 0);
    FILE *file = fopen(file_path, "wb");
    if (file == NULL)
    {
        perror("Erreur lors de la création du fichier");
        return;
    }
    char byte;
    long received_bytes = 0;
    while (received_bytes < file_size)
    {
        int byte_received = recv(client_socket, &byte, 1, 0);
        if (byte_received <= 0)
        {
            perror("Erreur lors de la réception du fichier");
            break;
        }
        fwrite(&byte, 1, 1, file);
        received_bytes++;
    }

    fclose(file);

    if (received_bytes == file_size)
    {
        printf("Fichier '%s' reçu avec succès et stocké dans le salon %s.\n", filename, salon_name);


        char notification[BUFFER_SIZE];
        snprintf(notification, sizeof(notification), "Un nouveau fichier '%s' est disponible au téléchargement dans le salon %s.\n", filename, salon_name);
        send_message_to_channel(salon_name, notification, client_socket);
    }
    else
    {
        printf("Erreur : fichier incomplet reçu.\n");
    }
}

void handle_client(int client_socket, client_t *client)
{
    char buffer[BUFFER_SIZE];
    int bytes_received = recv(client_socket, buffer, sizeof(buffer) - 1, 0);


    if (bytes_received <= 0)
    {
        printf("Client %s disconnected\n", client->username);

        for (int i = 0; i < MAX_CLIENTS; i++)
        {
            if (clients[i] == client)
            {
                clients[i] = NULL;
                break;
            }
        }

        close(client_socket);
        free(client);
        return;
    }


    buffer[bytes_received] = '\0';
    printf("Message reçu de %s: %s\n", client->username, buffer);


    if (strlen(client->username) == 0)
    {

        char username[50], password[50];
        sscanf(buffer, "%s %s", username, password);
        if (authenticate_user(username, password))
        {
            strcpy(client->username, username);
            client->is_admin = is_admin(username);
            send(client_socket, "Authentification réussie\n", 25, 0);
        }
        else
        {
            send(client_socket, "Échec de l'authentification\n", 28, 0);
        }
        return;
    }
    if (strncmp(buffer, "join ", 5) == 0)
    {
        char *channel_name = buffer + 5;
        clean_input(channel_name);

        if (channel_exists(channel_name))
        {
            strcpy(client->current_channel, channel_name);
            char response[BUFFER_SIZE];
            snprintf(response, sizeof(response), "Vous avez rejoint le salon %s\n", channel_name);
            send(client_socket, response, strlen(response), 0);
            send_message_to_channel(client->current_channel, "Un utilisateur a rejoint le salon.\n", client->socket);
        }
        else
        {
            send(client_socket, "Ce salon n'existe pas.\n", 24, 0);
        }
    }
    else if (strcmp(buffer, "leave") == 0)
    {
        if (strlen(client->current_channel) > 0)
        {
            char response[BUFFER_SIZE];
            snprintf(response, sizeof(response), "Vous avez quitté le salon %s\n", client->current_channel);
            send(client_socket, response, strlen(response), 0);
            send_message_to_channel(client->current_channel, "Un utilisateur a quitté le salon.\n", client->socket);
            strcpy(client->current_channel, "");
        }
        else
        {
            send(client_socket, "Vous n'êtes dans aucun salon.\n", 31, 0);
        }
    }
    else if (strcmp(buffer, "list_users") == 0)
    {
        if (strlen(client->current_channel) > 0)
        {
            list_users_in_channel(client);
        }
        else
        {
            send(client_socket, "Vous n'êtes dans aucun salon.\n", 31, 0);
        }
    }
    else if (strcmp(buffer, "list_admin") == 0)
    {
        if (is_admin(client->username))
        {
            handle_list_admin(client->socket);
        }
        else
        {
            send(client->socket, "Vous n'êtes pas autorisé à utiliser cette commande.\n", 52, 0);
        }
    }
    else if (strcmp(buffer, "current") == 0)
    {
        notify_current_channel(client);
    }
    else if (strncmp(buffer, "create ", 7) == 0)
    {
        char *channel_name = buffer + 7;
        create_channel(client, channel_name); 
    }

    else if (strncmp(buffer, "send ", 5) == 0)
    {
        char *filename = buffer + 5;
        receive_file_from_client(client->socket, client->current_channel, filename);
    }

    else if (strncmp(buffer, "receive ", 8) == 0)
    {
        char *filename = buffer + 8;
        send_file_to_client(client->socket, client->current_channel, filename);
    }

    else if (strncmp(buffer, "delete ", 7) == 0)
    {
        char *channel_name = buffer + 7;      
        delete_channel(client, channel_name); 
    }

    else if (strcmp(buffer, "list") == 0)
    {
        list_channels(client->socket); 
    }
    else if (strcmp(buffer, "disconnect") == 0)
    {
        printf("Client %s se déconnecte.\n", client->username);
        close(client_socket);

        for (int i = 0; i < MAX_CLIENTS; i++)
        {
            if (clients[i] == client)
            {
                clients[i] = NULL;
                break;
            }
        }

        free(client);
        return;
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
            send(client_socket, "Vous n'êtes dans aucun salon.\n", 31, 0);
        }
    }
}

int main()
{
    char buffer[BUFFER_SIZE];
    clear_server_directory();

    int server_fd, new_socket;
    struct sockaddr_in server_addr;
    socklen_t client_addr_len = sizeof(server_addr);
    initialize_salon_directories();
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
    struct pollfd fds[MAX_CLIENTS + 2];
    int nfds = 2;                       

    fds[0].fd = server_fd;
    fds[0].events = POLLIN;
    fds[1].fd = STDIN_FILENO;
    fds[1].events = POLLIN;

    for (int i = 2; i < MAX_CLIENTS + 2; i++)
    {
        fds[i].fd = -1;
    }

    while (1)
    {
        int poll_count = poll(fds, nfds, -1);

        if (poll_count < 0)
        {
            perror("poll failed");
            exit(EXIT_FAILURE);
        }

        if (fds[0].revents & POLLIN)
        {
            new_socket = accept(server_fd, (struct sockaddr *)&server_addr, &client_addr_len);
            if (new_socket < 0)
            {
                perror("accept failed");
                exit(EXIT_FAILURE);
            }

            printf("Nouvelle connexion acceptée.\n");

            for (int i = 2; i < MAX_CLIENTS + 2; i++)
            {
                if (fds[i].fd == -1)
                {
                    fds[i].fd = new_socket;
                    fds[i].events = POLLIN;
                    nfds++;
                    client_t *new_client = malloc(sizeof(client_t));
                    new_client->socket = new_socket;
                    strcpy(new_client->username, "");        
                    strcpy(new_client->current_channel, ""); 
                    clients[i - 2] = new_client;

                    break;
                }
            }
        }
        if (fds[1].revents & POLLIN)
        {

            fgets(buffer, sizeof(buffer), stdin);
            buffer[strcspn(buffer, "\n")] = 0;
            if (strcmp(buffer, "shut") == 0)
            {
                printf("Commande 'shut' détectée. Fermeture du serveur...\n");
                for (int i = 2; i < MAX_CLIENTS + 2; i++)
                {
                    if (fds[i].fd != -1)
                    {
                        printf("Fermeture de la connexion du client %s\n", clients[i - 2]->username);
                        close(fds[i].fd);    
                        free(clients[i - 2]); 
                        clients[i - 2] = NULL;
                        fds[i].fd = -1;        
                    }
                }
                clear_messages_in_db();
                const char *command = "rm -rf server/*";
                system(command);
                printf("Répertoires des salons supprimés.\n");
                close(server_fd);
                printf("Serveur arrêté.\n");
                exit(0);
            }
        }
        for (int i = 2; i < MAX_CLIENTS + 2; i++)
        {
            if (fds[i].fd != -1 && fds[i].revents & POLLIN)
            {
                handle_client(fds[i].fd, clients[i - 2]);
            }
        }
    }

    close(server_fd);
    return 0;
}
