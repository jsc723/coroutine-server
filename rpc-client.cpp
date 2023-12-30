#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <iostream>

#define SERVER_IP "127.0.0.1"
#define BUFFER_SIZE 1024

int main(int argc, char **argv) {
    if (argc != 2) {
        std::cout << "Usage: client <port>" << std::endl;
        return 0;
    }
    int port = std::atoi(argv[1]);
    int clientSocket;
    struct sockaddr_in serverAddr;

    char buffer[BUFFER_SIZE];
    char userInput[BUFFER_SIZE];

    // Create socket
    clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket == -1) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Set up server address struct
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    if (inet_pton(AF_INET, SERVER_IP, &(serverAddr.sin_addr)) <= 0) {
        perror("Invalid address/ Address not supported");
        close(clientSocket);
        exit(EXIT_FAILURE);
    }

    // Connect to server
    if (connect(clientSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == -1) {
        perror("Connection failed");
        close(clientSocket);
        exit(EXIT_FAILURE);
    }

    while (1) {
        // Get user input
        printf("Enter a message (type 'exit' to quit): ");
        fgets(userInput, sizeof(userInput), stdin);

        // Remove trailing newline character
        size_t len = strlen(userInput);
        if (len > 0 && userInput[len - 1] == '\n') {
            userInput[len - 1] = '\0';
        }

        // Send user input to server
        if (send(clientSocket, userInput, strlen(userInput), 0) == -1) {
            perror("Send failed");
            close(clientSocket);
            exit(EXIT_FAILURE);
        }

        // Break the loop if the user enters 'exit'
        if (strcmp(userInput, "exit") == 0) {
            break;
        }

        // Receive and display response from server
        ssize_t bytesRead = recv(clientSocket, buffer, sizeof(buffer), 0);
        if (bytesRead == -1) {
            perror("Receive failed");
            close(clientSocket);
            exit(EXIT_FAILURE);
        } else if (bytesRead == 0) {
            printf("Server closed the connection\n");
            break;
        } else {
            buffer[bytesRead] = '\0';
            printf("Received from server: %s\n", buffer);
        }
    }

    // Close the socket
    close(clientSocket);

    return 0;
}
