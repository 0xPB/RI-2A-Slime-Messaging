
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

char current_channel[50]; /**< Global variable to store the current channel */

/**
 * @brief Cleans the input string by removing newline characters.
 *
 * This function replaces newline characters (`\n` or `\r`) with a null terminator
 * to properly format user input.
 *
 * @param str Pointer to the string to be cleaned.
 */
void clean_input(char *str);

/**
 * @brief Prints a message to the console.
 *
 * This function displays a received message from the server
 * and re-prompts the user for input.
 *
 * @param message The message to be printed.
 * @param current_input The user's current input to be displayed again.
 */
void print_message(const char *message, const char *current_input);

/**
 * @brief Receives a file from the server.
 *
 * This function receives a file from the server, writing it to the specified filename.
 * It first receives the file size and then receives the file content byte by byte.
 *
 * @param client_socket The socket connected to the server.
 * @param filename The name of the file to be created and written to.
 */
void receive_file_from_server(int client_socket, const char *filename);

/**
 * @brief Sends a file to the server.
 *
 * This function reads a file from the specified filename and sends it to the server,
 * first transmitting the file size followed by the file content.
 *
 * @param client_socket The socket connected to the server.
 * @param filename The name of the file to be sent.
 */
void send_file_to_server(int client_socket, const char *filename);

/**
 * @brief Handles the reception of messages from the server.
 *
 * This function receives messages from the server and processes them.
 * If a message is received, it is printed to the console.
 *
 * @param client_fd The socket file descriptor for the client.
 * @param current_input The user's current input to be displayed if necessary.
 */
void handle_receive(int client_fd, char *current_input);

/**
 * @brief Handles sending messages or files to the server.
 *
 * This function reads user input and sends messages or files to the server.
 * It also provides help information for commands.
 *
 * @param client_fd The socket file descriptor for the client.
 * @param current_input The user's current input to be saved and managed.
 */
void handle_send(int client_fd, char *current_input);

/**
 * @brief Main function to run the client application.
 *
 * This function initializes the client, handles user authentication,
 * and manages communication with the server using polling.
 *
 * @return Returns 0 on successful execution, or exits on failure.
 */
int main();

#endif // CLIENT_H
