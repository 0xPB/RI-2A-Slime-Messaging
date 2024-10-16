/**
 * @file client.h
 * @brief Header file for the chat client application.
 *
 * This file contains the necessary includes, constants, and function
 * declarations for the chat client application.
 */

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

/**
 * @brief Buffer size used for message transmission.
 */
#define BUFFER_SIZE 1024

/**
 * @brief Global variable to store the current chat channel.
 */
extern char current_channel[50];

/**
 * @brief Cleans the input string by removing newline or carriage return characters.
 *
 * This function searches for newline (`\n`) or carriage return (`\r`) characters
 * in the input string and replaces them with a null terminator (`\0`).
 *
 * @param[in,out] str The input string to be cleaned.
 */
void clean_input(char *str);

/**
 * @brief Prints a message received from the server, updating the current input display.
 *
 * This function clears the current input line, displays the received message, and
 * re-displays the saved user input (if any).
 *
 * @param[in] message The message received from the server.
 * @param[in] current_input The current input string entered by the user.
 */
void print_message(const char *message, const char *current_input);

/**
 * @brief Handles receiving a message from the server.
 *
 * This function receives messages from the server, checks if the message
 * is different from the current input, and calls `print_message` to display
 * the received message.
 *
 * @param[in] client_fd The file descriptor of the client socket.
 * @param[in,out] current_input The current input string entered by the user.
 */
void handle_receive(int client_fd, char *current_input);

/**
 * @brief Handles sending a message to the server.
 *
 * This function gets user input, cleans it, sends the message to the server,
 * and saves the current input for later re-display if necessary.
 *
 * @param[in] client_fd The file descriptor of the client socket.
 * @param[in,out] current_input The current input string entered by the user.
 */
void handle_send(int client_fd, char *current_input);

#endif // CLIENT_H
