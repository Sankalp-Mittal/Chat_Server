# Chat Server

## Overview
This is a simple multi-client chat server that allows users to communicate through private messages, group messages, and broadcasts. The server is implemented in C++ using POSIX sockets and supports authentication via a `users.txt` file. It also provides an `/exit` command for shutting down the server.

## Features
- User authentication based on `users.txt`
- Private messaging between users
- Group chat functionality
- Broadcast messaging to all connected users
- Graceful shutdown when no clients are connected

## Compilation and Execution

### Prerequisites
- A C++ compiler (g++)
- make
- Linux or macOS environment

### Compiling the Server
To compile the server, use the following command:
```bash
make
```

### Running the Server
To start the server, execute:
```bash
 ./server_grp
```
The server will listen on port `12345` by default.

### Server Shutdown
To shut down the server, enter the command:
```bash
 /exit
```
The server will only shut down if no clients are connected.

## How the Code Works
### Authentication
- Users must log in with a username and password stored in `users.txt`.
- Format of `users.txt`:
  ```
  user1:password1
  user2:password2
  ```
- If a user logs in with incorrect credentials, the connection is closed.

### Client Interaction
- The server continuously listens for new connections.
- Each client runs in a separate thread to handle messages.
- Clients can send commands prefixed with `/`:
  - `/broadcast <message>`: Sends a message to all clients.
  - `/msg <username> <message>`: Sends a private message.
  - `/create_group <group_name>`: Creates a new chat group.
  - `/join_group <group_name>`: Joins an existing group.
  - `/leave_group <group_name>`: Leaves a group.
  - `/group_msg <group_name> <message>`: Sends a message to a group.
  - `/exit`: Disconnects from the server.

### Multi-threading
- The server handles each client in a separate thread.
- Mutex locks are used to ensure thread safety when accessing shared data.

### Graceful Shutdown
- The `/exit` command ensures that the server only shuts down when no clients are connected.
- The server closes all client connections before terminating.

## Notes
- Ensure `users.txt` exists before running the server.
- Run the server in a terminal session to see logs of user activities.
- The server uses `mutex` to prevent data races when handling multiple clients.

