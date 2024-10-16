
# 🟢 Slime

## Overview

Slime is a real-time messaging application with file-sharing capabilities and multiple channels. Users can create, join, and manage channels, as well as send files within the channels. This project provides a simple client-server architecture.

## 🚀 How to Use

### 1. 🔨 Compiling the Client

To compile the client code, you have two options:

Using `gcc`:
```bash
gcc client.c -o client.exe
```

Note: the client uses by default `localhost (127.0.0.1)`; you can change this to another IP by modifying `line 222` in `client.c`.

Or using the `make` command:
```bash
make client
```

### 2. 🔨 Compiling the Server

To compile the server code with SQLite support, you can also choose between:

Using `gcc`:
```bash
gcc server.c -o server.exe -lsqlite3
```

Or using the `make` command:
```bash
make server
```

### 3. ▶️ Launching the Server

Start the server by running:
```bash
./server.exe
```

### 4. ▶️ Launching Clients

You can launch as many clients as you need with the following command:
```bash
./client.exe
```

### 5. 🧹 Cleaning Up

You can clean up all generated binaries (client and server) using:
```bash
make clean
```

### 6. 🔄 Additional Makefile Commands

- `make all`  
  Compiles the client, server, documentation, and creates the `server` directory.

- `make clean`  
  Removes all compiled binaries, documentation, and the `server` directory.

- `make docs`  
  Generates the documentation with Doxygen; open `index.html` located in `docs/html/` to read the documentation.

- `make server_dir`  
  Creates the `server` directory.

## 📝 Commands

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

- `list`  
  Lists all existing channels.

- `list_users`  
  Lists the users in the current channel.

- `help`  
  Displays all available commands.

- `send filename`  
  Sends a file named `filename` in the current channel.

- `receive filename`  
  Receives a file in the current channel. `filename` is the name of the file you want to receive.

### Admin Commands:

- `create channel_name`  
  Creates a new channel with the specified `channel_name`.

- `delete channel_name`  
  Deletes the channel with the specified `channel_name`.

- `list_admin`  
  Lists all connected users, accessible only to administrators.

### Server Commands:

- `shut`  
  Shuts down the server and removes temporary files. This command is executed on the server (`server.c`).

## 📜 License

This project is licensed under the GPLv3 License. See the `LICENSE` file for more information.
