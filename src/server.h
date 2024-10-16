/**
 * @file server.h
 * @brief Header file for the chat server application.
 *
 * This file contains the necessary includes, constants, structures,
 * and function declarations for the chat server application.
 */

#ifndef SERVER_H
#define SERVER_H

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

/**
 * @brief Buffer size used for message transmission.
 */
#define BUFFER_SIZE 1024

/**
 * @brief Maximum number of clients that can connect to the server.
 */
#define MAX_CLIENTS 10

/**
 * @brief Structure representing a connected client.
 */
typedef struct
{
    int socket;                 /**< Socket file descriptor of the client. */
    char username[50];           /**< Username of the client. */
    char current_channel[50];    /**< The current channel the client is in. */
    int is_admin;                /**< 1 if the client is an admin, 0 otherwise. */
} client_t;

/**
 * @brief Array to store connected clients.
 */
extern client_t *clients[MAX_CLIENTS];

/**
 * @brief Structure to store connected users.
 */
typedef struct
{
    char username[50];    /**< Username of the connected user. */
    int is_admin;         /**< 1 if the user is an admin, 0 otherwise. */
} ConnectedUser;

/**
 * @brief Array to store connected users.
 */
extern ConnectedUser connected_users[MAX_CLIENTS];

/**
 * @brief Counter for the number of connected users.
 */
extern int user_count;

/**
 * @brief Checks if a user is an admin.
 *
 * @param[in] username The username to check.
 * @return 1 if the user is an admin, 0 otherwise.
 */
int is_admin(const char *username);

/**
 * @brief Cleans the input string by removing newline or carriage return characters.
 *
 * @param[in,out] str The input string to be cleaned.
 */
void clean_input(char *str);

/**
 * @brief Clears the server directory by removing all files.
 */
void clear_server_directory(void);

/**
 * @brief Authenticates a user by checking the username and password in the database.
 *
 * @param[in] username The username to authenticate.
 * @param[in] password The password to authenticate.
 * @return 1 if authentication is successful, 0 otherwise.
 */
int authenticate_user(const char *username, const char *password);

/**
 * @brief Stores a file in the appropriate salon directory on the server.
 *
 * @param[in] salon_name The name of the salon.
 * @param[in] filename The name of the file to store.
 */
void store_file_in_salon(const char *salon_name, const char *filename);

/**
 * @brief Stores a message in the database.
 *
 * @param[in] channel The name of the channel.
 * @param[in] username The username of the sender.
 * @param[in] message The message to store.
 */
void store_message_in_db(const char *channel, const char *username, const char *message);

/**
 * @brief Clears all messages from the database.
 */
void clear_messages_in_db(void);

/**
 * @brief Deletes a salon directory from the server.
 *
 * @param[in] salon_name The name of the salon to delete.
 */
void delete_salon_directory(const char *salon_name);

/**
 * @brief Creates a directory for a new salon.
 *
 * @param[in] salon_name The name of the salon.
 */
void create_salon_directory(const char *salon_name);

/**
 * @brief Checks if a channel exists in the database.
 *
 * @param[in] channel_name The name of the channel to check.
 * @return 1 if the channel exists, 0 otherwise.
 */
int channel_exists(const char *channel_name);

/**
 * @brief Creates a new channel and adds it to the database.
 *
 * @param[in] client The client requesting the channel creation.
 * @param[in] channel_name The name of the channel to create.
 */
void create_channel(client_t *client, const char *channel_name);

/**
 * @brief Lists the users currently in the same channel as the client.
 *
 * @param[in] client The client requesting the list.
 */
void list_users_in_channel(client_t *client);

/**
 * @brief Sends a message to all clients in the specified channel.
 *
 * @param[in] channel The channel name.
 * @param[in] message The message to send.
 * @param[in] sender_socket The socket of the client sending the message.
 */
void send_message_to_channel(const char *channel, const char *message, int sender_socket);

/**
 * @brief Deletes a channel and its related data from the server.
 *
 * @param[in] client The client requesting the channel deletion.
 * @param[in] channel_name The name of the channel to delete.
 */
void delete_channel(client_t *client, const char *channel_name);

/**
 * @brief Lists all channels available on the server.
 *
 * @param[in] socket The socket of the client requesting the list.
 */
void list_channels(int socket);

/**
 * @brief Lists all connected users and their current channels for the admin.
 *
 * @param[in] admin_socket The socket of the admin requesting the list.
 */
void handle_list_admin(int admin_socket);

/**
 * @brief Handles the "exit" command to shut down the server.
 *
 * @param[in] server_fd_ptr Pointer to the server's file descriptor.
 * @return NULL
 */
void *handle_exit_command(void *server_fd_ptr);

/**
 * @brief Notifies the client of their current channel.
 *
 * @param[in] client The client to notify.
 */
void notify_current_channel(client_t *client);

/**
 * @brief Initializes the server directories for existing salons.
 */
void initialize_salon_directories(void);

/**
 * @brief Sends a file to the client from the specified salon.
 *
 * @param[in] client_socket The client's socket.
 * @param[in] salon_name The name of the salon.
 * @param[in] filename The name of the file to send.
 */
void send_file_to_client(int client_socket, const char *salon_name, const char *filename);

/**
 * @brief Receives a file from the client and stores it in the specified salon.
 *
 * @param[in] client_socket The client's socket.
 * @param[in] salon_name The name of the salon.
 * @param[in] filename The name of the file to receive.
 */
void receive_file_from_client(int client_socket, const char *salon_name, const char *filename);

/**
 * @brief Handles communication with a connected client.
 *
 * @param[in] client_socket The client's socket.
 * @param[in] client The client structure representing the connected client.
 */
void handle_client(int client_socket, client_t *client);

#endif // SERVER_H
