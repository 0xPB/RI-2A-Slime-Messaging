#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

#define BUFFER_SIZE 1024

void clean_input(char *str)
{
    char *pos;
    if ((pos = strchr(str, '\n')) != NULL)
    {
        *pos = '\0'; // Retirer le retour à la ligne
    }
}

void *receive_messages(void *client_socket)
{
    int client_fd = *(int *)client_socket;
    char buffer[BUFFER_SIZE];
    int bytes_received;

    while ((bytes_received = recv(client_fd, buffer, sizeof(buffer) - 1, 0)) > 0)
    {
        buffer[bytes_received] = '\0'; // Terminer la chaîne reçue
        printf("%s\n", buffer);        // Afficher le message du serveur
        printf("> ");                  // Afficher le prompt à nouveau après le message
        fflush(stdout);                // Assurer que le tampon est vidé
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

int main()
{
    int client_fd;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];
    char username[50];

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

    pthread_t receive_thread;
    pthread_create(&receive_thread, NULL, receive_messages, (void *)&client_fd);

    // Authentification
    char password[50];
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

    // Boucle principale du client
    while (1)
    {
        printf("> ");   // Indiquer que le client est prêt à recevoir une commande
        fflush(stdout); // Assurer que le prompt s'affiche immédiatement
        fgets(buffer, BUFFER_SIZE, stdin);
        clean_input(buffer); // Nettoyer les retours à la ligne

        // Vérifiez si l'utilisateur a entré la commande "disconnect"
        if (strcmp(buffer, "disconnect") == 0)
        {
            send(client_fd, buffer, strlen(buffer), 0);
            printf("Déconnexion...\n");
            break; // Sortir de la boucle
        }

        send(client_fd, buffer, strlen(buffer), 0);
    }

    close(client_fd);
    return 0;
}
