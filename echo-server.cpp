#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define MAX_CLIENTS 10
#define BUFFER_SIZE 1024

int main() {
    int serverSocket, clientSockets[MAX_CLIENTS], maxClients = MAX_CLIENTS;
    fd_set readfds;
    struct sockaddr_in serverAddr, clientAddr;
    socklen_t addrLen = sizeof(clientAddr);

    char buffer[BUFFER_SIZE];

    // Create socket
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == -1) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Set up server address struct
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(8080); // Change this port as needed

    // Bind socket
    if (bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == -1) {
        perror("Bind failed");
        close(serverSocket);
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections
    if (listen(serverSocket, MAX_CLIENTS) == -1) {
        perror("Listen failed");
        close(serverSocket);
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port 8080...\n");

    // Initialize client sockets array
    for (int i = 0; i < MAX_CLIENTS; ++i) {
        clientSockets[i] = 0;
    }

    while (1) {
        FD_ZERO(&readfds);
        FD_SET(serverSocket, &readfds);

        int maxSocket = serverSocket;

        // Add child sockets to set
        for (int i = 0; i < MAX_CLIENTS; ++i) {
            int sd = clientSockets[i];

            if (sd > 0) {
                FD_SET(sd, &readfds);
            }

            if (sd > maxSocket) {
                maxSocket = sd;
            }
        }

        // Wait for activity on any of the sockets
        int activity = select(maxSocket + 1, &readfds, NULL, NULL, NULL);

        if (activity == -1) {
            perror("Select error");
            exit(EXIT_FAILURE);
        }

        // If a new connection is available, accept it
        if (FD_ISSET(serverSocket, &readfds)) {
            int newSocket = accept(serverSocket, (struct sockaddr*)&clientAddr, &addrLen);
            if (newSocket == -1) {
                perror("Accept failed");
                exit(EXIT_FAILURE);
            }

            printf("New connection, socket fd is %d, ip is: %s, port: %d\n",
                   newSocket, inet_ntoa(clientAddr.sin_addr), ntohs(clientAddr.sin_port));

            // Add the new socket to the array of sockets
            for (int i = 0; i < MAX_CLIENTS; ++i) {
                if (clientSockets[i] == 0) {
                    clientSockets[i] = newSocket;
                    break;
                }
            }
        }

        // Handle data from clients
        for (int i = 0; i < MAX_CLIENTS; ++i) {
            int sd = clientSockets[i];

            if (FD_ISSET(sd, &readfds)) {
                // Read the incoming message
                int bytesRead = read(sd, buffer, BUFFER_SIZE);
                if (bytesRead == 0) {
                    // Client disconnected
                    printf("Host disconnected, socket fd is %d\n", sd);
                    close(sd);
                    clientSockets[i] = 0;
                } else {
                    // Echo the message back to the client
                    buffer[bytesRead] = '\0';
                    printf("Received: %s", buffer);
                    write(sd, buffer, strlen(buffer));
                }
            }
        }
    }

    close(serverSocket);

    return 0;
}