
#ifndef SERVER_H
#define SERVER_H

/**
 * @file server.h
 * @brief Header file for the server-side functionalities of the messaging application.
 *
 * This file contains the declarations for the functions used in the server-side of the messaging application.
 * It includes necessary libraries and defines important constants used throughout the server.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <poll.h>
#include <sqlite3.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

/** Buffer size for message handling */
#define BUFFER_SIZE 1024

/** Maximum number of clients that can connect to the server */
#define MAX_CLIENTS 10

/** Structure to hold client information */
typedef struct
{
    int socket;                  /**< Client socket descriptor */
    char username[50];          /**< Username of the client */
    char current_channel[50];   /**< Current channel the client is in */
    int is_admin;               /**< Admin status: 1 for admin, 0 for regular user */
} client_t;

/**
 * @brief Check if the user is an admin.
 *
 * @param username The username to check.
 * @return int Returns 1 if the user is an admin, 0 otherwise.
 */
int is_admin(const char *username);

/**
 * @brief Clean input string by removing newline characters.
 *
 * @param str The string to clean.
 */
void clean_input(char *str);

/**
 * @brief Clear the server directory by removing all files.
 */
void clear_server_directory();

/**
 * @brief Authenticate a user by username and password.
 *
 * @param username The username of the user.
 * @param password The password of the user.
 * @return int Returns 1 on successful authentication, 0 otherwise.
 */
int authenticate_user(const char *username, const char *password);

/**
 * @brief Store a file in the specified salon.
 *
 * @param salon_name The name of the salon where the file will be stored.
 * @param filename The name of the file to store.
 */
void store_file_in_salon(const char *salon_name, const char *filename);

/**
 * @brief Store a message in the database associated with the specified channel.
 *
 * @param channel The channel where the message belongs.
 * @param username The username of the message sender.
 * @param message The message to store.
 */
void store_message_in_db(const char *channel, const char *username, const char *message);

/**
 * @brief Clear all messages from the database.
 */
void clear_messages_in_db();

/**
 * @brief Delete the directory of the specified salon.
 *
 * @param salon_name The name of the salon to delete.
 */
void delete_salon_directory(const char *salon_name);

/**
 * @brief Create a directory for the specified salon if it does not exist.
 *
 * @param salon_name The name of the salon to create a directory for.
 */
void create_salon_directory(const char *salon_name);

/**
 * @brief Check if the specified channel exists.
 *
 * @param channel_name The name of the channel to check.
 * @return int Returns 1 if the channel exists, 0 otherwise.
 */
int channel_exists(const char *channel_name);

/**
 * @brief Create a new channel if the user is an admin and it does not already exist.
 *
 * @param client The client requesting the channel creation.
 * @param channel_name The name of the new channel.
 */
void create_channel(client_t *client, const char *channel_name);

/**
 * @brief List all users currently in the same channel as the client.
 *
 * @param client The client requesting the user list.
 */
void list_users_in_channel(client_t *client);

/**
 * @brief Send a message to all users in the specified channel.
 *
 * @param channel The channel to send the message to.
 * @param message The message to send.
 * @param sender_socket The socket of the user sending the message.
 */
void send_message_to_channel(const char *channel, const char *message, int sender_socket);

/**
 * @brief Delete a specified channel if the user is an admin.
 *
 * @param client The client requesting the channel deletion.
 * @param channel_name The name of the channel to delete.
 */
void delete_channel(client_t *client, const char *channel_name);

/**
 * @brief List all available channels to the specified socket.
 *
 * @param socket The socket to send the list of channels to.
 */
void list_channels(int socket);

/**
 * @brief Handle the request to list all users and their channels for an admin.
 *
 * @param admin_socket The socket of the admin requesting the user list.
 */
void handle_list_admin(int admin_socket);

/**
 * @brief Notify the client of their current channel.
 *
 * @param client The client whose current channel is to be notified.
 */
void notify_current_channel(client_t *client);

/**
 * @brief Initialize directories for existing salons based on the database.
 */
void initialize_salon_directories();

/**
 * @brief Send a file to a client from a specified salon.
 *
 * @param client_socket The socket of the client receiving the file.
 * @param salon_name The name of the salon where the file is located.
 * @param filename The name of the file to send.
 */
void send_file_to_client(int client_socket, const char *salon_name, const char *filename);

/**
 * @brief Receive a file from a client and store it in the specified salon.
 *
 * @param client_socket The socket of the client sending the file.
 * @param salon_name The name of the salon where the file will be stored.
 * @param filename The name of the file being sent.
 */
void receive_file_from_client(int client_socket, const char *salon_name, const char *filename);

/**
 * @brief Handle communication with a connected client.
 *
 * @param client_socket The socket of the client to handle.
 * @param client The client structure associated with this socket.
 */
void handle_client(int client_socket, client_t *client);

#endif // SERVER_H
