#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <iostream>
#include "scheduler.hpp"
#include "co_syscall.hpp"

#define SERVER_IP "127.0.0.1"
#define BUFFER_SIZE 1024


class client {
    int clientSocket;
    struct sockaddr_in serverAddr;

    
public:

    client(int port) {
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
    }

    task<int> run(scheduler &sche) {
        char package[MAX_PACKAGE_SIZE+1];
        char userInput[BUFFER_SIZE];
        while (1) {
            // Get user input
            printf("Enter a message (type 'exit' to quit): ");
            fgets(userInput, sizeof(userInput), stdin);

            // Remove trailing newline character
            int32_t len = strlen(userInput) - 1;
            if (len > 0 && userInput[len] == '\n') {
                userInput[len] = '\0';
            }
            int32_t a, b;
            sscanf(userInput, "%d %d", &a, &b);
            int err{};
            int32_t res = co_await co_rpc::add_number(sche, clientSocket, a, b, err);

            if(err) {
                perror("Send failed");
                close(clientSocket);
                exit(EXIT_FAILURE);
            }
            
            printf("Received from server: %d\n", res);
        }

    }
    

    ~client()
    {
        close(clientSocket);
    }
};




int main(int argc, char **argv) {
    if (argc != 2) {
        std::cout << "Usage: client <port>" << std::endl;
        return 0;
    }
    int port = std::atoi(argv[1]);
    client cli(port);
    scheduler sche;
    sche.schedule(cli.run(sche));
    sche.run();

    return 0;
}
