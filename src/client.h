/**
 * @brief Cleans the input by removing the newline character.
 * 
 * @param str The input string to be cleaned.
 */
void clean_input(char *str);

/**
 * @brief Displays the received message and restores the current user input.
 * 
 * @param message The message to display.
 * @param current_input The current input from the user.
 */
void print_message(const char *message, const char *current_input);

/**
 * @brief Sends a file to the server in packets.
 * 
 * @param socket The socket to send the file through.
 * @param filename The name of the file to be sent.
 */
void send_file(int socket, const char *filename);

/**
 * @brief Receives a file from the server in packets.
 * 
 * @param socket The socket to receive the file through.
 * @param filename The name of the file to be received.
 */
void receive_file_from_server(int socket, const char *filename);

/**
 * @brief Thread function to continuously receive messages from the server.
 * 
 * @param client_socket Pointer to the client socket.
 * @return void* 
 */
void *receive_messages(void *client_socket);

/**
 * @brief Thread function to send messages and handle user input.
 * 
 * @param client_socket Pointer to the client socket.
 * @return void* 
 */
void *send_messages(void *client_socket);

/**
 * @brief Main function to initialize the client, authenticate, and manage threads.
 * 
 * @return int 
 */
int main();
