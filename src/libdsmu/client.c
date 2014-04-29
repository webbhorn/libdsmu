//
//  client.c
//  Simple C client socket demo
//  6.824 Final Project, Spring 14
//

#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define PORT "4444" // Port that the server operates on

int main(int argc, const char * argv[]) {
    // Initialize client variables.
    struct addrinfo * resolvedAddr;     // Our server's address struct.
    struct addrinfo hints;              // Parameters for setting up that address.
    memset(&hints, 0, sizeof(hints));   // Clear hints, then set it up:
    
    hints.ai_family = AF_UNSPEC;        // IPv4 or IPv6 socket, left up to OS.
    hints.ai_socktype = SOCK_STREAM;    // Want stream protocol, not datagram.
    hints.ai_flags = AI_PASSIVE;        // Want localhost: let the OS take care of finding my local address.
    
    // Lookup the server's address.
    int lookupStatus = getaddrinfo(NULL, PORT, &hints, &resolvedAddr); // NULL means localhost for this example.
    if (lookupStatus < 0) {
        fprintf(stderr, "Local address lookup failed with error %s\n", gai_strerror(lookupStatus));
        return 1;
    }
    
    // Acquire the socket to the server, save as server file descriptor.
    int serverfd = socket(resolvedAddr->ai_family, resolvedAddr->ai_socktype, resolvedAddr->ai_protocol);
    if (serverfd < 0) {
        fprintf(stderr, "Server socket could not be acquired, quitting...\n");
        return 2;
    }
    
    // Connect to the host.
    if (connect(serverfd, resolvedAddr->ai_addr, resolvedAddr->ai_addrlen) < 0) {
        printf("Could not connect to remote host, quitting...\n");
        return 2;
    }
    
    // Send our test message to the server.
    char msgToSend[] = "hello, server! this is the client!"; // Dummy message, just any string here.
    if (send(serverfd, msgToSend, sizeof(msgToSend), 0) < 0) {
        printf("Could not send message over socket, quitting...\n");
        return 3;
    }
    
    // Close the socket and quit.
    close(serverfd);
    printf("Program terminated successfully!\n");
    
    return 0;
}
