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

#define BUFFER_SIZE 1024
#define MAX_CLIENTS 10

/** 
 * @brief Structure representing a connected client.
 */
typedef struct
{
    int socket;                    /**< The socket descriptor for the client */
    char username[50];             /**< The username of the client */
    char current_channel[50];      /**< The current channel the client is in */
    int is_admin;                  /**< 1 if the client is an admin, 0 if a regular user */
} client_t;

client_t *clients[MAX_CLIENTS]; /**< Array of connected clients */

/** 
 * @brief Structure for storing connected users.
 */
typedef struct
{
    char username[50];             /**< The username of the connected user */
    int is_admin;                  /**< 1 if the user is an admin, 0 if a regular user */
} ConnectedUser;

ConnectedUser connected_users[MAX_CLIENTS]; /**< Array to store connected users */
int user_count = 0;                          /**< Count of connected users */

/**
 * @brief Checks if a user is an admin.
 *
 * @param username The username to check.
 * @return Returns 1 if the user is an admin, 0 otherwise.
 */
int is_admin(const char *username);

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
 * @brief Clears the server's directory by removing all files.
 *
 * This function uses the system command to remove all files in the server directory.
 */
void clear_server_directory();

/**
 * @brief Authenticates a user based on username and password.
 *
 * This function checks the provided username and password against the database
 * to authenticate the user.
 *
 * @param username The username of the user.
 * @param password The password of the user.
 * @return Returns 1 if authentication is successful, 0 otherwise.
 */
int authenticate_user(const char *username, const char *password);

/**
 * @brief Stores a file in the specified channel directory.
 *
 * @param salon_name The name of the channel where the file will be stored.
 * @param filename The name of the file to be stored.
 */
void store_file_in_salon(const char *salon_name, const char *filename);

/**
 * @brief Stores a message in the database.
 *
 * @param channel The name of the channel where the message was sent.
 * @param username The username of the sender.
 * @param message The message to store.
 */
void store_message_in_db(const char *channel, const char *username, const char *message);

/**
 * @brief Clears all messages from the database.
 *
 * This function deletes all records from the messages table in the database.
 */
void clear_messages_in_db();

/**
 * @brief Deletes the directory of the specified channel.
 *
 * @param salon_name The name of the channel whose directory will be deleted.
 */
void delete_salon_directory(const char *salon_name);

/**
 * @brief Creates a directory for the specified channel.
 *
 * @param salon_name The name of the channel for which the directory will be created.
 */
void create_salon_directory(const char *salon_name);

/**
 * @brief Checks if a channel exists in the database.
 *
 * @param channel_name The name of the channel to check.
 * @return Returns 1 if the channel exists, 0 otherwise.
 */
int channel_exists(const char *channel_name);

/**
 * @brief Creates a channel and notifies the users.
 *
 * @param client Pointer to the client requesting the channel creation.
 * @param channel_name The name of the channel to create.
 */
void create_channel(client_t *client, const char *channel_name);

/**
 * @brief Lists the users in the current channel for the specified client.
 *
 * @param client Pointer to the client requesting the user list.
 */
void list_users_in_channel(client_t *client);

/**
 * @brief Sends a message to all users in the specified channel.
 *
 * @param channel The name of the channel.
 * @param message The message to be sent.
 * @param sender_socket The socket of the user sending the message.
 */
void send_message_to_channel(const char *channel, const char *message, int sender_socket);

/**
 * @brief Deletes a channel and notifies all users in that channel.
 *
 * @param client Pointer to the client requesting the channel deletion.
 * @param channel_name The name of the channel to delete.
 */
void delete_channel(client_t *client, const char *channel_name);

/**
 * @brief Lists all channels in the database and sends them to the client.
 *
 * @param socket The socket of the client to send the channel list.
 */
void list_channels(int socket);

/**
 * @brief Handles the request of the admin to list all connected users.
 *
 * @param admin_socket The socket of the admin requesting the user list.
 */
void handle_list_admin(int admin_socket);

/**
 * @brief Notifies the current channel to the specified client.
 *
 * @param client Pointer to the client whose current channel is being notified.
 */
void notify_current_channel(client_t *client);

/**
 * @brief Initializes the directories for existing channels.
 *
 * This function creates directories for all channels that exist in the database.
 */
void initialize_salon_directories();

/**
 * @brief Sends a specified file to a client.
 *
 * @param client_socket The socket of the client to send the file.
 * @param salon_name The name of the channel where the file is stored.
 * @param filename The name of the file to be sent.
 */
void send_file_to_client(int client_socket, const char *salon_name, const char *filename);

/**
 * @brief Receives a file from a client and stores it in the specified channel.
 *
 * @param client_socket The socket of the client sending the file.
 * @param salon_name The name of the channel where the file will be stored.
 * @param filename The name of the file being sent.
 */
void receive_file_from_client(int client_socket, const char *salon_name, const char *filename);

/**
 * @brief Handles communication with a specific client.
 *
 * This function processes incoming messages from the client and executes commands
 * based on the received input.
 *
 * @param client_socket The socket of the client.
 * @param client Pointer to the client structure.
 */
void handle_client(int client_socket, client_t *client);

/**
 * @brief Main function to start the server.
 *
 * This function initializes the server, accepts incoming connections,
 * and manages communication with clients.
 *
 * @return Returns 0 on successful execution, or exits on failure.
 */
int main();

#endif // SERVER_H
