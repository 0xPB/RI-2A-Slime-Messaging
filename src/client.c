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
#define PACKET_SIZE 1024

char current_channel[50] = ""; // Variable globale pour stocker le salon actuel

void clean_input(char *str)
{
    char *pos;
    if ((pos = strchr(str, '\n')) != NULL)
    {
        *pos = '\0'; // Retirer le retour à la ligne
    }
}

void print_message(const char *message, const char *current_input)
{
    printf("\r\033[K");      // Effacer la ligne actuelle
    printf("%s\n", message); // Afficher le message reçu du serveur

    if (strlen(current_input) > 0)
    {
        printf("> %s", current_input); // Réafficher l'entrée utilisateur sauvegardée
    }
    else
    {
        printf("> "); // Réafficher le prompt si aucune entrée utilisateur
    }

    fflush(stdout); // S'assurer que le tampon est vidé
}

void send_file(int socket, const char *filename)
{
    FILE *file = fopen(filename, "rb");
    if (file == NULL)
    {
        perror("Erreur lors de l'ouverture du fichier");
        return;
    }

    char buffer[PACKET_SIZE];
    size_t bytes_read;
    ssize_t bytes_sent;
    size_t total_bytes_sent = 0;

    // Envoyer le fichier en paquets
    while ((bytes_read = fread(buffer, 1, sizeof(buffer), file)) > 0)
    {
        size_t total_sent = 0;
        while (total_sent < bytes_read)
        {
            bytes_sent = send(socket, buffer + total_sent, bytes_read - total_sent, 0);
            if (bytes_sent < 0)
            {
                perror("Erreur lors de l'envoi du fichier");
                fclose(file);
                return;
            }
            total_sent += bytes_sent;
        }
        total_bytes_sent += total_sent;
    }

    if (ferror(file))
    {
        perror("Erreur lors de la lecture du fichier");
    }
    else
    {
        printf("Fichier %s envoyé avec succès (%zu octets).\n", filename, total_bytes_sent);
    }

    fclose(file);

    // Envoyer un paquet vide pour signaler la fin de la transmission
    send(socket, "", 0, 0);
}

void receive_file_from_server(int socket, const char *filename)
{
    FILE *file = fopen(filename, "wb");
    if (file == NULL)
    {
        perror("Erreur lors de la création du fichier");
        return;
    }

    char buffer[PACKET_SIZE];
    ssize_t bytes_received;
    size_t total_bytes_received = 0;

    // Recevoir le fichier en paquets
    while ((bytes_received = recv(socket, buffer, sizeof(buffer), 0)) > 0)
    {
        fwrite(buffer, 1, bytes_received, file);
        total_bytes_received += bytes_received;

        // Si moins que PACKET_SIZE est reçu, on a atteint la fin du fichier
        if (bytes_received < PACKET_SIZE)
        {
            break;
        }
    }

    if (bytes_received < 0)
    {
        perror("Erreur lors de la réception du fichier");
    }
    else
    {
        printf("Fichier %s reçu avec succès (%zu octets).\n", filename, total_bytes_received);
    }

    fclose(file);
}

void *receive_messages(void *client_socket)
{
    int client_fd = *(int *)client_socket;
    char buffer[BUFFER_SIZE];
    int bytes_received;
    char current_input[BUFFER_SIZE] = ""; // Pour sauvegarder l'entrée utilisateur

    while ((bytes_received = recv(client_fd, buffer, sizeof(buffer) - 1, 0)) > 0)
    {
        buffer[bytes_received] = '\0'; // Terminer la chaîne reçue

        // Afficher le message sans effacer l'entrée utilisateur
        print_message(buffer, current_input);
    }

    if (bytes_received == 0)
    {
        printf("Connexion fermée par le serveur.\n");
    }
    else if (bytes_received < 0)
    {
        perror("Erreur lors de la réception des données");
    }

    pthread_exit(NULL);
}

void *send_messages(void *client_socket)
{
    int client_fd = *(int *)client_socket;
    char buffer[BUFFER_SIZE];
    char current_input[BUFFER_SIZE] = "";

    while (1)
    {
        printf("> ");
        fflush(stdout);
        fgets(buffer, BUFFER_SIZE, stdin);
        clean_input(buffer);

        // Sauvegarder l'entrée actuelle dans current_input
        strncpy(current_input, buffer, BUFFER_SIZE);

        if (strcmp(buffer, "help") == 0)
        {
            printf("\nLister tout les salons\t\t\t\tUsage : list\n");
            printf("\nAfficher le salon actuel\t\t\tUsage : current\n");
            printf("\nCréer un salon\t\t\t\t\tUsage : create <nom_du_salon>\n");
            printf("\nSuprimer un salon\t\t\t\tUsage : delete <nom_du_salon>\n");
            printf("\nRejoindre un salon\t\t\t\tUsage : join <nom_du_salon>\n");
            printf("\nQuitter le salon\t\t\t\tUsage : leave\n");
            printf("\nEnvoyer un fichier au salon actuel.\t\tUsage : send <nom_du_fichier>\n");
            printf("\nRecevoir un fichier du salon actuel.\t\tUsage : receive <nom_du_fichier>\n");
            printf("\nSe déconnecter du serveur.\t\t\tUsage : disconnect\n");
            printf("\n");
        }

        else if (strncmp(buffer, "send ", 5) == 0)
        {
            char *filename = buffer + 5; // Récupérer le nom de fichier
            if (*filename)
            {
                printf("Envoi du fichier : %s au salon %s\n", filename, current_channel);

                // Envoyer d'abord la commande pour informer le serveur du nom du salon
                char command[BUFFER_SIZE];
                snprintf(command, sizeof(command), "send %s %s", current_channel, filename); // Inclure le nom du salon
                send(client_fd, command, strlen(command), 0);                                // Envoyer la commande au serveur

                // Envoyer le fichier
                send_file(client_fd, filename); // Appeler la fonction pour envoyer le fichier
            }
            else
            {
                printf("Nom de fichier manquant.\n");
            }
        }

        else if (strncmp(buffer, "receive ", 8) == 0)
        {
            char *filename = buffer + 8; // Récupérer le nom de fichier
            if (*filename)
            {
                printf("Réception du fichier : %s\n", filename);

                // Envoyer la commande de réception de fichier au serveur
                send(client_fd, buffer, strlen(buffer), 0);

                // Recevoir le fichier
                receive_file_from_server(client_fd, filename);
            }
            else
            {
                printf("Nom de fichier manquant.\n");
            }
        }

        else if (strcmp(buffer, "disconnect") == 0)
        { // Gérer la déconnexion
            send(client_fd, buffer, strlen(buffer), 0);
            printf("Déconnexion...\n");
            break;
        }
        else
        {
            // Envoi du message
            send(client_fd, buffer, strlen(buffer), 0);
        }

        // Réinitialiser l'entrée utilisateur après envoi
        current_input[0] = '\0';
    }

    pthread_exit(NULL);
}

int main()
{
    int client_fd;
    struct sockaddr_in server_addr;
    char username[50], password[50];
    pthread_t receive_thread, send_thread;

    client_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (client_fd < 0)
    {
        perror("socket() failed");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8080);
    inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr);

    if (connect(client_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("connect() failed");
        close(client_fd);
        exit(EXIT_FAILURE);
    }

    // Authentification
    printf("Login: ");
    fgets(username, sizeof(username), stdin);
    clean_input(username); // Nettoyer le retour à la ligne
    printf("Password: ");
    fgets(password, sizeof(password), stdin);
    clean_input(password); // Nettoyer le retour à la ligne

    // Envoi des informations d'identification
    char auth_info[100];
    snprintf(auth_info, sizeof(auth_info), "%s %s", username, password);
    send(client_fd, auth_info, strlen(auth_info), 0);

    // Attendre la réponse d'authentification
    char auth_response[BUFFER_SIZE];
    int bytes_received = recv(client_fd, auth_response, sizeof(auth_response) - 1, 0);
    auth_response[bytes_received] = '\0';
    printf("%s\n", auth_response); // Afficher le message d'authentification

    // Créer les threads pour recevoir et envoyer des messages
    pthread_create(&receive_thread, NULL, receive_messages, (void *)&client_fd);
    pthread_create(&send_thread, NULL, send_messages, (void *)&client_fd);

    // Attendre la fin des threads
    pthread_join(send_thread, NULL);

    // Terminer le thread de réception
    pthread_cancel(receive_thread);

    close(client_fd);
    return 0;
}
