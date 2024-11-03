#include <iostream>
#include <string>
#include <cstring>
#include <unistd.h>
#include <sstream>  // Include this header for std::istringstream
#include <arpa/inet.h>
#include <vector>

#define PORT 4242

// List of server IPs
std::vector<std::string> serverIPs = {"127.0.0.2", "127.0.0.3", "127.0.0.4"};
int currentServerIndex = 0;  // Round-robin index

// Function to connect to the server and send a command

void sendCommand(const std::string& command) {
    std::string serverIP = serverIPs[currentServerIndex];
    int sock = 0;
    struct sockaddr_in serv_addr;

    // Create socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        std::cerr << "Socket creation error" << std::endl;
        return;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    // Convert IP address from text to binary form
    int connected = 0;
    int retries = 0;
    do {
        if (inet_pton(AF_INET, serverIP.c_str(), &serv_addr.sin_addr) <= 0) {
            std::cerr << "Invalid address/ Address not supported" << std::endl;
            currentServerIndex = (currentServerIndex + 1) % serverIPs.size();
            serverIP = serverIPs[currentServerIndex];
            retries += 1;
        } else if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
            std::cerr << "Connection to server " << serverIP << " failed, skipping" << std::endl;
            currentServerIndex = (currentServerIndex + 1) % serverIPs.size();
            serverIP = serverIPs[currentServerIndex];
            retries += 1;
        } else {
            connected = 1;
        }
    } while (connected == 0 && retries < serverIPs.size());

    if (connected == 0) {
        std::cerr << "No server found" << std::endl;
        close(sock);
        return;
    }

    // Send the command to the server
    send(sock, command.c_str(), command.length(), 0);

    // Buffer to receive the response
    char buffer[1024] = {0};
    int bytesRead = read(sock, buffer, sizeof(buffer) - 1);
    if (bytesRead > 0) {
        buffer[bytesRead] = '\0';  // Null-terminate the received data
        std::cout << buffer << std::endl;
    }

    // Close the socket
    close(sock);
    currentServerIndex = (currentServerIndex + 1) % serverIPs.size();
}

int main() {
    std::string command;

    std::cout << "Enter commands (SET key value, GET key or DELETE key) or 'exit' to quit:\n";

    while (true) {
        std::cout << "> ";
        std::getline(std::cin, command);

        // Exit command
        if (command == "exit") {
            break;
        }

        // Process command
        std::istringstream iss(command);
        std::string operation, key, value;

        iss >> operation;

        if (operation == "SET") {
            iss >> key >> value;
            if (key.empty() || value.empty()) {
                std::cout << "Invalid SET command. Usage: SET key value" << std::endl;
                continue;
            }
            sendCommand("SET " + key + " " + value);
        }
        else if (operation == "GET") {
            iss >> key;
            if (key.empty()) {
                std::cout << "Invalid GET command. Usage: GET key" << std::endl;
                continue;
            }
            sendCommand("GET " + key);
        }
        else if (operation == "DELETE") {
            iss >> key;
            if (key.empty()) {
                std::cout << "Invalid DELETE command. Usage: DELETE key" << std::endl;
                continue;
            }
            sendCommand("DELETE " + key);
        }
        else {
            std::cout << "Unknown command. Use SET or GET." << std::endl;
        }
    }

    return 0;
}
