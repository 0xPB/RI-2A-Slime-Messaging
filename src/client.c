#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

#define BUFFER_SIZE 1024

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

// Remplacez ces variables par celles de votre projet
const char *remote_user = "handrejewsk";                                   // Nom d'utilisateur sur le serveur
const char *remote_host = "10.7.2.203";                                    // Adresse IP ou nom de domaine du serveur
const char *remote_path = "/net/netud/r/handrejewsk/RE213/projet/server/"; // Chemin du dossier de destination sur le serveur

void send_file(const char *local_file)
{
    if (strlen(current_channel) == 0)
    {
        printf("Vous n'êtes dans aucun salon. Veuillez rejoindre un salon avant d'envoyer des fichiers.\n");
        return;
    }

    char remote_path_with_channel[256];
    snprintf(remote_path_with_channel, sizeof(remote_path_with_channel), "%s%s/", remote_path, current_channel);

    char command[1024];
    snprintf(command, sizeof(command), "scp %s %s@%s:%s", local_file, remote_user, remote_host, remote_path_with_channel);
    int ret = system(command);
    if (ret == 0)
    {
        printf("Fichier %s envoyé avec succès dans le salon %s.\n", local_file, current_channel);
    }
    else
    {
        printf("Erreur lors de l'envoi du fichier %s dans le salon %s.\n", local_file, current_channel);
    }
}

void receive_file(const char *remote_file, const char *local_path)
{
    if (strlen(current_channel) == 0)
    {
        printf("Vous n'êtes dans aucun salon. Veuillez rejoindre un salon avant de recevoir des fichiers.\n");
        return;
    }

    char remote_path_with_channel[256];
    snprintf(remote_path_with_channel, sizeof(remote_path_with_channel), "%s%s/", remote_path, current_channel);

    char command[1024];
    snprintf(command, sizeof(command), "scp %s@%s:%s%s %s", remote_user, remote_host, remote_path_with_channel, remote_file, local_path);
    int ret = system(command);
    if (ret == 0)
    {
        printf("Fichier %s reçu avec succès depuis le salon %s.\n", remote_file, current_channel);
    }
    else
    {
        printf("Erreur lors de la réception du fichier %s depuis le salon %s.\n", remote_file, current_channel);
    }
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

        if (strncmp(buffer, "send ", 5) == 0)
        {
            char *filename = buffer + 5; // Récupérer le nom de fichier
            if (*filename)
            { // Vérifier que le nom de fichier n'est pas vide
                send_file(filename);
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
            { // Vérifier que le nom de fichier n'est pas vide
                receive_file(filename, ".");
            }
            else
            {
                printf("Nom de fichier manquant.\n");
            }
        }
        else if (strcmp(buffer, "help") == 0)
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
        else if (strncmp(buffer, "join ", 5) == 0)
        {                                    // Gérer la commande join
            char *channel_name = buffer + 5; // Récupérer le nom du salon
            if (*channel_name)
            {                                                                        // Vérifier que le nom du salon n'est pas vide
                strncpy(current_channel, channel_name, sizeof(current_channel) - 1); // Mettre à jour le salon actuel
                current_channel[sizeof(current_channel) - 1] = '\0';                 // Assurer la terminaison de la chaîne
                printf("Vous avez rejoint le salon : %s\n", current_channel);
            }
            else
            {
                printf("Nom de salon manquant.\n");
            }
            send(client_fd, buffer, strlen(buffer), 0); // Envoyer la commande au serveur
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
