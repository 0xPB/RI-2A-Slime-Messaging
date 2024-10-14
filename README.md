
# Slime

## Overview

Slime is a real-time messaging application with file-sharing capabilities and multiple channels. Users can create, join, and manage channels, as well as send files within the channels. This project provides a simple client-server architecture.

## How to Use

### 1. Compiling the Client
To compile the client code, use the following command:
```bash
gcc client.c -o client -lpthread
```

### 2. Compiling the Server
To compile the server code with SQLite support, use this command:
```bash
gcc server.c -o server -lsqlite3 -lpthread
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

### User Commands:

- `join channel_name`  
  Joins the specified `channel_name`.

- `leave`  
  Leaves the current channel.

- `current`  
  Displays the current channel.

- `disconnect`  
  Disconnects the current user from the server.

- `send_files path_to_file1 path_to_file2 ...`  
  Sends one or more files to the current channel.

- `list_channel`  
  Lists all existing channels.

- `list_users`  
  Lists the users in the current channel.

### Admin Commands:

- `create_channel channel_name`  
  Creates a new channel with the specified `channel_name`.

- `delete_channel channel_name`  
  Deletes the channel with the specified `channel_name`.

- `list_admin`  
  Lists all connected users, accessible by administrators.

### Server Commands:

- `shut`  
  Shuts down the server and removes temporary files. This command is executed on the server (`server.c`).

### Summary of User Commands (Non-Administrators):

| Syntax                     | Action                                         |
|-----------------------------|------------------------------------------------|
| `send_files path_to_file1 path_to_file2 ...` | Sends one or more files to the current channel. |
| `current`                   | Notifies the user of the current channel.      |
| `join channel_name`          | Joins the channel named "channel_name".        |
| `leave`                     | Leaves the current channel.                   |
| `list_users`                | Lists the users in the current channel.        |
| `list_channel`              | Lists all channels.                           |
| `disconnect`                | Disconnects the current user.                 |

### Summary of Admin Commands:

| Syntax                     | Action                                         |
|-----------------------------|------------------------------------------------|
| `list_admin`                | Lists all users connected to the server (administrators only). |
| `create_channel channel_name`| Creates a channel named "channel_name".       |
| `delete_channel channel_name`| Deletes the channel named "channel_name".     |
| `current`                   | Notifies the user of the current channel.      |
| `join channel_name`          | Joins the channel named "channel_name".        |
| `leave`                     | Leaves the current channel.                   |
| `list_users`                | Lists the users in the current channel.        |
| `disconnect`                | Disconnects the current user.                 |

### Summary of Server Commands:

| Syntax   | Action                                    |
|----------|-------------------------------------------|
| `shut`   | Shuts down the server and removes temporary files. |

## License

This project is licensed under the GPLv3 License. See the `LICENSE` file for more information.