#include <iostream>  
#include <cstring>   
#include <unistd.h>  
#include <sys/socket.h> 
#include <netinet/in.h> 
#include <arpa/inet.h>  
#include <mutex>        
#include <thread>       
#include <vector>       
#include <map>          
#include<unordered_map>
#include <set>          

#define PORT 12345            // Port number the server will listen on
#define BUFFER_SIZE 1024     // Buffer size for communication

std::mutex cout_mutex;       // Mutex to synchronize access to std::cout
std::mutex user_mutex;       // Mutex to synchronize access to active users

// Predefined username and password pairs for authentication
std::map<std::string, std::string> credentials = {
    {"user1", "password1"},
    {"user2", "password2"},
    {"user3", "password3"},
    {"admin", "admin123"}
};

// Map to store connected usernames and their respective threads
std::unordered_map<std::string, int> active_users;
std::unordered_map<std::string, std::set<std::string>> groups;


// Function to handle communication with a single client
void handle_client(int client_socket) {
    char buffer[BUFFER_SIZE] = {0};  // Buffer to store data received from the client

    // Prompt the client for username and password
    const char *auth_request = "Enter username: ";
    send(client_socket, auth_request, strlen(auth_request), 0);

    // Read username from the client
    int valread = read(client_socket, buffer, BUFFER_SIZE);
    if (valread > 0) {
        buffer[valread] = '\0';  // Null-terminate the received string
        std::string input(buffer);

        // Parse username and password
        size_t space_pos = input.find(' ');
        std::string username = input;
        
        memset(buffer, 0, BUFFER_SIZE);
        const char *auth_request2 = "Enter password: ";
        send(client_socket, auth_request2, strlen(auth_request2), 0);
        std::string password;
        valread = read(client_socket, buffer, BUFFER_SIZE);
        if (valread > 0) {
            buffer[valread] = '\0';  // Null-terminate the received string
            password = buffer;
        }
        else{
            const char *failure_message = "Authentication failed. Disconnecting...\n";
            send(client_socket, failure_message, strlen(failure_message), 0);
            close(client_socket);
            return;
        }

        // Authenticate the user
        if (credentials.find(username) != credentials.end() && credentials[username] == password) {
            {
                std::lock_guard<std::mutex> lock(user_mutex);
                if (active_users.find(username) != active_users.end()) {
                    const char *already_connected = "User already connected. Disconnecting...\n";
                    // active_users.erase(username);
                    send(client_socket, already_connected, strlen(already_connected), 0);
                    close(client_socket);
                    return;
                }
                // std::lock_guard<std::mutex> lock(user_mutex);
                active_users[username] = client_socket;
            }

            const char *success_message = "Authentication successful. Welcome!\n";
            send(client_socket, success_message, strlen(success_message), 0);

            {
                std::lock_guard<std::mutex> lock(cout_mutex);
                std::cout << "User " << username << " authenticated successfully." << std::endl;
            }

            // Simulate client interaction (extend as needed)
            const char *interaction_message = "You are now connected to the server.\n";
            send(client_socket, interaction_message, strlen(interaction_message), 0);
            usleep(1000);

            while(true){
                memset(buffer, 0, BUFFER_SIZE);
                int message = read(client_socket, buffer, BUFFER_SIZE);
                if(message>0){
                    buffer[message] = '\0';
                    std::string input(buffer);
                    space_pos = input.find(' ');
                    if(space_pos!=std::string::npos){
                        std::string function = input.substr(0, space_pos);
                        std::string information = input.substr(space_pos + 1);
                        std::string final_message = "[" + username + "]: " + information;
                    
                        // {
                        //     std::lock_guard<std::mutex> lock(cout_mutex);
                        //     std::cout<<final_message<<"\n";
                        // }

                        if(function == "/broadcast"){
                            {
                                std::lock_guard<std::mutex> lock(user_mutex);
                                for(auto &user : active_users){
                                    if(user.first != username){
                                        send(user.second, final_message.c_str(), final_message.size(), 0);
                                    }
                                }
                            }
                        }
                        else if(function == "/msg"){
                            int second_space = information.find(' ');
                            std::string receiver = information.substr(0, second_space);
                            std::string message = information.substr(second_space + 1);
                            final_message = "[" + username + "]: " + message;
                            {
                                std::lock_guard<std::mutex> lock(user_mutex);
                                if(active_users.find(receiver) != active_users.end()){
                                    send(active_users[receiver], final_message.c_str(), final_message.size(), 0);
                                }
                            }
                        }
                        else if(function == "/create_group"){
                            std::string group_name = information;
                            {
                                std::lock_guard<std::mutex> lock(user_mutex);
                                if(groups.find(group_name) == groups.end()){
                                    groups[group_name] = std::set<std::string>();
                                    groups[group_name].insert(username);
                                    std::string message = "Group created.";
                                    send(client_socket, message.c_str(), message.size(), 0);
                                }
                                else{
                                    std::string message = "Group already exists.";
                                    send(client_socket, message.c_str(), message.size(), 0);
                                }
                            }
                        }
                        else if(function == "/join_group"){
                            std::string group_name = information;
                            {
                                std::lock_guard<std::mutex> lock(user_mutex);
                                if(groups.find(group_name) != groups.end()){
                                    groups[group_name].insert(username);
                                    std::string message = "You have joined the group.";
                                    send(client_socket, message.c_str(), message.size(), 0);
                                }
                                else{
                                    std::string message = "Group does not exist.";
                                    send(client_socket, message.c_str(), message.size(), 0);
                                }
                            }
                        }
                        else if(function == "/leave_group"){
                            std::string group_name = information;
                            {
                                std::lock_guard<std::mutex> lock(user_mutex);
                                if(groups.find(group_name) != groups.end()){
                                    if(groups[group_name].find(username) != groups[group_name].end()){
                                        groups[group_name].erase(username);
                                        std::string message = "You have left the group.";
                                        send(client_socket, message.c_str(), message.size(), 0);
                                    }
                                    else{
                                        std::string message = "You are not part of this group.";
                                        send(client_socket, message.c_str(), message.size(), 0);
                                    }
                                }
                                else{
                                    std::string message = "Group does not exist.";
                                    send(client_socket, message.c_str(), message.size(), 0);
                                }
                            }
                        }
                        else if(function == "/msg_group"){
                            int second_space = information.find(' ');
                            std::string group_name = information.substr(0, second_space);
                            std::string message = information.substr(second_space + 1);
                            final_message = "["+ group_name+ "]:" + "[" + username + "]: " + message;
                            {
                                std::lock_guard<std::mutex> lock(user_mutex);
                                if(groups.find(group_name) != groups.end()){
                                    for(auto &user : groups[group_name]){
                                        send(active_users[user], final_message.c_str(), final_message.size(), 0);
                                    }
                                }
                            }
                        }
                        
                    }
                    else{
                        std::string function = input;
                        if(function == "/quit"){
                            break;
                        }
                        else if(function == "/list_groups"){
                            std::string group_list = "Groups: ";
                            {
                                std::lock_guard<std::mutex> lock(user_mutex);
                                for(auto &group : groups){
                                    group_list += group.first + " ";
                                }
                            }
                            send(client_socket, group_list.c_str(), group_list.size(), 0);
                        }
                        else if(function == "/list_users"){
                            std::string user_list = "Users: ";
                            {
                                std::lock_guard<std::mutex> lock(user_mutex);
                                for(auto &user : active_users){
                                    user_list += user.first + " ";
                                }
                            }
                            send(client_socket, user_list.c_str(), user_list.size(), 0);
                        }
                    }
                }
            }

            // Remove user from active_users upon disconnection
            {
                std::lock_guard<std::mutex> lock(user_mutex);
                active_users.erase(username);
            }

            {
                std::lock_guard<std::mutex> lock(cout_mutex);
                std::cout << "User " << username << " disconnected." << std::endl;
            }
        } 
    }
    else {
        const char *failure_message = "Authentication failed. Disconnecting...\n";
        send(client_socket, failure_message, strlen(failure_message), 0);
        close(client_socket);
        return;
    }

    // Close the client socket after communication
    close(client_socket);
}

int main() {
    int server_fd;                     // File descriptor for the server socket
    struct sockaddr_in address;        // Structure to hold server address information
    int opt = 1;                       // Option for setting socket options
    int addrlen = sizeof(address);     // Length of the address structure

    // Create socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket failed");      // Print error message if socket creation fails
        exit(EXIT_FAILURE);
    }

    // Set socket options to reuse address and port
    // if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
    //     perror("Setsockopt failed");
    //     close(server_fd);
    //     exit(EXIT_FAILURE);
    // }

    // Configure server address structure
    address.sin_family = AF_INET;        // Address family: IPv4
    address.sin_addr.s_addr = INADDR_ANY; // Accept connections from any IP address
    address.sin_port = htons(PORT);      // Convert port number to network byte order

    // Bind the socket to the specified address and port
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // Start listening for incoming connections
    if (listen(server_fd, 3) < 0) {
        perror("Listen failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    {
        std::lock_guard<std::mutex> lock(cout_mutex);
        std::cout << "Server is listening on port " << PORT << "..." << std::endl;
    }

    std::vector<std::thread> threads;  // Vector to keep track of client-handling threads

    while (true) {
        int new_socket;  // Socket for the new client connection

        // Accept an incoming connection
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
            perror("Accept failed");
            close(server_fd);
            exit(EXIT_FAILURE);
        }

        {
            std::lock_guard<std::mutex> lock(cout_mutex);
            std::cout << "Connection established with a client." << std::endl;
        }

        // Handle the client in a separate thread
        threads.emplace_back(std::thread(handle_client, new_socket));
    }

    // Join all threads before exiting (optional cleanup)
    for (auto &t : threads) {
        if (t.joinable()) {
            t.join();
        }
    }

    // Close the server socket
    close(server_fd);

    return 0;
}
