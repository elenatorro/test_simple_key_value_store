#include <iostream>
#include <string>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <map>
#include <fstream>
#include <sstream>
#include <thread>
#include <chrono>
#include <vector>
#include <mutex>

#define PORT 4242

std::map<std::string, std::string> dataStore; // Key-value store
std::mutex dataMutex; // Mutex for thread-safe access to dataStore

// List of other server IPs for synchronization
std::vector<std::string> otherServers;
std::string localIP;


void saveDataToFile(const std::string& filename) {
    while (true) {
        std::ofstream outFile(filename, std::ios::trunc);
        for (const auto& pair : dataStore) {
            outFile << pair.first << " " << pair.second << std::endl;
        }
        outFile.close();
        std::this_thread::sleep_for(std::chrono::seconds(1)); // Save every second
    }
}

// Function to load existing data from a file into memory on startup
void loadDataFromFile(const std::string& filename) {
    std::ifstream inFile(filename);
    if (!inFile.is_open()) {
        std::cerr << "File " << filename << " not found or could not be opened." << std::endl;
        return;
    }

    std::string key, value;
    dataMutex.lock(); // Lock for thread-safe access to dataStore
    while (inFile >> key >> value) {
        dataStore[key] = value; // Populate the dataStore with data from the file
    }
    dataMutex.unlock();
    inFile.close();
}


// Function to synchronize data with other servers
void sendData(const std::string& operation, const std::string& key) {
    // Prepare a synchronization message with current data
    std::ostringstream dataStream;
    for (const auto& pair : dataStore) {
        if (key == pair.first) {
            dataStream << "SYNC" << " " << operation << " " << pair.first << " " << pair.second << "\n";
        }
        else {
            dataStream << "SYNC" << " " << "KEEP" << " " << pair.first << " " << pair.second << "\n";
        }
    }

    if (operation == "DELETE") {
        dataStore.erase(key);
    }

    std::string syncData = dataStream.str();

    // Send current data to other servers
    for (const auto& serverIP : otherServers) {
        int sock = socket(AF_INET, SOCK_DGRAM, 0);
        struct sockaddr_in serv_addr;

        if (sock < 0) {
            std::cerr << "Socket creation error" << std::endl;
            continue;
        }

        serv_addr.sin_family = AF_INET;
        serv_addr.sin_port = htons(PORT);  // Ensure this matches the port defined above
        inet_pton(AF_INET, serverIP.c_str(), &serv_addr.sin_addr);

        // Send the synchronization data
        sendto(sock, syncData.c_str(), syncData.length(), 0, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
        close(sock);  // Close the socket after sending
    }
}


// Function to receive synchronization data from other servers
void receiveSyncData(const std::string& ip) {
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = inet_addr(ip.c_str()); // Bind to the specific server IP
    address.sin_port = htons(PORT); // Bind to the same port

    // Bind to the specific IP
    if (bind(sock, (struct sockaddr*)&address, sizeof(address)) < 0) {
        std::cerr << "Bind failed for IP " << ip << std::endl;
        return;
    }

    while (true) {
        char buffer[1024] = {0};
        socklen_t addr_len = sizeof(address);
        int bytesRead = recvfrom(sock, buffer, sizeof(buffer) - 1, 0, (struct sockaddr*)&address, &addr_len);
        if (bytesRead > 0) {
            buffer[bytesRead] = '\0'; // Null-terminate the received data
            std::string syncMessage(buffer);
            
            std::istringstream iss(syncMessage);
            std::string sync, operation, key, value;

            while (iss >> sync) {
                if (sync == "SYNC") {
                    iss >> operation; // Read operation
                    std::cout << "operation: " << operation << std::endl;
                    dataMutex.lock();
                    // Update the local store
                    if (operation == "SET") {
                        iss >> key >> value;
                        std::cout << "key: " << key << std::endl;
                        dataStore[key] = value; // Always overwrite the existing key
                    }
                    else if (operation == "DELETE") {
                        iss >> key; // Read key
                        std::cout << "key: " << key << std::endl;
                        dataStore.erase(key);
                    }
                    dataMutex.unlock();
                }
            }
        }
    }
}

void requestData(const std::string& localIP) {
    for (const auto& serverIP : otherServers) {
        if (localIP == serverIP) {
            continue;
        }
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
        if (inet_pton(AF_INET, serverIP.c_str(), &serv_addr.sin_addr) <= 0) {
            std::cerr << "Invalid address/ Address not supported" << std::endl;
            close(sock);
            return;
        }

        // Connect to the server
        if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
            std::cerr << "Connection to server " << serverIP << " failed" << std::endl;
            close(sock);
            return;
        }

        std::string command;
        command = "GET_ALL_DATA";

        // Send the command to the server
        send(sock, command.c_str(), command.length(), 0);

        // Buffer to receive the response
        char buffer[1024] = {0};
        int bytesRead = read(sock, buffer, sizeof(buffer) - 1);

        if (bytesRead > 0) {
            buffer[bytesRead] = '\0';  // Null-terminate the received data
            std::istringstream response(buffer);
            std::string key, value;

            while (response >> key >> value) {
                dataStore[key] = value;
            }
            // Save updated data to file
            std::string filename = "data_" + localIP + ".txt";
        }
        // Close the socket
        close(sock);
    }
    return;
}

// Function to handle client connections
void handleClient(int clientSocket) {
    char buffer[1024] = {0};
    int bytesRead = read(clientSocket, buffer, sizeof(buffer) - 1);
    if (bytesRead <= 0) {
        close(clientSocket);
        return;
    }

    buffer[bytesRead] = '\0';  // Null-terminate the received data
    std::string command(buffer);
    
    std::istringstream iss(command);
    std::string operation, key, value;

    iss >> operation;
    dataMutex.lock(); // Lock for thread-safe access to dataStore
    if (operation == "SET") {
        iss >> key >> value; // Read key and value
        dataStore[key] = value; // Store the key-value pair in memory

        std::string response = "Key set successfully.";
        send(clientSocket, response.c_str(), response.length(), 0);
        sendData(operation, key);
    } else if (operation == "GET") {
        iss >> key; // Read key
        if (dataStore.find(key) != dataStore.end()) {
            std::string response = "Value: " + dataStore[key];
            send(clientSocket, response.c_str(), response.length(), 0);
        } else {
            std::string response = "Key not found.";
            send(clientSocket, response.c_str(), response.length(), 0);
        }
    } else if (operation == "DELETE") {
        iss >> key; // Read key
        if (dataStore.find(key) != dataStore.end()) {
            std::string response = "Key deleted successfully.";
            send(clientSocket, response.c_str(), response.length(), 0);
            sendData(operation, key);
        } else {
            std::string response = "Key not found.";
            send(clientSocket, response.c_str(), response.length(), 0);
        }
    } else if (operation == "GET_ALL_DATA") {
        std::cout << "Command received: GET ALL DATA" << std::endl;
        std::ostringstream responseStream;
        for (const auto& pair : dataStore) {
            responseStream << pair.first << " " << pair.second << "\n";
        }

        std::string response = responseStream.str();
        send(clientSocket, response.c_str(), response.length(), 0);   
    }
    dataMutex.unlock(); // Unlock after modifying dataStore

    close(clientSocket);
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: ./server <local_ip>" << std::endl;
        return 1;
    }

    std::string localIP = argv[1];
    otherServers = {"127.0.0.2", "127.0.0.3", "127.0.0.4"};
    std::string filename = "data_" + localIP + ".txt";

    loadDataFromFile(filename);

    // Start saving data to file in a separate thread
    std::thread fileThread(saveDataToFile, filename);
    fileThread.detach();

    // Start the UDP listener for synchronization messages
    std::thread receiveThread(receiveSyncData, localIP); // Bind to local IP
    receiveThread.detach();

    // Set up the server socket
    int serverFd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);

    // Set socket options
    if (setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        std::cerr << "setsockopt failed" << std::endl;
        return 1;
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = inet_addr(localIP.c_str());
    address.sin_port = htons(PORT);

    bind(serverFd, (struct sockaddr*)&address, sizeof(address));
    listen(serverFd, 3);

    std::cout << "Server listening on " << localIP << ":" << PORT << std::endl;

    requestData(localIP);

    while (true) {
        int newSocket = accept(serverFd, (struct sockaddr*)&address, (socklen_t*)&addrlen);
        std::thread clientThread(handleClient, newSocket);
        clientThread.detach();
    }

    close(serverFd);
    return 0;
}
