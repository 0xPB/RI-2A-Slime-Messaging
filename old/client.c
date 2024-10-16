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

char current_channel[50] = ""; 

/**
 * @brief Cleans the input by removing the newline character.
 * 
 * @param str The input string to be cleaned.
 */
void clean_input(char *str)
{
    char *pos;
    if ((pos = strchr(str, '\n')) != NULL)
    {
        *pos = '\0'; 
    }
}

/**
 * @brief Displays the received message and restores the current user input.
 * 
 * @param message The message to display.
 * @param current_input The current input from the user.
 */
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

/**
 * @brief Sends a file to the server in packets.
 * 
 * @param socket The socket to send the file through.
 * @param filename The name of the file to be sent.
 */
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

    send(socket, "", 0, 0);
}

/**
 * @brief Receives a file from the server in packets.
 * 
 * @param socket The socket to receive the file through.
 * @param filename The name of the file to be received.
 */
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

    while ((bytes_received = recv(socket, buffer, sizeof(buffer), 0)) > 0)
    {
        fwrite(buffer, 1, bytes_received, file);
        total_bytes_received += bytes_received;

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

/**
 * @brief Thread function to continuously receive messages from the server.
 * 
 * @param client_socket Pointer to the client socket.
 * @return void* 
 */
void *receive_messages(void *client_socket)
{
    int client_fd = *(int *)client_socket;
    char buffer[BUFFER_SIZE];
    int bytes_received;
    char current_input[BUFFER_SIZE] = ""; 

    while ((bytes_received = recv(client_fd, buffer, sizeof(buffer) - 1, 0)) > 0)
    {
        buffer[bytes_received] = '\0'; 

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

/**
 * @brief Thread function to send messages and handle user input.
 * 
 * @param client_socket Pointer to the client socket.
 * @return void* 
 */
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
            char *filename = buffer + 5; 
            if (*filename)
            {
                printf("Envoi du fichier : %s au salon %s\n", filename, current_channel);

                char command[BUFFER_SIZE];
                snprintf(command, sizeof(command), "send %s %s", current_channel, filename); 
                send(client_fd, command, strlen(command), 0);                                

                send_file(client_fd, filename); 
            }
            else
            {
                printf("Nom de fichier manquant.\n");
            }
        }

        else if (strncmp(buffer, "receive ", 8) == 0)
        {
            char *filename = buffer + 8; 
            if (*filename)
            {
                printf("Réception du fichier : %s\n", filename);

                send(client_fd, buffer, strlen(buffer), 0);

                receive_file_from_server(client_fd, filename);
            }
            else
            {
                printf("Nom de fichier manquant.\n");
            }
        }

        else if (strcmp(buffer, "disconnect") == 0)
        { 
            send(client_fd, buffer, strlen(buffer), 0);
            printf("Déconnexion...\n");
            break;
        }
        else
        {
            send(client_fd, buffer, strlen(buffer), 0);
        }

        current_input[0] = '\0';
    }

    pthread_exit(NULL);
}

/**
 * @brief Main function to initialize the client, authenticate, and manage threads.
 * 
 * @return int 
 */
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

    printf("Login: ");
    fgets(username, sizeof(username), stdin);
    clean_input(username); 
    printf("Password: ");
    fgets(password, sizeof(password), stdin);
    clean_input(password); 

    char auth_info[100];
    snprintf(auth_info, sizeof(auth_info), "%s %s", username, password);
    send(client_fd, auth_info, strlen(auth_info), 0);

    char auth_response[BUFFER_SIZE];
    int bytes_received = recv(client_fd, auth_response, sizeof(auth_response) - 1, 0);
    auth_response[bytes_received] = '\0';
    printf("%s\n", auth_response); 

    pthread_create(&receive_thread, NULL, receive_messages, (void *)&client_fd);
    pthread_create(&send_thread, NULL, send_messages, (void *)&client_fd);

    pthread_join(send_thread, NULL);

    pthread_cancel(receive_thread);

    close(client_fd);
    return 0;
}
