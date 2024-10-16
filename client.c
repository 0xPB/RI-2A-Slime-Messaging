#include "client.h"

void clean_input(char *str)
{
    char *pos;
    if ((pos = strchr(str, '\n')) != NULL || (pos = strchr(str, '\r')) != NULL)
    {
        *pos = '\0'; // Remplacer le retour à la ligne ou retour chariot par un caractère de fin de chaîne
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

void receive_file_from_server(int client_socket, const char *filename)
{
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
    FILE *file = fopen(filename, "wb");
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
        printf("Fichier '%s' reçu avec succès.\n", filename);
    }
    else
    {
        printf("Erreur : fichier incomplet reçu.\n");
    }
}

void send_file_to_server(int client_socket, const char *filename)
{
    FILE *file = fopen(filename, "rb");
    if (file == NULL)
    {
        perror("Erreur lors de l'ouverture du fichier");
        return;
    }

    // Obtenir la taille du fichier
    fseek(file, 0L, SEEK_END);
    long file_size = ftell(file);
    rewind(file);

    // Envoyer la taille du fichier au serveur
    char size_str[20];
    snprintf(size_str, sizeof(size_str), "%ld", file_size); // écrire chaîne de caractère dans un buffer pour éviter débordement de mémoire
    send(client_socket, size_str, strlen(size_str), 0);

    // Attendre la confirmation du serveur pour commencer le transfert
    char buffer[BUFFER_SIZE];
    recv(client_socket, buffer, sizeof(buffer), 0);

    // Envoyer le fichier bit par bit
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
    printf("Fichier '%s' envoyé au serveur.\n", filename);
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

    if (strncmp(buffer, "receive ", 8) == 0)
    {
        char *filename = buffer + 8;
        receive_file_from_server(client_fd, filename);
    }

    else if (strncmp(buffer, "send ", 5) == 0)
    {
        char *filename = buffer + 5;
        send_file_to_server(client_fd, filename);
    }

    else if (strcmp(buffer, "help") == 0)
    {
        printf("\nLister tous les salons\t\t\t\t\t\tUsage : list\n");
        printf("\nLister tous les utilisateurs connectés dans le salon\t\tUsage : list_users\n");
        printf("\nLister tous les utilisateurs connectés dans tous les salons\tUsage : list_admin\n");
        printf("\nAfficher le salon actuel\t\t\t\t\tUsage : current\n");
        printf("\nCréer un salon\t\t\t\t\t\t\tUsage : create <nom_du_salon>\n");
        printf("\nSuprimer un salon\t\t\t\t\t\tUsage : delete <nom_du_salon>\n");
        printf("\nRejoindre un salon\t\t\t\t\t\tUsage : join <nom_du_salon>\n");
        printf("\nQuitter le salon\t\t\t\t\t\tUsage : leave\n");
        printf("\nEnvoyer un fichier au salon actuel.\t\t\t\tUsage : send <nom_du_fichier>\n");
        printf("\nRecevoir un fichier du salon actuel.\t\t\t\tUsage : receive <nom_du_fichier>\n");
        printf("\nSe déconnecter du serveur.\t\t\t\t\tUsage : disconnect\n");
        printf("\n");
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

    client_fd = socket(AF_INET, SOCK_STREAM, 0); // sock_stream - TCP
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

    // Dans client.c, après avoir reçu le mot de passe
    printf("Password: ");
    fgets(password, sizeof(password), stdin);

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
        int poll_count = poll(fds, 2, -1); // Attendre indéfiniment (-1) un événement sur stdin ou le socket client (2)
        if (poll_count < 0)                // valeur négative, erreur
        {
            perror("poll() failed");
            break;
        }

        // Vérifier si l'utilisateur a entré une commande
        if (fds[0].revents & POLLIN) // après l'appel à poll pour indiquer ce qu'il s'est réellement passé
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