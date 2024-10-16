#include "client.h"

#define BUFFER_SIZE 1024

char current_channel[50] = "";

void clean_input(char *str)
{
    char *pos;
    if ((pos = strchr(str, '\n')) != NULL || (pos = strchr(str, '\r')) != NULL)
    {
        *pos = '\0';
    }
}

void print_message(const char *message, const char *current_input)
{
    printf("\r\033[K");      
    printf("%s\n", message);

    if (strlen(current_input) > 0)
    {
        printf("> %s", current_input);
    }
    else
    {
        printf("> ");
    }

    fflush(stdout);
}

void receive_file_from_server(int client_socket, const char *filename)
{
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
    FILE *file = fopen(filename, "wb");
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
    printf("Fichier '%s' envoyé au serveur.\n", filename);
}

void handle_receive(int client_fd, char *current_input)
{
    char buffer[BUFFER_SIZE];
    int bytes_received = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
    if (bytes_received > 0)
    {
        buffer[bytes_received] = '\0';
        if (strcmp(buffer, current_input) != 0)
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
    strncpy(current_input, buffer, sizeof(current_input) - 1);
    memset(current_input, 0, sizeof(current_input));
}

int main()
{
    int client_fd;
    struct sockaddr_in server_addr;
    char username[50], password[50];
    char current_input[BUFFER_SIZE] = "";

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
    printf("Login: ");
    fgets(username, sizeof(username), stdin);
    printf("Password: ");
    fgets(password, sizeof(password), stdin);
    char auth_info[100];
    snprintf(auth_info, sizeof(auth_info), "%s %s", username, password);
    send(client_fd, auth_info, strlen(auth_info), 0);
    char auth_response[BUFFER_SIZE];
    int bytes_received = recv(client_fd, auth_response, sizeof(auth_response) - 1, 0);
    auth_response[bytes_received] = '\0';
    printf("%s\n", auth_response);
    struct pollfd fds[2];
    fds[0].fd = STDIN_FILENO;
    fds[0].events = POLLIN;
    fds[1].fd = client_fd;
    fds[1].events = POLLIN;

    while (1)
    {
        int poll_count = poll(fds, 2, -1);
        if (poll_count < 0)
        {
            perror("poll() failed");
            break;
        }
        if (fds[0].revents & POLLIN)
        {
            handle_send(client_fd, current_input);
        }
        if (fds[1].revents & POLLIN)
        {
            handle_receive(client_fd, current_input);
        }
    }
    close(client_fd);
    return 0;
}