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

typedef struct
{
    int socket;
    char username[50];
    char current_channel[50]; // salon actuel
    int is_admin;             // 1 pour admin, 0 pour utilisateur normal
} client_t;

client_t *clients[MAX_CLIENTS];

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
    if ((pos = strchr(str, '\n')) != NULL || (pos = strchr(str, '\r')) != NULL)
    {
        *pos = '\0'; // Remplacer le retour à la ligne ou retour chariot par un caractère de fin de chaîne
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

    // Afficher la requête et les paramètres utilisés
    printf("Authenticating user: %s, password: %s\n", username, password);

    if (sqlite3_step(stmt) == SQLITE_ROW)
    {
        result = 1; // Authentification réussie
    }
    else
    {
        // Ajoute un message pour voir si la requête échoue
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

void store_message_in_db(const char *channel, const char *username, const char *message)
{
    sqlite3 *db;
    sqlite3_stmt *stmt;

    if (sqlite3_open("database.db", &db) != SQLITE_OK)
    {
        fprintf(stderr, "Erreur lors de l'ouverture de la base de données : %s\n", sqlite3_errmsg(db));
        return;
    }

    // Préparer la requête SQL pour insérer le message
    const char *sql = "INSERT INTO messages (salon_id, username, message) VALUES ((SELECT id FROM salons WHERE name = ?), ?, ?);";
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, 0) != SQLITE_OK)
    {
        fprintf(stderr, "Erreur lors de la préparation de la requête SQL : %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return;
    }

    // Lier les valeurs du nom du salon, du nom d'utilisateur et du message à la requête SQL
    sqlite3_bind_text(stmt, 1, channel, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, username, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, message, -1, SQLITE_STATIC);

    // Exécuter la requête SQL
    if (sqlite3_step(stmt) != SQLITE_DONE)
    {
        fprintf(stderr, "Erreur lors de l'insertion du message : %s\n", sqlite3_errmsg(db));
    }

    // Finaliser et fermer la connexion à la base de données
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

    // Requête SQL pour supprimer tous les messages
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

    // Supprimer le dossier du salon
    char command[512];
    snprintf(command, sizeof(command), "rm -rf %s", directory_path);
    system(command);
}

void create_salon_directory(const char *salon_name)
{
    char directory_path[256];
    snprintf(directory_path, sizeof(directory_path), "server/%s", salon_name);

    // Créer le dossier si nécessaire
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
        exists = 1; // Le salon existe
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return exists;
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

void list_users_in_channel(client_t *client)
{
    char message[BUFFER_SIZE];
    snprintf(message, sizeof(message), "Utilisateurs connectés dans le salon %s:\n", client->current_channel);

    int found_user = 0; // Flag pour vérifier si des utilisateurs sont trouvés

    // Parcourir la liste des clients pour trouver ceux qui sont dans le même salon
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

    // Si aucun autre utilisateur n'a été trouvé
    if (!found_user)
    {
        snprintf(message + strlen(message), sizeof(message) - strlen(message), "Aucun autre utilisateur connecté.\n");
    }

    // Envoyer la liste des utilisateurs au client
    if (send(client->socket, message, strlen(message), 0) < 0)
    {
        perror("Erreur lors de l'envoi de la liste des utilisateurs");
    }
}

void send_message_to_channel(const char *channel, const char *message, int sender_socket)
{
    // Parcourir la liste des clients et envoyer le message à ceux qui sont dans le même salon
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

    // Extraire le nom d'utilisateur de l'envoyeur et stocker le message dans la base de données
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

void handle_list_admin(int admin_socket)
{
    // Suppression de l'utilisation du mutex, car la gestion des clients est désormais gérée par `poll`
    char message[BUFFER_SIZE];
    snprintf(message, sizeof(message), "Liste des utilisateurs connectés et leurs salons :\n");

    // Boucle pour parcourir les clients et récupérer leurs informations
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        if (clients[i]) // Si un client est connecté
        {
            snprintf(message + strlen(message), sizeof(message) - strlen(message),
                     "Utilisateur : %s, Salon : %s\n",
                     clients[i]->username,
                     strlen(clients[i]->current_channel) > 0 ? clients[i]->current_channel : "Aucun");
        }
    }

    // Envoyer la liste complète des utilisateurs connectés à l'administrateur
    if (send(admin_socket, message, strlen(message), 0) < 0)
    {
        perror("Erreur lors de l'envoi de la liste des utilisateurs");
    }
}

void *handle_exit_command(void *server_fd_ptr)
{
    int server_fd = *(int *)server_fd_ptr;
    char input[BUFFER_SIZE];

    while (1)
    {
        fgets(input, sizeof(input), stdin); // Lire la commande à partir de la console
        if (strncmp(input, "shut", 4) == 0)
        {
            // Vider la table des messages
            clear_messages_in_db();

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

void notify_current_channel(client_t *client)
{
    // Vérification que le client n'est pas NULL et que le salon actuel est valide
    if (client != NULL && strlen(client->current_channel) > 0)
    {
        // Ajout d'un message de débogage pour s'assurer que la chaîne est correcte
        printf("Salon actuel du client %s = %s\n", client->username, client->current_channel);

        // Créer un message à envoyer au client
        char message[BUFFER_SIZE];
        snprintf(message, sizeof(message), "Salon actuel : %s\n", client->current_channel);

        // Envoyer le message au client
        if (send(client->socket, message, strlen(message), 0) < 0)
        {
            perror("Erreur lors de l'envoi du message de salon actuel");
        }
    }
    else
    {
        // Si aucun salon n'est rejoint, informer le client
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
        create_salon_directory(salon_name); // Créer le dossier pour chaque salon
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

    // Obtenir la taille du fichier
    fseek(file, 0L, SEEK_END);
    long file_size = ftell(file);
    rewind(file);

    // Envoyer la taille du fichier
    char size_str[20];
    snprintf(size_str, sizeof(size_str), "%ld", file_size);
    send(client_socket, size_str, strlen(size_str), 0);

    // Attendre la confirmation du client pour démarrer le transfert
    char buffer[BUFFER_SIZE];
    recv(client_socket, buffer, sizeof(buffer), 0);

    // Transférer le fichier bit par bit
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

    // Réception de la taille du fichier
    char buffer[BUFFER_SIZE];
    int bytes_received = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
    if (bytes_received <= 0)
    {
        perror("Erreur lors de la réception de la taille du fichier");
        return;
    }
    buffer[bytes_received] = '\0';
    long file_size = atol(buffer); // Convertir la taille en nombre

    // Envoyer la confirmation pour démarrer le transfert
    send(client_socket, "OK", 2, 0);

    // Ouvrir le fichier pour l'écriture
    FILE *file = fopen(file_path, "wb");
    if (file == NULL)
    {
        perror("Erreur lors de la création du fichier");
        return;
    }

    // Recevoir les bits du fichier un par un
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

        // Écrire le byte dans le fichier
        fwrite(&byte, 1, 1, file);
        received_bytes++;
    }

    fclose(file);

    if (received_bytes == file_size)
    {
        printf("Fichier '%s' reçu avec succès et stocké dans le salon %s.\n", filename, salon_name);

        // Notifier les utilisateurs dans le salon que le fichier est disponible
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

    // Vérifier si le client s'est déconnecté ou s'il y a une erreur
    if (bytes_received <= 0)
    {
        printf("Client %s disconnected\n", client->username);

        // Supprimer le client de la liste des clients
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

    // Terminer la chaîne reçue
    buffer[bytes_received] = '\0';
    printf("Message reçu de %s: %s\n", client->username, buffer);

    // Si l'utilisateur n'est pas encore authentifié, on demande les credentials
    if (strlen(client->username) == 0)
    {
        // Supposons que l'utilisateur envoie 'username password'
        char username[50], password[50];
        sscanf(buffer, "%s %s", username, password);
        if (authenticate_user(username, password))
        {
            strcpy(client->username, username);
            client->is_admin = is_admin(username); // Vérifier et stocker si l'utilisateur est admin
            send(client_socket, "Authentification réussie\n", 25, 0);
        }
        else
        {
            send(client_socket, "Échec de l'authentification\n", 28, 0);
        }
        return;
    }
    // Gestion des différentes commandes client
    if (strncmp(buffer, "join ", 5) == 0)
    {
        // Commande pour rejoindre un salon
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
        // Commande pour quitter un salon
        if (strlen(client->current_channel) > 0)
        {
            char response[BUFFER_SIZE];
            snprintf(response, sizeof(response), "Vous avez quitté le salon %s\n", client->current_channel);
            send(client_socket, response, strlen(response), 0);
            send_message_to_channel(client->current_channel, "Un utilisateur a quitté le salon.\n", client->socket);
            strcpy(client->current_channel, ""); // Réinitialiser le salon
        }
        else
        {
            send(client_socket, "Vous n'êtes dans aucun salon.\n", 31, 0);
        }
    }
    else if (strcmp(buffer, "list_users") == 0)
    {
        // Commande pour lister les utilisateurs dans le salon
        if (strlen(client->current_channel) > 0)
        {
            list_users_in_channel(client); // Appelle la fonction pour lister les utilisateurs
        }
        else
        {
            send(client_socket, "Vous n'êtes dans aucun salon.\n", 31, 0);
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

    else if (strncmp(buffer, "send ", 5) == 0)
    {
        char *filename = buffer + 5;
        receive_file_from_client(client->socket, client->current_channel, filename);
    }

    // Par exemple, dans handle_client, si l'utilisateur envoie "receive <filename>"
    else if (strncmp(buffer, "receive ", 8) == 0)
    {
        char *filename = buffer + 8;
        send_file_to_client(client->socket, client->current_channel, filename);
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
        // Commande pour déconnexion
        printf("Client %s se déconnecte.\n", client->username);
        close(client_socket);

        // Supprimer le client de la liste des clients
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
        // Si aucune commande spécifique n'est reconnue, on considère que c'est un message pour le salon
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

    // Initialiser les dossiers des salons existants
    initialize_salon_directories();

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

    // Tableau de structures pollfd pour surveiller les sockets et l'entrée standard
    struct pollfd fds[MAX_CLIENTS + 2]; // +1 pour le socket du serveur et +1 pour STDIN_FILENO
    int nfds = 2;                       // Nombre de descripteurs surveillés (server + stdin)

    // Ajouter le socket du serveur à la liste des descripteurs surveillés
    fds[0].fd = server_fd;
    fds[0].events = POLLIN; // Surveiller les événements d'entrée

    // Ajouter l'entrée standard (console) pour la commande "shut"
    fds[1].fd = STDIN_FILENO; // Surveiller l'entrée standard (console)
    fds[1].events = POLLIN;   // Surveiller les événements d'entrée (input)

    // Initialiser le tableau des clients à -1 (aucun client connecté)
    for (int i = 2; i < MAX_CLIENTS + 2; i++)
    {
        fds[i].fd = -1;
    }

    while (1)
    {
        int poll_count = poll(fds, nfds, -1); // Attendre un événement sur les sockets ou l'entrée standard

        if (poll_count < 0)
        {
            perror("poll failed");
            exit(EXIT_FAILURE);
        }

        // Vérifier si le socket du serveur a une nouvelle connexion entrante
        if (fds[0].revents & POLLIN)
        {
            new_socket = accept(server_fd, (struct sockaddr *)&server_addr, &client_addr_len);
            if (new_socket < 0)
            {
                perror("accept failed");
                exit(EXIT_FAILURE);
            }

            printf("Nouvelle connexion acceptée.\n");

            // Ajouter le nouveau client au tableau des descripteurs
            for (int i = 2; i < MAX_CLIENTS + 2; i++)
            {
                if (fds[i].fd == -1)
                {
                    fds[i].fd = new_socket;
                    fds[i].events = POLLIN; // Surveiller les événements d'entrée
                    nfds++;

                    // Créer un nouveau client_t et l'associer au client
                    client_t *new_client = malloc(sizeof(client_t));
                    new_client->socket = new_socket;
                    strcpy(new_client->username, "");        // Initialiser le nom d'utilisateur à vide
                    strcpy(new_client->current_channel, ""); // Initialiser le salon à vide
                    clients[i - 2] = new_client;             // Associer ce client à l'index i-2 (correspondance clients[])

                    break;
                }
            }
        }

        // Vérifier si une commande a été entrée dans la console (entrée standard)
        if (fds[1].revents & POLLIN)
        {
            // Lire l'entrée de la console
            fgets(buffer, sizeof(buffer), stdin);
            buffer[strcspn(buffer, "\n")] = 0; // Enlever le retour à la ligne

            // Si la commande est "shut", fermer le serveur
            if (strcmp(buffer, "shut") == 0)
            {
                printf("Commande 'shut' détectée. Fermeture du serveur...\n");

                // Fermer toutes les connexions clients
                for (int i = 2; i < MAX_CLIENTS + 2; i++)
                {
                    if (fds[i].fd != -1)
                    {
                        printf("Fermeture de la connexion du client %s\n", clients[i - 2]->username);
                        close(fds[i].fd);      // Fermer le socket du client
                        free(clients[i - 2]);  // Libérer la mémoire du client
                        clients[i - 2] = NULL; // Supprimer le client de la liste
                        fds[i].fd = -1;        // Retirer le socket de poll
                    }
                }
                // Vider la table des messages
                clear_messages_in_db();
                // Supprimer tous les dossiers de salons
                const char *command = "rm -rf server/*";
                system(command);
                printf("Répertoires des salons supprimés.\n");

                // Fermer le socket du serveur
                close(server_fd);

                // Quitter le programme
                printf("Serveur arrêté.\n");
                exit(0);  // Terminer le programme proprement
            }
        }

        // Vérifier les événements sur les sockets des clients existants
        for (int i = 2; i < MAX_CLIENTS + 2; i++)
        {
            if (fds[i].fd != -1 && fds[i].revents & POLLIN)
            {
                // Un client a envoyé un message
                handle_client(fds[i].fd, clients[i - 2]); // Gérer la communication avec le client
            }
        }
    }

    close(server_fd);
    return 0;
}

