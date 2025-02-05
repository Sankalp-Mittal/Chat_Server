#include <iostream>  
#include <cstring>   
#include <unistd.h>  
#include <sys/socket.h> 
#include <netinet/in.h> 
#include <arpa/inet.h>  
#include <fcntl.h>
#include <mutex>        
#include <thread>  
#include<atomic>     
#include <vector>       
#include <map>          
#include <unordered_map>
#include <set>     
#include <unordered_set>
#include <fstream>
#include <sstream>

#define PORT 12345
#define BUFFER_SIZE 1024

using namespace std;

mutex cout_mutex;
mutex user_mutex;
mutex group_mutex;

unordered_map <int, string> clients; // Client socket -> username
unordered_map <string,int> user_socket; // Username -> client socket
unordered_map <string, string> users; // Username -> password
unordered_map <string, unordered_set <int> > groups; // Group -> client sockets

atomic<bool> shutdown_server(false);

void listen_for_exit_command() {
    string input;
    while (true) {
        cin >> input;
        if (input == "/exit") {
            if(clients.size()!=0){
                {
                    lock_guard<mutex> lock(cout_mutex);
                    std::cout<<"Error: There are still clients connected. Please disconnect all clients before shutting down the server.\n";
                    continue;
                }
            }
            
            shutdown_server = true; // Set the shutdown flag
            break;
        }
    }
}

void broadcast_message(const string &message, int sender_socket) {
    for (auto &client : clients) {
        if (client.first != sender_socket) {
            send(client.first, message.c_str(), message.size(), 0);
        }
    }
}

void message_person(const string &message, int sender_socket, string receiver) {
    if(user_socket.find(receiver) != user_socket.end()){
        send(user_socket[receiver], message.c_str(), message.size(), 0);
    }
    else{
        string user_not_found = "Error: User not found.\n";
        send(sender_socket, user_not_found.c_str(), user_not_found.size(), 0);
    }
}

void create_group(const string &group_name, int client_socket) {
    size_t space_pos = group_name.find(' ');
    if (space_pos != string::npos) {
        string invalid_group_name = "Error: Group name cannot contain spaces.\n";
        send(client_socket, invalid_group_name.c_str(), invalid_group_name.size(), 0);
        return;
    }
    if (groups.find(group_name) == groups.end()) {
        groups[group_name].insert(client_socket);
        string group_created = "Group " + group_name + " has been created by " + clients[client_socket] + ".\n";
        broadcast_message(group_created, client_socket);
    }
    else {
        string group_exists = "Error: Group already exists.\n";
        send(client_socket, group_exists.c_str(), group_exists.size(), 0);
    }
}

void group_message(const string &message, int sender_socket, string group) {
    if (groups.find(group) != groups.end()) {
        if (groups[group].find(sender_socket) == groups[group].end()) {
            string not_in_group = "Error: You are not part of this group.\n";
            send(sender_socket, not_in_group.c_str(), not_in_group.size(), 0);
            return;
        }
        for (auto &client : groups[group]) {
            if (client != sender_socket) {
                send(client, message.c_str(), message.size(), 0);
            }
        }
    }
    else {
        string group_not_found = "Error: Group not found.\n";
        send(sender_socket, group_not_found.c_str(), group_not_found.size(), 0);
    }
}

void handle_client(int client_socket) {
    char buffer[BUFFER_SIZE] = {0}; // buffer to store messages

    string auth_request = "Enter username: ";
    send(client_socket, auth_request.c_str(), auth_request.size(), 0);

    int valread = read(client_socket, buffer, BUFFER_SIZE);
    if (valread > 0) {
        buffer[valread] = '\0';
        string input(buffer);

        size_t space_pos = input.find(' ');
        string username = input;
        
        memset(buffer, 0, BUFFER_SIZE);
        string auth_request2 = "Enter password: ";
        send(client_socket, auth_request2.c_str(), auth_request2.size(), 0);
        string password;
        valread = read(client_socket, buffer, BUFFER_SIZE);
        if (valread > 0) {
            buffer[valread] = '\0';
            password = buffer;
        }
        else{
            string failure_message = "Authentication failed. Disconnecting...\n";
            send(client_socket, failure_message.c_str(), failure_message.size(), 0);
            close(client_socket);
            return;
        }

        if (users.find(username) != users.end() && users[username] == password) {
            {
                lock_guard<mutex> lock(user_mutex);
                for (auto &user : clients) {
                    if (user.second == username) {
                        string already_connected = "Error: User already connected. Disconnecting...\n";
                        send(client_socket, already_connected.c_str(), already_connected.size(), 0);
                        close(client_socket);
                        return;
                    }
                }
                clients[client_socket] = username;
                user_socket[username] = client_socket;
            }

            string success_message = "Authentication successful. Welcome!\n";
            send(client_socket, success_message.c_str(), success_message.size(), 0);

            {
                lock_guard<mutex> lock(cout_mutex);
                std::cout << "User " << username << " authenticated successfully." << endl;
            }

            string interaction_message = "You are now connected to the server.\n";
            send(client_socket, interaction_message.c_str(), interaction_message.size(), 0);
            usleep(1000);

            while(true){
                memset(buffer, 0, BUFFER_SIZE);
                int message = read(client_socket, buffer, BUFFER_SIZE);
                if(message>0){
                    buffer[message] = '\0';
                    string input(buffer);
                    space_pos = input.find(' ');
                
                    {
                        lock_guard<mutex> lock(cout_mutex);
                        string message = "[" + username+ "]" + ": " + input + "\n";
                        std::cout<<message<<"\n";
                    }
                    string function;
                    string information;
                    if(space_pos == string::npos){
                        function = input;
                    }
                    else{
                        function = input.substr(0, space_pos);
                        information = input.substr(space_pos + 1);
                    }
                    if(function == "/broadcast"){
                        string final_message = "[Broadcast message by " + username + "]: " + information;
                        {
                            lock_guard<mutex> lock(user_mutex);
                            broadcast_message(final_message, client_socket);
                        }
                    }
                    else if(function == "/msg"){
                        int second_space = information.find(' ');
                        string receiver = information.substr(0, second_space);
                        string message = information.substr(second_space + 1);
                        string final_message = "[" + username + "]: " + message;
                        {
                            lock_guard<mutex> lock(user_mutex);
                            message_person(final_message, client_socket, receiver);
                        }
                    }
                    else if (function == "/create_group") {
                        {
                            lock_guard<mutex> lock(group_mutex);
                            create_group(information, client_socket);
                        }
                    }
                    else if (function == "/join_group") {
                        {
                            lock_guard<mutex> lock(group_mutex);
                            if (groups.find(information) != groups.end()) {
                                groups[information].insert(client_socket);
                                string joined_group = username + " has joined the group " + information + ".\n";
                                group_message(joined_group, client_socket, information);
                                string alert = "You have joined the group " + information + ".\n";
                                send(client_socket, alert.c_str(), alert.size(), 0);
                            }
                            else {
                                string group_not_found = "Error: Group not found.\n";
                                send(client_socket, group_not_found.c_str(), group_not_found.size(), 0);
                            }
                        }
                    }
                    else if (function == "/leave_group") {
                        {
                            lock_guard<mutex> lock(group_mutex);
                            if (groups.find(information) != groups.end()) {
                                if(groups[information].find(client_socket) == groups[information].end()){
                                    string not_in_group = "Error: You are not part of this group.\n";
                                    send(client_socket, not_in_group.c_str(), not_in_group.size(), 0);
                                    continue;
                                }
                                else{
                                    string left_group = username + " has left the group " + information + ".\n";
                                    group_message(left_group, client_socket, information);
                                    string alert = "You have left the group " + information + ".\n";
                                    send(client_socket, alert.c_str(), alert.size(), 0);
                                    groups[information].erase(client_socket);
                                }

                            }
                            else {
                                string group_not_found = "Error: Group not found.\n";
                                send(client_socket, group_not_found.c_str(), group_not_found.size(), 0);
                            }
                        }
                    }
                    else if (function == "/group_msg") {
                        int second_space = information.find(' ');
                        string group = information.substr(0, second_space);
                        string message = information.substr(second_space + 1);
                        string final_message = "[" + group + " - " + username + "]: " + message;
                        {
                            lock_guard<mutex> lock(group_mutex);
                            group_message(final_message, client_socket, group);
                        }
                    }
                    else if(function == "/exit"){
                        string exit_message = "Exiting...\n";
                        send(client_socket, exit_message.c_str(), exit_message.size(), 0);
                        break;
                    }
                    else if(function == "/list_users"){
                        string user_list = "Users connected to the server:\n";
                        {
                            lock_guard<mutex> lock(user_mutex);
                            for (auto &user : clients) {
                                user_list += user.second + "\n";
                            }
                        }
                        send(client_socket, user_list.c_str(), user_list.size(), 0);
                    }
                    else if(function == "/list_groups"){
                        string group_list = "Groups available:\n";
                        {
                            lock_guard<mutex> lock(group_mutex);
                            for (auto &group : groups) {
                                group_list += group.first + "\n";
                            }
                        }
                        send(client_socket, group_list.c_str(), group_list.size(), 0);
                    }
                    else {
                        string invalid_command = "Error: Invalid command.\n";
                        send(client_socket, invalid_command.c_str(), invalid_command.size(), 0);
                    }
                }
            }

            // Remove user upon disconnection
            {
                lock_guard<mutex> lock(user_mutex);
                clients.erase(client_socket);
                user_socket.erase(username);
            }

            {
                lock_guard<mutex> lock(cout_mutex);
                std::cout << "User " << username << " disconnected." << endl;
            }
        }
        else {
            string failure_message = "Authentication failed. Disconnecting...\n";
            send(client_socket, failure_message.c_str(), failure_message.size(), 0);
            close(client_socket);
            return;
        }
    }
    else {
        string failure_message = "Authentication failed. Disconnecting...\n";
        send(client_socket, failure_message.c_str(), failure_message.size(), 0);
        close(client_socket);
        return;
    }

    close(client_socket);
}

int main() {

    ifstream file("users.txt");
    if (!file.is_open()) {
        cerr << "Error: Could not open the file." << endl;
        return 1;
    }

    string line;
    while (getline(file, line)) {
        istringstream lineStream(line);
        string part1, part2;

        // Use getline to extract the two parts, separated by ':'
        if (getline(lineStream, part1, ':') && getline(lineStream, part2)) {
            users[part1] = part2;
        } else {
            cerr << "Warning: Malformed line: " << line << endl;
        }
    }

    file.close();

    int server_fd;                     // fd for server socket
    struct sockaddr_in address;        // server address information
    int opt = 1;                       // Option
    int addrlen = sizeof(address);
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }

    //Set socket options to reuse address and port
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("Setsockopt failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // Configure server address structure
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // Bind the socket to the specified address and port
    if ((::bind(server_fd, (struct sockaddr *)&address, sizeof(address))) < 0) {
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
        lock_guard<mutex> lock(cout_mutex);
        std::cout << "Server is listening on port " << PORT << "..." << endl;
    }

    // TODO
    //fcntl(server_fd, F_SETFL, O_NONBLOCK); // Make the server socket non-blocking

    thread exit_thread(listen_for_exit_command);
    exit_thread.detach(); //listen for the exit command in the background

    vector<thread> threads;

    while (!shutdown_server) {
        int new_socket;  // Socket for the new client connection
        
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
            if (errno == EWOULDBLOCK || errno == EAGAIN) {
                // No pending connections, sleep for a short time and retry
                this_thread::sleep_for(chrono::milliseconds(100));
                continue;
            } 
            else if (shutdown_server) {
                // Accept failed because the server is shutting down
                break;
            }
            perror("Accept failed");
            close(server_fd);
            exit(EXIT_FAILURE);
        }

        {
            lock_guard<mutex> lock(cout_mutex);
            std::cout << "Connection established with a client." << endl;
        }

        // Handle the client in a separate thread
        threads.emplace_back(::thread(handle_client, new_socket));
    }

    // Server is shutting down
    std::cout << "Server is shutting down..." << endl;

    // Close all client sockets
    for (auto client_socket : clients) {
        close(client_socket.first);
    }

    std::cout<<"Closing server socket..."<<endl;

    // Close the server socket
    close(server_fd);

    std::cout<<"Server socket closed."<<endl;

    // Join all client threads
    for (thread &t : threads) {
        if (t.joinable()) {
            t.join();
        }
    }

    std::cout<<"All client threads joined."<<endl;

    // Join the exit thread
    if (exit_thread.joinable()) {
        exit_thread.join();
    }

    std::cout << "Server has shut down cleanly." << endl;
    return 0;
}
