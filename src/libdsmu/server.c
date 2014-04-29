//
//  server.c
//  Simple C server socket demo
//  6.824 Final Project, Spring 14
//

#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define PORT   "4444" // Port that the server operates on
#define MAXCON 12     // Maximum number of simultaneous connections (before accept() call)
#define BUFLEN 512    // Length of the buffer to store client messages to the server

int main(int argc, char *argv[]) {
    // Initialize server variables.
    struct addrinfo * resolvedAddr;     // Our local server address.
    struct addrinfo hints;              // Parameters for setting up that local address.
    memset(&hints, 0, sizeof(hints));   // Clear hints, then set it up:
    
    hints.ai_family = AF_UNSPEC;        // IPv4 or IPv6 socket, left up to OS.
    hints.ai_socktype = SOCK_STREAM;    // Want stream protocol, not datagram.
    hints.ai_flags = AI_PASSIVE;        // Let the OS take care of finding my local address.
    
    // Lookup the local server address.
    int lookupStatus = getaddrinfo(NULL, PORT, &hints, &resolvedAddr);
    if (lookupStatus < 0) {
        fprintf(stderr, "Local address lookup failed with error %s\n", gai_strerror(lookupStatus));
        return 1;
    }
    
    // Acquire the server socket, save as server file descriptor.
    int serverfd = socket(resolvedAddr->ai_family, resolvedAddr->ai_socktype, resolvedAddr->ai_protocol);
    if (serverfd < 0) {
        fprintf(stderr, "Server socket could not be acquired, quitting...\n");
        return 2;
    }
    
    // Bind to the server's socket address.
    if (bind(serverfd, resolvedAddr->ai_addr, resolvedAddr->ai_addrlen) < 0) {
        fprintf(stderr, "Could not bind to server socket, quitting...\n");
        return 3;
    }
    
    
    listen(serverfd, 128); // Listen on our socket's address.
    
    
    int clientfd;                                       // File descriptor for the client.
    char inputBuf[BUFLEN];                              // Buffer to store messages from the client.
    memset(&inputBuf, '0', sizeof(inputBuf));           // Clear that buffer before first use.
    
    struct sockaddr_storage clientAddr;                 // An address structure to accept a client connection with.
    socklen_t clientAddrSize = sizeof(clientAddr);      // The length of that structure.
    
    // Accept a client connection with this information.
    clientfd = accept(serverfd, (struct sockaddr *) &clientAddr, &clientAddrSize);
    if (clientfd < 0) {
        printf("Error accepting new connection, quitting...\n");
        return 4;
    }

    while (1) {
        memset(&inputBuf, '0', sizeof(inputBuf));
		    
        // Receive a message from the client.
        ssize_t n = recv(clientfd, &inputBuf, BUFLEN-1, 0);     // 0 parameter is flags for recv, can send signals this way.
        if (n < 0) {
            printf("Error reading client message, quitting...");
            return 5;
        }

        // Print that message to standard out. (This will be null terminated.)
        printf("Message from the client: %s\n", inputBuf);
        
        // Send a dummy message to the client.
        //n = send(clientfd, "I got your message\n", 19, 0);
        //if (n < 0) {
        //    printf("Error writing message to client, quitting...\n");
        //    return 6;
        //}    
    }

    // Close opened sockets before terminating.
    close(clientfd);
    close(serverfd);
    printf("Program terminated successfully!\n");
    
    return 0;
}
