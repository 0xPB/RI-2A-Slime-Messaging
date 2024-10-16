/**
 * @file client.h
 * @brief Header file containing functions for client operations, including message handling, file transfers, and communication with the server.
 */

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

extern char current_channel[50]; ///< Global variable for storing the current chat channel.

/**
 * @brief Cleans the input string by removing newline or carriage return characters.
 * 
 * @param str Pointer to the input string to be cleaned.
 */
void clean_input(char *str);

/**
 * @brief Prints the received message while preserving the current user input.
 * 
 * @param message The message received from the server.
 * @param current_input The current input entered by the user.
 */
void print_message(const char *message, const char *current_input);

/**
 * @brief Receives a file from the server and writes it to the local file system.
 * 
 * @param client_socket The socket connected to the server.
 * @param filename The name of the file to be saved locally.
 */
void receive_file_from_server(int client_socket, const char *filename);

/**
 * @brief Sends a file from the local file system to the server.
 * 
 * @param client_socket The socket connected to the server.
 * @param filename The name of the file to be sent to the server.
 */
void send_file_to_server(int client_socket, const char *filename);

/**
 * @brief Handles the reception of messages from the server and prints them.
 * 
 * @param client_fd The socket connected to the server.
 * @param current_input The current input entered by the user, to be preserved during message printing.
 */
void handle_receive(int client_fd, char *current_input);

/**
 * @brief Handles the sending of user input to the server and manages file transfer commands.
 * 
 * @param client_fd The socket connected to the server.
 * @param current_input The user input to be sent to the server.
 */
void handle_send(int client_fd, char *current_input);


