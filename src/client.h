#ifndef CLIENT_H
#define CLIENT_H

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

extern char current_channel[50]; /**< Global variable to store the current channel */

/**
 * @brief Cleans the input string by removing newline or carriage return characters.
 * 
 * @param str The input string to clean.
 */
void clean_input(char *str);

/**
 * @brief Displays a message from the server and restores the user's input.
 * 
 * @param message The message to display.
 * @param current_input The current user input to restore after displaying the message.
 */
void print_message(const char *message, const char *current_input);

/**
 * @brief Receives a file from the server.
 * 
 * This function receives a file from the server, storing it in the specified filename.
 * 
 * @param client_socket The socket connected to the server.
 * @param filename The name of the file to receive.
 */
void receive_file_from_server(int client_socket, const char *filename);

/**
 * @brief Sends a file to the server.
 * 
 * This function sends a file to the server from the specified filename.
 * 
 * @param client_socket The socket connected to the server.
 * @param filename The name of the file to send.
 */
void send_file_to_server(int client_socket, const char *filename);

/**
 * @brief Handles the reception of messages from the server.
 * 
 * This function is responsible for handling the reception of messages or data from the server,
 * including disconnect messages or other events.
 * 
 * @param client_fd The socket connected to the server.
 * @param current_input The current user input to maintain consistency during message reception.
 */
void handle_receive(int client_fd, char *current_input);

/**
 * @brief Handles sending user input to the server.
 * 
 * This function manages sending user messages or commands to the server, including file transfers.
 * 
 * @param client_fd The socket connected to the server.
 * @param current_input The current user input to manage.
 */
void handle_send(int client_fd, char *current_input);

#endif // CLIENT_H
