#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <poll.h>

#define MAX_CLIENTS 10
#define MAX_SALONS 5
#define BUFFER_SIZE 1024

typedef struct {
    int fd;
    char username[50];
    int current_salon;
} Client;

typedef struct {
    char name[50];
    int clients[MAX_CLIENTS];
    int client_count;
} Salon;

Client clients[MAX_CLIENTS];
Salon salons[MAX_SALONS] = {{"General", {0}, 0}, {"Dev", {0}, 0}};
int salon_count = 2;

void broadcast_message(int salon_index, char *message, int sender_fd) {
    for (int i = 0; i < salons[salon_index].client_count; i++) {
        int client_fd = salons[salon_index].clients[i];
        if (client_fd != sender_fd) {
            send(client_fd, message, strlen(message), 0);
        }
    }
}

void handle_client_message(int client_fd) {
    char buffer[BUFFER_SIZE];
    int bytes_received = recv(client_fd, buffer, BUFFER_SIZE, 0);
    if (bytes_received <= 0) {
        close(client_fd);
        return;
    }
    buffer[bytes_received] = '\0';
    
    // Find the client and their current salon
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].fd == client_fd) {
            int salon_index = clients[i].current_salon;
            broadcast_message(salon_index, buffer, client_fd);
            break;
        }
    }
}

int main() {
    int server_fd, client_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket() failed");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(8080);

    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind() failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, MAX_CLIENTS) < 0) {
        perror("listen() failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    struct pollfd fds[MAX_CLIENTS];
    for (int i = 0; i < MAX_CLIENTS; i++) {
        fds[i].fd = -1;
    }

    fds[0].fd = server_fd;
    fds[0].events = POLLIN;

    while (1) {
        if (poll(fds, MAX_CLIENTS, -1) < 0) {
            perror("poll() failed");
            exit(EXIT_FAILURE);
        }

        if (fds[0].revents & POLLIN) {
            client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &addr_len);
            if (client_fd < 0) {
                perror("accept() failed");
                continue;
            }

            for (int i = 1; i < MAX_CLIENTS; i++) {
                if (fds[i].fd == -1) {
                    fds[i].fd = client_fd;
                    fds[i].events = POLLIN;

                    // Register new client
                    clients[i].fd = client_fd;
                    clients[i].current_salon = 0; // Join "General" by default
                    salons[0].clients[salons[0].client_count++] = client_fd;
                    break;
                }
            }
        }

        for (int i = 1; i < MAX_CLIENTS; i++) {
            if (fds[i].fd != -1 && fds[i].revents & POLLIN) {
                handle_client_message(fds[i].fd);
            }
        }
    }

    close(server_fd);
    return 0;
}
