# Assignment README

## Features

### Implemented Features
1. **Authentication**: Users can log in with a username and password.
2. **Broadcast Messaging**: Users can send messages to all connected clients.
3. **Private Messaging**: Users can send direct messages to specific clients.
4. **Group Management**:
   - Create groups.
   - Join and leave groups.
   - Send group messages.
5. **Server Shutdown**: Admin can shut down the server with an `/exit` command.

### Not Implemented Features
- Automatic user registration.
- Handling message history or persistent data beyond the current session.

## Design Decisions

### Multi-threaded Handling of Clients
- The server spawns a new thread (`handle_client`) for each client connection to handle client-server communication concurrently.
- **Reason**: Threads allow multiple clients to communicate with the server simultaneously, providing better scalability.

### Mutex Synchronization
- Mutexes (`cout_mutex`, `user_mutex`, `group_mutex`) are used to ensure thread-safe operations on shared data structures.
- **Reason**: Prevents data races and inconsistencies when multiple threads access or modify shared resources.

### Atomic Shutdown Flag
- An atomic boolean (`shutdown_server`) indicates when the server should stop accepting new connections.
- **Reason**: Atomics ensure safe updates across multiple threads without complex locking.

### Non-blocking Accept
- The server uses non-blocking socket operations to avoid indefinitely waiting for connections when the shutdown flag is set.
- **Reason**: Enhances responsiveness during server shutdown.

## Implementation

### High-Level Overview

#### Important Functions
1. **Server Functions**
   - `main()`: Initializes the server, loads user data, and starts the listener loop.
   - `handle_client()`: Handles authentication and command processing for a connected client.
   - `broadcast_message()`: Sends a message to all clients except the sender.
   - `create_group()`: Creates a new group and notifies users.
   - `group_message()`: Sends messages within a group.
   - `listen_for_exit_command()`: Monitors the console for the `/exit` command.

2. **Client Functions**
   - `handle_server_messages()`: Continuously receives and prints server messages.
   - `main()`: Connects to the server and manages user input.

### Code Flow
- **Server:**
  1. Start the server and load user credentials.
  2. Listen for new connections and spawn a thread for each client.
  3. Handle client commands and messages.
  4. Shut down upon receiving the `/exit` command.

- **Client:**
  1. Connect to the server.
  2. Authenticate with a username and password.
  3. Continuously handle server messages and send user input.

## Testing

### Correctness Testing
- Tested individual features by connecting multiple clients and verifying message delivery.
- Ensured proper handling of authentication errors.

### Stress Testing
- Simulated multiple concurrent client connections to test server stability and message handling.
- Checked for performance degradation and deadlocks.

## Restrictions

- **Maximum Clients:** No fixed limit; depends on system resources.
- **Maximum Groups:** No fixed limit; dynamically managed.
- **Maximum Group Members:** No fixed limit; determined by server performance.
- **Maximum Message Size:** 1024 bytes.

## Challenges

1. **Thread Synchronization Issues**: Faced occasional deadlocks when multiple threads accessed shared resources.
   - **Solution**: Introduced fine-grained locks for specific operations.

2. **Socket Errors**: Errors during client disconnection led to resource leaks.
   - **Solution**: Added error handling and resource cleanup.

3. **Authentication Logic**: Difficulty in ensuring unique logins.
   - **Solution**: Checked for duplicate usernames before granting access.

## Contribution of Each Member

| Member Name       | Contribution Description                         | Percentage |
|-------------------|--------------------------------------------------|------------|
| Member A          | Server design and implementation                 | 40%        |
| Member B          | Client design and implementation                 | 30%        |
| Member C          | Testing and bug fixes                            | 20%        |
| Member D          | Documentation and README preparation             | 10%        |

## Sources Referred
- C++ documentation.
- Online tutorials on socket programming.
- Blogs on multi-threaded server design.

## Declaration
We declare that the assignment was completed independently and no instances of plagiarism occurred.

## Feedback
- The assignment was well-structured, but clearer requirements on message and group management would have been helpful.
- Testing with edge cases was challenging due to a lack of predefined test cases.