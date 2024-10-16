/**
 * @file client.h
 * @brief Header file for the client-side messaging application.
 * 
 * This file contains the necessary includes, macro definitions, and function 
 * prototypes required for the messaging client to interact with a server. It 
 * provides functionalities for sending and receiving messages, handling file 
 * transfers, and user authentication.
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

#define BUFFER_SIZE 1024  /**< Buffer size for sending/receiving data */

/** 
 * @brief Stores the current chat channel. 
 */
char current_channel[50] = ""; 

/**
 * @brief Cleans the input by removing newline or carriage return characters.
 * 
 * This function looks for newline (`\n`) or carriage return (`\r`) characters 
 * in the input string and replaces them with a null terminator (`\0`).
 * 
 * @param[in,out] str The input string to be cleaned.
 */
void clean_input(char *str);

/**
 * @brief Displays a message from the server, preserving the current user input.
 * 
 * This function clears the current line and displays a message received from 
 * the server. It then re-displays the user's current input or a prompt.
 * 
 * @param[in] message The message received from the server.
 * @param[in] current_input The user's current input string.
 */
void print_message(const char *message, const char *current_input);

/**
 * @brief Receives a file from the server and saves it locally.
 * 
 * This function handles the reception of a file from the server. It first 
 * receives the file size, sends a confirmation, and then writes the incoming 
 * file data to a local file.
 * 
 * @param[in] client_socket The socket connected to the server.
 * @param[in] filename The name of the file to save locally.
 */
void receive_file_from_server(int client_socket, const char *filename);

/**
 * @brief Sends a file from the client to the server.
 * 
 * This function handles the transfer of a file from the client to the server. 
 * It first sends the file size, waits for confirmation, and then sends the 
 * file bit by bit.
 * 
 * @param[in] client_socket The socket connected to the server.
 * @param[in] filename The name of the file to be sent to the server.
 */
void send_file_to_server(int client_socket, const char *filename);

/**
 * @brief Handles the reception of data from the server.
 * 
 * This function monitors the socket for incoming messages from the server. It 
 * displays the message if it differs from the user's current input.
 * 
 * @param[in] client_fd The file descriptor of the client socket.
 * @param[in] current_input The current input entered by the user.
 */
void handle_receive(int client_fd, char *current_input);

/**
 * @brief Handles the sending of data from the client to the server.
 * 
 * This function reads user input, sends it to the server, and processes 
 * specific commands such as sending or receiving files, or displaying help 
 * information.
 * 
 * @param[in] client_fd The file descriptor of the client socket.
 * @param[in] current_input The current input entered by the user.
 */
void handle_send(int client_fd, char *current_input);
