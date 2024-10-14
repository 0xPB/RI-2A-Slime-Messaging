#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <sqlite3.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>

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

// Structure pour stocker les utilisateurs connectés
typedef struct
{
    char username[50];
    int is_admin; // 1 pour admin, 0 pour utilisateur normal
} ConnectedUser;

ConnectedUser connected_users[MAX_CLIENTS]; // MAX_USERS est la taille maximale
int user_count = 0;                         // Compteur pour les utilisateurs connectés

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
            result = 1; // L'utilisateur est un admin
        }
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return result;
}

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

    strcpy(connected_users[user_count].username, username);
    connected_users[user_count].is_admin = is_admin(username); // Passer username à is_admin
    user_count++;

    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return result;
}

void store_file_in_salon(const char *salon_name, const char *filename)
{
    char directory_path[256];
    snprintf(directory_path, sizeof(directory_path), "server/%s", salon_name);

    // Créer le dossier si nécessaire
    struct stat st = {0};
    if (stat(directory_path, &st) == -1)
    {
        mkdir(directory_path, 0700);
    }

    // Construire le chemin de destination pour le fichier
    char destination_path[256];
    snprintf(destination_path, sizeof(destination_path), "%s/%s", directory_path, filename);

    // Utiliser la commande cp pour copier le fichier
    char command[512];
    snprintf(command, sizeof(command), "cp %s %s", filename, destination_path);
    system(command);
}

void clear_server_directory()
{
    system("rm -rf server/*");
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
        exists = 1; // Le salon existe
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return exists;
}

void create_salon_directory(const char *salon_name) {
    char directory_path[256];
    snprintf(directory_path, sizeof(directory_path), "server/%s", salon_name);

    // Créer le dossier si nécessaire
    struct stat st = {0};
    if (stat(directory_path, &st) == -1) {
        mkdir(directory_path, 0700);
        printf("Dossier créé pour le salon : %s\n", salon_name);
    }
}

void initialize_salon_directories() {
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

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const char *salon_name = (const char *)sqlite3_column_text(stmt, 0);
        create_salon_directory(salon_name); // Créer le dossier pour chaque salon
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);
}



void create_channel(client_t *client, const char *channel_name)
{
    // Vérifier si l'utilisateur est un admin
    if (!is_admin(client->username))
    {
        send(client->socket, "Vous devez être un administrateur pour créer un salon.\n", 55, 0);
        return;
    }

    // Vérifier si le salon existe déjà
    if (channel_exists(channel_name))
    {
        send(client->socket, "Ce salon existe déjà.\n", 23, 0);
        return;
    }

    sqlite3 *db;
    sqlite3_stmt *stmt;

    // Ouvrir la base de données
    if (sqlite3_open("database.db", &db) != SQLITE_OK)
    {
        send(client->socket, "Erreur d'ouverture de la base de données.\n", 41, 0);
        return;
    }

    // Préparer la requête SQL pour insérer un nouveau salon
    const char *sql = "INSERT INTO salons (name) VALUES (?);";
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, 0) != SQLITE_OK)
    {
        send(client->socket, "Erreur lors de la préparation de la requête SQL.\n", 48, 0);
        sqlite3_close(db);
        return;
    }

    // Nettoyer le nom du salon et le lier à la requête SQL
    clean_input((char *)channel_name);
    sqlite3_bind_text(stmt, 1, channel_name, -1, SQLITE_STATIC);

    // Exécuter la requête
    if (sqlite3_step(stmt) == SQLITE_DONE)
    {
        send(client->socket, "Salon créé avec succès.\n", 25, 0);
        printf("Création du channel %s par %s\n", channel_name, client->username);

        // Créer le dossier pour le salon
        create_salon_directory(channel_name);
    }
    else
    {
        send(client->socket, "Erreur lors de la création du salon.\n", 37, 0);
    }

    // Finaliser la requête et fermer la base de données
    sqlite3_finalize(stmt);
    sqlite3_close(db);
}

void delete_salon_directory(const char *salon_name) {
    char directory_path[256];
    snprintf(directory_path, sizeof(directory_path), "server/%s", salon_name);

    // Supprimer le dossier du salon
    char command[512];
    snprintf(command, sizeof(command), "rm -rf %s", directory_path);
    system(command);
}


void delete_channel(client_t *client, const char *channel_name)
{
    // Vérifier si l'utilisateur est un admin
    if (!is_admin(client->username))
    {
        send(client->socket, "Vous devez être un administrateur pour supprimer un salon.\n", 58, 0);
        return;
    }

    sqlite3 *db;
    sqlite3_stmt *stmt;
    char sql[BUFFER_SIZE];

    // Ouvrir la base de données
    if (sqlite3_open("database.db", &db) != SQLITE_OK)
    {
        send(client->socket, "Erreur d'ouverture de la base de données.\n", 41, 0);
        return;
    }

    // Supprimer les messages liés au salon
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

    // Annonce la suppression du salon à tous les clients présents
    char message[BUFFER_SIZE];
    snprintf(message, sizeof(message), "Le salon %s a été supprimé par %s.\n", channel_name, client->username);
    send_message_to_channel(channel_name, message, client->socket); // Informer tous les utilisateurs

    // Déconnecter tous les utilisateurs présents dans le salon
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        if (clients[i] && strcmp(clients[i]->current_channel, channel_name) == 0)
        {
            // Informer l'utilisateur qu'il a été déconnecté
            send(clients[i]->socket, "Vous avez été déconnecté car le salon a été supprimé.\n", 54, 0);
            strcpy(clients[i]->current_channel, "\0");
            clients[i] = NULL; // Retirer le client de la liste
        }
    }
    pthread_mutex_unlock(&clients_mutex);

    // Supprimer le dossier du salon
    delete_salon_directory(channel_name);

    // Supprimer le salon
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

    // Finaliser la requête et fermer la base de données
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

    send(socket, message, strlen(message), 0); // Envoyer la liste au client

    sqlite3_finalize(stmt);
    sqlite3_close(db);
}

void list_users_in_channel(client_t *client)
{
    pthread_mutex_lock(&clients_mutex);
    char message[BUFFER_SIZE];
    snprintf(message, sizeof(message), "Utilisateurs connectés dans le salon %s:\n", client->current_channel);

    int found_user = 0; // Flag pour vérifier si des utilisateurs sont trouvés

    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        if (clients[i] && strcmp(clients[i]->current_channel, client->current_channel) == 0)
        {
            // Exclure l'utilisateur lui-même de la liste
            if (strcmp(clients[i]->username, client->username) != 0)
            {
                snprintf(message + strlen(message), sizeof(message) - strlen(message), "%s\n", clients[i]->username);
                found_user = 1; // Marquer qu'au moins un utilisateur a été trouvé
            }
        }
    }

    if (!found_user)
    {
        snprintf(message + strlen(message), sizeof(message) - strlen(message), "Aucun autre utilisateur connecté.\n");
    }

    send(client->socket, message, strlen(message), 0); // Envoyer la liste au client
    pthread_mutex_unlock(&clients_mutex);
}

void handle_list_admin(int admin_socket)
{
    pthread_mutex_lock(&clients_mutex); // Protéger l'accès à la liste des clients
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

    send(admin_socket, message, strlen(message), 0); // Envoyer la liste à l'administrateur
    pthread_mutex_unlock(&clients_mutex);
}

// Fonction pour gérer la commande "exit"
void *handle_exit_command(void *server_fd_ptr)
{
    int server_fd = *(int *)server_fd_ptr;
    char input[BUFFER_SIZE];

    while (1)
    {
        fgets(input, sizeof(input), stdin); // Lire la commande à partir de la console
        if (strncmp(input, "shut", 4) == 0)
        {
            // Supprimer tous les dossiers de salons
            const char *command = "rm -r server/*";
            system(command);

            // Fermer le socket du serveur
            close(server_fd);

            // Quitter le programme
            exit(0);
        }
    }

    return NULL;
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

        printf("%s se connecte.\n", client->username);

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

            // Vérifier si le salon existe avant de permettre de le rejoindre
            if (channel_exists(channel_name))
            {
                strcpy(client->current_channel, channel_name);
                char response[BUFFER_SIZE];
                snprintf(response, sizeof(response), "Vous avez rejoint le salon %s\n", channel_name);
                send(client->socket, response, strlen(response), 0);
                send_message_to_channel(channel_name, "A new user has joined the channel.\n", client->socket);
            }
            else
            {
                send(client->socket, "Ce salon n'existe pas.\n", 24, 0);
            }
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
        else if (strcmp(buffer, "list_users") == 0) // Commande pour lister les utilisateurs
        {
            if (strlen(client->current_channel) > 0)
            {
                list_users_in_channel(client); // Appeler la fonction pour lister les utilisateurs
            }
            else
            {
                send(client->socket, "Vous n'êtes dans aucun salon.\n", 31, 0);
            }
        }
        else if (strcmp(buffer, "list_admin") == 0)
        {
            // Vérifier si l'utilisateur est un administrateur
            if (is_admin(client->username))
            {
                handle_list_admin(client->socket); // Appeler la fonction pour lister les utilisateurs
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
            char *channel_name = buffer + 7;      // Extraire le nom du salon après "create "
            create_channel(client, channel_name); // Appeler la fonction pour créer le salon
        }

        else if (strncmp(buffer, "delete ", 7) == 0)
        {
            char *channel_name = buffer + 7;      // Extraire le nom du salon après "delete "
            delete_channel(client, channel_name); // Appeler la fonction pour supprimer le salon
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

int main() {
    char buffer[BUFFER_SIZE];
    clear_server_directory();
    int server_fd, new_socket;
    struct sockaddr_in server_addr;
    socklen_t client_addr_len = sizeof(server_addr);
    pthread_t tid;

    // Initialiser les dossiers des salons existants
    initialize_salon_directories();

    // Configuration du serveur
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(8080);

    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 5) < 0) {
        perror("listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port 8080...\n");

    // Thread pour gérer la commande "exit"
    pthread_t exit_tid;
    pthread_create(&exit_tid, NULL, handle_exit_command, (void *)&server_fd);

    while ((new_socket = accept(server_fd, (struct sockaddr *)&server_addr, &client_addr_len))) {
        pthread_t tid;
        pthread_create(&tid, NULL, handle_client, (void *)(intptr_t)new_socket);
    }

    close(server_fd);
    return 0;
}