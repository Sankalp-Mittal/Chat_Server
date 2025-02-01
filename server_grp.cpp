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

unordered_map <int, string> clients; // Client socket -> username
unordered_map <string, string> users; // Username -> password
unordered_map <string, unordered_set <int> > groups; // Group -> client sockets

void handle_client(int client_socket) {
    char buffer[BUFFER_SIZE] = {0}; // buffer to store messages

    const char *auth_request = "Enter username: ";
    send(client_socket, auth_request, strlen(auth_request), 0);

    int valread = read(client_socket, buffer, BUFFER_SIZE);
    if (valread > 0) {
        buffer[valread] = '\0';
        string input(buffer);

        size_t space_pos = input.find(' ');
        string username = input;
        
        memset(buffer, 0, BUFFER_SIZE);
        const char *auth_request2 = "Enter password: ";
        send(client_socket, auth_request2, strlen(auth_request2), 0);
        string password;
        valread = read(client_socket, buffer, BUFFER_SIZE);
        if (valread > 0) {
            buffer[valread] = '\0';
            password = buffer;
        }
        else{
            const char *failure_message = "Authentication failed. Disconnecting...\n";
            send(client_socket, failure_message, strlen(failure_message), 0);
            close(client_socket);
            return;
        }

        if (users.find(username) != users.end() && users[username] == password) {
            {
                lock_guard<mutex> lock(user_mutex);
                for (auto &user : clients) {
                    if (user.second == username) {
                        const char *already_connected = "User already connected. Disconnecting...\n";
                        send(client_socket, already_connected, strlen(already_connected), 0);
                        close(client_socket);
                        return;
                    }
                }
                clients[client_socket] = username;
            }

            const char *success_message = "Authentication successful. Welcome!\n";
            send(client_socket, success_message, strlen(success_message), 0);

            {
                lock_guard<mutex> lock(cout_mutex);
                cout << "User " << username << " authenticated successfully." << endl;
            }

            const char *interaction_message = "You are now connected to the server.\n";
            send(client_socket, interaction_message, strlen(interaction_message), 0);
            usleep(1000);

            while(true){
                memset(buffer, 0, BUFFER_SIZE);
                int message = read(client_socket, buffer, BUFFER_SIZE);
                if(message>0){
                    buffer[message] = '\0';
                    string input(buffer);
                    space_pos = input.find(' ');
                    string function = input.substr(0, space_pos);
                    string information = input.substr(space_pos + 1);
                    string final_message = "[" + username + "]: " + information;
                
                    {
                        lock_guard<mutex> lock(cout_mutex);
                        cout<<final_message<<"\n";
                    }

                    if(function == "/broadcast"){
                        {
                            lock_guard<mutex> lock(user_mutex);
                            for(auto &user : clients){
                                if(user.second != username){
                                    send(user.first, final_message.c_str(), final_message.size(), 0);
                                }
                            }
                        }
                    }
                    else if(function == "/msg"){
                        int second_space = information.find(' ');
                        string receiver = information.substr(0, second_space);
                        string message = information.substr(second_space + 1);
                        final_message = "[" + username + "]: " + message;
                        {
                            lock_guard<mutex> lock(user_mutex);
                            for (auto &user : clients) {
                                if (user.second == receiver) {
                                    send(user.first, final_message.c_str(), final_message.size(), 0);
                                }
                            }
                        }
                    }
                    else if (function == "/create_group") {
                        {
                            lock_guard<mutex> lock(user_mutex);
                            if (groups.find(information) == groups.end()) {
                                groups[information].insert(client_socket);
                            }
                            else {
                                const char *group_exists = "Group already exists.\n";
                                send(client_socket, group_exists, strlen(group_exists), 0);
                            }
                        }
                    }
                    else if (function == "/join_group") {
                        {
                            lock_guard<mutex> lock(user_mutex);
                            if (groups.find(information) != groups.end()) {
                                groups[information].insert(client_socket);
                            }
                            else {
                                const char *group_not_found = "Group not found.\n";
                                send(client_socket, group_not_found, strlen(group_not_found), 0);
                            }
                        }
                    }
                    else if (function == "/leave_group") {
                        {
                            lock_guard<mutex> lock(user_mutex);
                            if (groups.find(information) != groups.end()) {
                                groups[information].erase(client_socket);
                            }
                            else {
                                const char *group_not_found = "Group not found.\n";
                                send(client_socket, group_not_found, strlen(group_not_found), 0);
                            }
                        }
                    }
                    else if (function == "/group_msg") {
                        int second_space = information.find(' ');
                        string group = information.substr(0, second_space);
                        string message = information.substr(second_space + 1);
                        final_message = "[" + group + " - " + username + "]: " + message;
                        {
                            lock_guard<mutex> lock(user_mutex);
                            if (groups.find(group) != groups.end()) {
                                if (groups[group].find(client_socket) == groups[group].end()) {
                                    const char *not_in_group = "You are not part of this group.\n";
                                    send(client_socket, not_in_group, strlen(not_in_group), 0);
                                    continue;
                                }
                                for (auto &client : groups[group]) {
                                    if (client != client_socket) {
                                        send(client, final_message.c_str(), final_message.size(), 0);
                                    }
                                }
                            }
                            else {
                                const char *group_not_found = "Group not found.\n";
                                send(client_socket, group_not_found, strlen(group_not_found), 0);
                            }
                        }
                    }
                    else {
                        const char *invalid_command = "Invalid command.\n";
                        send(client_socket, invalid_command, strlen(invalid_command), 0);
                    }
                }
            }

            // Remove user upon disconnection
            {
                lock_guard<mutex> lock(user_mutex);
                clients.erase(client_socket);
            }

            {
                lock_guard<mutex> lock(cout_mutex);
                cout << "User " << username << " disconnected." << endl;
            }
        }
        else {
            const char *failure_message = "Authentication failed. Disconnecting...\n";
            send(client_socket, failure_message, strlen(failure_message), 0);
            close(client_socket);
            return;
        }
    }
    else {
        const char *failure_message = "Authentication failed. Disconnecting...\n";
        send(client_socket, failure_message, strlen(failure_message), 0);
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
        cout << "Server is listening on port " << PORT << "..." << endl;
    }

    vector<thread> threads;

    while (true) {
        int new_socket;  // Socket for the new client connection

        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
            perror("Accept failed");
            close(server_fd);
            exit(EXIT_FAILURE);
        }

        {
            lock_guard<mutex> lock(cout_mutex);
            cout << "Connection established with a client." << endl;
        }

        // Handle the client in a separate thread
        threads.emplace_back(::thread(handle_client, new_socket));
    }

    // Join all threads before exiting for cleanup
    for (auto &t : threads) {
        if (t.joinable()) {
            t.join();
        }
    }

    close(server_fd);

    return 0;
}
