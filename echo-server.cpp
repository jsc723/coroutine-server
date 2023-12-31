#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <iostream>
#include <string>
#include <vector>
#include <format>
#include "scheduler.hpp"
#include "co_syscall.hpp"

#define MAX_CLIENTS 10
#define BUFFER_SIZE 1024


class server {
public:
    int serverSocket, clientSockets[MAX_CLIENTS], maxClients = MAX_CLIENTS;
    struct sockaddr_in serverAddr, clientAddr;
    socklen_t addrLen = sizeof(clientAddr);

    char buffer[BUFFER_SIZE];

    server(int port) {
        serverSocket = socket(AF_INET, SOCK_STREAM, 0);
        if (serverSocket == -1) {
            perror("Socket creation failed");
            exit(EXIT_FAILURE);
        }

        // Set up server address struct
        memset(&serverAddr, 0, sizeof(serverAddr));
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_addr.s_addr = INADDR_ANY;
        serverAddr.sin_port = htons(port); // Change this port as needed

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

        std::cout << std::format("Server listening on port {}...\n", port);

        // Initialize client sockets array
        for (int i = 0; i < MAX_CLIENTS; ++i) {
            clientSockets[i] = 0;
        }
    }
    

    
    scheduler::task_t co_listen(scheduler &sche)
    {
        while(1) {
            // If a new connection is available, accept it
            std::cout << "before accept" << std::endl;
            int newSocket = co_await co_syscall::accept(sche, serverSocket, (struct sockaddr *)&clientAddr, &addrLen);
            if (newSocket == -1)
            {
                perror("Accept failed");
                exit(EXIT_FAILURE);
            }

            printf("New connection, socket fd is %d, ip is: %s, port: %d\n",
                newSocket, inet_ntoa(clientAddr.sin_addr), ntohs(clientAddr.sin_port));

            // Add the new socket to the array of sockets
            for (int i = 0; i < MAX_CLIENTS; ++i)
            {
                if (clientSockets[i] == 0)
                {
                    clientSockets[i] = newSocket;
                    sche.schedule(co_handle_client(sche, i), await_state::schedule_now);
                    break;
                }
            }
        }
        
    }

    scheduler::task_t co_handle_client(scheduler &sche, int clientSockets_i)
    {
        while(1) {
            int sd = clientSockets[clientSockets_i];
            // Read the incoming message
            int bytesRead = co_await co_syscall::read(sche, sd, buffer, BUFFER_SIZE);
            if (bytesRead == 0)
            {
                // Client disconnected
                printf("Host disconnected, socket fd is %d\n", sd);
                close(sd);
                clientSockets[clientSockets_i] = 0;
                break;
            }
            else
            {
                // Echo the message back to the client
                buffer[bytesRead] = '\0';
                printf("Received: %s", buffer);
                write(sd, buffer, strlen(buffer));
            }
        }
    }

    ~server()
    {
        close(serverSocket);
    }
};



int main(int argc, char **argv) {
    if (argc != 2) {
        std::cout << "Usage: server <port>" << std::endl;
        return 0;
    }
    int port = std::atoi(argv[1]);
    scheduler sche;
    server sv(port);

    sche.schedule(sv.co_listen(sche));
    sche.run();

    return 0;
}