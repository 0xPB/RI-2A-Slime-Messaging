/**
 * @file server.c
 * @brief Chat server implementation.
 * 
 * This file implements a multi-threaded chat server with user authentication,
 * channel management, file sending and receiving capabilities.
 */

/**
 * @struct client_t
 * @brief Structure to hold client information.
 * 
 * This structure stores the socket descriptor, username, and current channel of the client.
 */
typedef struct
{
    int socket;              ///< Client's socket descriptor.
    char username[50];       ///< Client's username.
    char current_channel[50]; ///< Current channel the client is in.
} client_t;

/**
 * @struct ConnectedUser
 * @brief Structure to hold information about connected users.
 * 
 * This structure keeps track of the username and admin status of connected users.
 */
typedef struct
{
    char username[50]; ///< Username of the connected user.
    int is_admin;      ///< Admin status (1 for admin, 0 for regular user).
} ConnectedUser;

/**
 * @brief Check if a user is an admin.
 * 
 * This function queries the database to determine if the specified username has admin privileges.
 * 
 * @param username The username to check.
 * @return 1 if the user is an admin, 0 otherwise.
 */
int is_admin(const char *username);

/**
 * @brief Clean the input string by removing the newline character.
 * 
 * This function replaces any newline characters in the input string with a null terminator.
 * 
 * @param str The input string to clean.
 */
void clean_input(char *str);

/**
 * @brief Authenticate a user based on username and password.
 * 
 * This function checks the provided username and password against the database.
 * 
 * @param username The username to authenticate.
 * @param password The password to authenticate.
 * @return 1 if authentication is successful, 0 otherwise.
 */
int authenticate_user(const char *username, const char *password);

/**
 * @brief Store a file in the specified channel's directory.
 * 
 * This function creates a directory for the channel if it does not exist and stores the file there.
 * 
 * @param salon_name The name of the channel.
 * @param filename The name of the file to store.
 */
void store_file_in_salon(const char *salon_name, const char *filename);

/**
 * @brief Store a message in the database.
 * 
 * This function inserts a message into the database for the specified channel.
 * 
 * @param channel The channel where the message is sent.
 * @param username The username of the sender.
 * @param message The message content.
 */
void store_message_in_db(const char *channel, const char *username, const char *message);

/**
 * @brief Clear all messages from the database.
 * 
 * This function deletes all messages from the messages table in the database.
 */
void clear_messages_in_db();

/**
 * @brief Notify the client of their current channel.
 * 
 * This function sends a message to the client indicating the channel they are currently in.
 * 
 * @param client The client to notify.
 */
void notify_current_channel(client_t *client);

/**
 * @brief Check if a channel exists in the database.
 * 
 * This function queries the database to check if a channel with the given name exists.
 * 
 * @param channel_name The name of the channel to check.
 * @return 1 if the channel exists, 0 otherwise.
 */
int channel_exists(const char *channel_name);

/**
 * @brief Create a directory for a specified channel.
 * 
 * This function creates a directory for the channel if it does not already exist.
 * 
 * @param salon_name The name of the channel to create a directory for.
 */
void create_salon_directory(const char *salon_name);

/**
 * @brief Initialize directories for all channels stored in the database.
 * 
 * This function creates directories for all existing channels in the database.
 */
void initialize_salon_directories();

/**
 * @brief Create a new channel.
 * 
 * This function allows an admin to create a new channel if it does not already exist.
 * 
 * @param client The client requesting the channel creation.
 * @param channel_name The name of the new channel.
 */
void create_channel(client_t *client, const char *channel_name);

/**
 * @brief Delete the directory of a specified channel.
 * 
 * This function removes the directory associated with the specified channel.
 * 
 * @param salon_name The name of the channel whose directory is to be deleted.
 */
void delete_salon_directory(const char *salon_name);

/**
 * @brief Delete a channel.
 * 
 * This function allows an admin to delete a channel, removing its messages and notifying users.
 * 
 * @param client The client requesting the channel deletion.
 * @param channel_name The name of the channel to delete.
 */
void delete_channel(client_t *client, const char *channel_name);

/**
 * @brief Receive a file from a client and store it in the specified channel.
 * 
 * This function receives a file over the network and saves it in the appropriate channel's directory.
 * 
 * @param client_socket The socket of the client sending the file.
 * @param salon_name The name of the channel where the file is to be stored.
 * @param filename The name of the file being received.
 */
void receive_file(int client_socket, const char *salon_name, const char *filename);

/**
 * @brief List all channels for a client.
 * 
 * This function retrieves the list of all channels from the database and sends it to the specified client.
 * 
 * @param socket The socket of the client to send the channel list to.
 */
void list_channels(int socket);

/**
 * @brief List all users in the current channel for a client.
 * 
 * This function sends the list of users currently in the same channel as the requesting client.
 * 
 * @param client The client requesting the user list.
 */
void list_users_in_channel(client_t *client);

/**
 * @brief Handle the command to list connected users for an admin.
 * 
 * This function retrieves and sends the list of all connected users to the requesting admin client.
 * 
 * @param admin_socket The socket of the admin client.
 */
void handle_list_admin(int admin_socket);

/**
 * @brief Handle the exit command for the server.
 * 
 * This function allows the server to perform cleanup actions and shut down when the exit command is issued.
 * 
 * @param server_fd_ptr Pointer to the server socket file descriptor.
 * @return NULL.
 */
void *handle_exit_command(void *server_fd_ptr);

/**
 * @brief Send a file to a client.
 * 
 * This function reads a file from the server's directory and sends it to the specified client.
 * 
 * @param client_socket The socket of the client receiving the file.
 * @param salon_name The name of the channel from which to retrieve the file.
 * @param filename The name of the file to send.
 */
void send_file_to_client(int client_socket, const char *salon_name, const char *filename);

/**
 * @brief Handle communication with a connected client.
 * 
 * This function manages the interaction with a client, processing commands and maintaining the client's state.
 * 
 * @param arg Pointer to the client socket.
 * @return NULL.
 */
void *handle_client(void *arg);

/**
 * @brief Main function to initialize and run the chat server.
 * 
 * This function sets up the server, accepts incoming connections, and spawns threads to handle clients.
 * 
 * @return 0 on success, or an error code on failure.
 */
int main();
