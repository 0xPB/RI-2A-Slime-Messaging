#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

#define BUFFER_SIZE 1024

void *receive_messages(void *socket_fd) {
    int fd = *((int *)socket_fd);
    char buffer[BUFFER_SIZE];
    while (1) {
        int bytes_received = recv(fd, buffer, BUFFER_SIZE, 0);
        if (bytes_received <= 0) {
            perror("recv() failed");
            break;
        }
        buffer[bytes_received] = '\0';
        printf("%s", buffer);
    }
    return NULL;
}

int main() {
    int client_fd;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];

    client_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (client_fd < 0) {
        perror("socket() failed");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8080);
    inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr);

    if (connect(client_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect() failed");
        close(client_fd);
        exit(EXIT_FAILURE);
    }

    pthread_t receive_thread;
    pthread_create(&receive_thread, NULL, receive_messages, (void *)&client_fd);

    while (1) {
        fgets(buffer, BUFFER_SIZE, stdin);
        send(client_fd, buffer, strlen(buffer), 0);
    }

    close(client_fd);
    return 0;
}
