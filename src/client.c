#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sqlite3.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <poll.h>

#define BUFFER_SIZE 1024

char current_channel[50] = ""; // Variable globale pour stocker le salon actuel

void clean_input(char *str) {
    char *pos;
    if ((pos = strchr(str, '\n')) != NULL || (pos = strchr(str, '\r')) != NULL) {
        *pos = '\0';  // Remplacer le retour à la ligne ou retour chariot par un caractère de fin de chaîne
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


void handle_receive(int client_fd, char *current_input)
{
    char buffer[BUFFER_SIZE];
    int bytes_received = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
    if (bytes_received > 0)
    {
        buffer[bytes_received] = '\0';
        if (strcmp(buffer, current_input) != 0) // Ne pas réafficher l'entrée utilisateur
        {
            print_message(buffer, current_input);
        }
    }
    else if (bytes_received == 0)
    {
        printf("Le serveur a fermé la connexion.\n");
        exit(0);
    }
    else
    {
        perror("Erreur lors de la réception des données");
    }
}



void handle_send(int client_fd, char *current_input)
{
    char buffer[BUFFER_SIZE];
    printf("> ");
    fflush(stdout);
    fgets(buffer, BUFFER_SIZE, stdin);
    clean_input(buffer);

    // Envoi du message
    if (send(client_fd, buffer, strlen(buffer), 0) < 0)
    {
        perror("Erreur lors de l'envoi du message");
    }

    // Sauvegarde de l'entrée utilisateur
    strncpy(current_input, buffer, sizeof(current_input) - 1);

    // Effacer l'entrée utilisateur après l'envoi
    memset(current_input, 0, sizeof(current_input));
}




int main()
{
    int client_fd;
    struct sockaddr_in server_addr;
    char username[50], password[50];
    char current_input[BUFFER_SIZE] = ""; // Pour sauvegarder l'entrée utilisateur

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
    //clean_input(username); // Nettoyer le retour à la ligne

    // Dans client.c, après avoir reçu le mot de passe
    printf("Password: ");
    fgets(password, sizeof(password), stdin);
    //clean_input(password); // Nettoyer le retour à la ligne ou autres caractères parasites

    // Ensuite, lors de l'envoi
    char auth_info[100];
    snprintf(auth_info, sizeof(auth_info), "%s %s", username, password);

    // S'assurer d'envoyer uniquement la longueur correcte
    send(client_fd, auth_info, strlen(auth_info), 0);

    // Attendre la réponse d'authentification
    char auth_response[BUFFER_SIZE];
    int bytes_received = recv(client_fd, auth_response, sizeof(auth_response) - 1, 0);
    auth_response[bytes_received] = '\0';
    printf("%s\n", auth_response); // Afficher le message d'authentification

    // Utiliser `poll` pour gérer à la fois les entrées utilisateur et les messages du serveur
    struct pollfd fds[2];
    fds[0].fd = STDIN_FILENO; // Entrée utilisateur (stdin)
    fds[0].events = POLLIN;   // Surveiller les entrées disponibles
    fds[1].fd = client_fd;    // Socket client
    fds[1].events = POLLIN;   // Surveiller les messages du serveur

    while (1)
    {
        int poll_count = poll(fds, 2, -1); // Attendre un événement sur stdin ou le socket client
        if (poll_count < 0)
        {
            perror("poll() failed");
            break;
        }

        // Vérifier si l'utilisateur a entré une commande
        if (fds[0].revents & POLLIN)
        {
            handle_send(client_fd, current_input); // Gérer l'entrée utilisateur et envoyer le message
        }

        // Vérifier si des données sont reçues du serveur
        if (fds[1].revents & POLLIN)
        {
            handle_receive(client_fd, current_input); // Gérer la réception des messages du serveur
        }
    }

    close(client_fd);
    return 0;
}
