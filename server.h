/**
 * @file server.h
 * @brief Header file for the server-side of the messaging application.
 * 
 * This file contains the includes, macro definitions, structure definitions, 
 * and function prototypes for managing clients, file transfers, and 
 * database operations on the server.
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

#define BUFFER_SIZE 1024  /**< Buffer size for communication */
#define MAX_CLIENTS 10    /**< Maximum number of clients that can connect */

/**
 * @brief Structure representing a client.
 * 
 * This structure holds the information related to a connected client, such 
 * as their socket, username, current chat channel, and whether they have 
 * administrative privileges.
 */
typedef struct
{
    int socket;               /**< Client socket descriptor */
    char username[50];         /**< Username of the client */
    char current_channel[50];  /**< Current chat channel the client has joined */
    int is_admin;              /**< 1 if the client is an admin, 0 otherwise */
} client_t;

/** Array of client pointers to store connected clients. */
client_t *clients[MAX_CLIENTS];

/**
 * @brief Checks if a user is an administrator.
 * 
 * This function queries the database to check whether a user has the 'admin' role.
 * 
 * @param[in] username The username to check.
 * @return 1 if the user is an admin, 0 otherwise.
 */
int is_admin(const char *username);

/**
 * @brief Cleans the input by removing newline or carriage return characters.
 * 
 * This function removes any newline (`\n`) or carriage return (`\r`) characters 
 * from the input string and replaces them with a null terminator (`\0`).
 * 
 * @param[in,out] str The input string to clean.
 */
void clean_input(char *str);

/**
 * @brief Clears the server directory by removing all files and directories within it.
 */
void clear_server_directory(void);

/**
 * @brief Authenticates a user by checking their username and password in the database.
 * 
 * This function queries the database to check if the provided username and password are correct.
 * 
 * @param[in] username The username to authenticate.
 * @param[in] password The password to authenticate.
 * @return 1 if authentication is successful, 0 otherwise.
 */
int authenticate_user(const char *username, const char *password);

/**
 * @brief Stores a file in the specified chat channel directory.
 * 
 * This function copies a file into the directory of the specified chat channel.
 * 
 * @param[in] salon_name The name of the chat channel.
 * @param[in] filename The name of the file to store.
 */
void store_file_in_salon(const char *salon_name, const char *filename);

/**
 * @brief Stores a message in the database.
 * 
 * This function inserts a message into the database, associating it with the specified chat channel and username.
 * 
 * @param[in] channel The chat channel where the message was sent.
 * @param[in] username The username of the sender.
 * @param[in] message The message content.
 */
void store_message_in_db(const char *channel, const char *username, const char *message);

/**
 * @brief Clears all messages from the database.
 */
void clear_messages_in_db(void);

/**
 * @brief Deletes a chat channel directory.
 * 
 * This function deletes the directory of the specified chat channel.
 * 
 * @param[in] salon_name The name of the chat channel.
 */
void delete_salon_directory(const char *salon_name);

/**
 * @brief Creates a directory for a chat channel.
 * 
 * This function creates a directory for the specified chat channel if it doesn't exist.
 * 
 * @param[in] salon_name The name of the chat channel.
 */
void create_salon_directory(const char *salon_name);

/**
 * @brief Checks if a chat channel exists in the database.
 * 
 * This function queries the database to check if the specified chat channel exists.
 * 
 * @param[in] channel_name The name of the chat channel to check.
 * @return 1 if the chat channel exists, 0 otherwise.
 */
int channel_exists(const char *channel_name);

/**
 * @brief Creates a new chat channel.
 * 
 * This function creates a new chat channel if the user is an administrator and the channel doesn't already exist.
 * 
 * @param[in] client The client requesting the channel creation.
 * @param[in] channel_name The name of the chat channel to create.
 */
void create_channel(client_t *client, const char *channel_name);

/**
 * @brief Lists the users in the client's current chat channel.
 * 
 * This function sends a list of users currently connected to the same chat channel as the client.
 * 
 * @param[in] client The client requesting the user list.
 */
void list_users_in_channel(client_t *client);

/**
 * @brief Sends a message to all users in the specified chat channel.
 * 
 * This function broadcasts a message to all users in the same chat channel, except the sender.
 * 
 * @param[in] channel The chat channel to which the message is sent.
 * @param[in] message The message content.
 * @param[in] sender_socket The socket of the client sending the message.
 */
void send_message_to_channel(const char *channel, const char *message, int sender_socket);

/**
 * @brief Deletes a chat channel and its messages.
 * 
 * This function deletes a chat channel from the database, including all associated messages, and notifies users.
 * 
 * @param[in] client The client requesting the deletion.
 * @param[in] channel_name The name of the chat channel to delete.
 */
void delete_channel(client_t *client, const char *channel_name);

/**
 * @brief Lists all available chat channels.
 * 
 * This function sends a list of all available chat channels to the client.
 * 
 * @param[in] socket The socket of the client requesting the channel list.
 */
void list_channels(int socket);

/**
 * @brief Sends a list of all connected users and their chat channels to an administrator.
 * 
 * This function sends the list of all users, along with their chat channel, to an admin client.
 * 
 * @param[in] admin_socket The socket of the admin client.
 */
void handle_list_admin(int admin_socket);

/**
 * @brief Notifies the client of their current chat channel.
 * 
 * This function informs the client of the chat channel they are currently in.
 * 
 * @param[in] client The client to notify.
 */
void notify_current_channel(client_t *client);

/**
 * @brief Initializes directories for all existing chat channels.
 * 
 * This function creates directories for all chat channels found in the database.
 */
void initialize_salon_directories(void);

/**
 * @brief Sends a file to a client in the specified chat channel.
 * 
 * This function transfers a file from the server to a client in the given chat channel.
 * 
 * @param[in] client_socket The socket of the client requesting the file.
 * @param[in] salon_name The chat channel to which the file belongs.
 * @param[in] filename The name of the file to send.
 */
void send_file_to_client(int client_socket, const char *salon_name, const char *filename);

/**
 * @brief Receives a file from a client and stores it in the server's directory for the specified chat channel.
 * 
 * This function receives a file from the client and saves it in the server's directory for the specified chat channel.
 * 
 * @param[in] client_socket The socket of the client sending the file.
 * @param[in] salon_name The chat channel to which the file belongs.
 * @param[in] filename The name of the file being received.
 */
void receive_file_from_client(int client_socket, const char *salon_name, const char *filename);

/**
 * @brief Handles communication with a connected client.
 * 
 * This function processes incoming messages and commands from a client, 
 * such as joining a channel, sending messages, and file transfers.
 * 
 * @param[in] client_socket The socket of the connected client.
 * @param[in] client The client data structure.
 */
void handle_client(int client_socket, client_t *client);
