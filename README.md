
# Slime

## Overview

Slime is a real-time messaging application with file-sharing capabilities and multiple channels. Users can create, join, and manage channels, as well as send files within the channels. This project provides a simple client-server architecture.

## How to Use

### 1. Compiling the Client
To compile the client code, use the following command:
```bash
gcc client.c -o client
```

### 2. Compiling the Server
To compile the server code with SQLite support, use this command:
```bash
gcc server.c -o server -lsqlite3
```

### 3. Launching the Server
Start the server by running:
```bash
./server
```

### 4. Launching Clients
You can launch as many clients as you need with the following command:
```bash
./client
```

## Commands

Below is a list of available commands for interacting with the application:

- `send_files path_to_file1 path_to_file2 ...`  
  Sends one or more files to the current channel.

- `create_channel channel_name`  
  Creates a new channel with the specified `channel_name`.

- `delete_channel channel_name`  
  Deletes the channel with the specified `channel_name`.

- `list_channel`  
  Lists all existing channels.

- `join_channel channel_name`  
  Joins the specified `channel_name`.

- `leave_space channel_name`  
  Leaves the specified `channel_name`.

- `disconnect`  
  Disconnects the current user from the server.

## License

This project is licensed under the GPLv3 License. See the `LICENSE` file for more information.
