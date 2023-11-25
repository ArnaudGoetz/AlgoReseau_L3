#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <poll.h>
#include <netdb.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>

#define CHECK(op) do { if ((op) == -1) { perror(#op); exit(EXIT_FAILURE); } } while (0)

#define PORT(p) htons(p)
#define SIZE 100

int main(int argc, char *argv[]) {
    /* Check if the correct number of arguments is provided */
    if (argc != 2) {
        fprintf(stderr, "usage: %s port_number\n", argv[0]);
        return 1;
    }

    /* Convert and check the provided port number */
    unsigned short port_number = atoi(argv[1]);
    if (port_number < 10000 || port_number > 65000) {
        fprintf(stderr, "Invalid port number. Please use a port number in the range [10000; 65000].\n");
        return 1;
    }

    /* Create a socket for both IPv4 and IPv6 */
    int sockfd = socket(AF_INET6, SOCK_DGRAM, 0);
    CHECK(sockfd);

    /* Set the socket to be dual-stack */
    int value = 0;
    CHECK(setsockopt(sockfd, IPPROTO_IPV6, IPV6_V6ONLY, &value, sizeof value));

    char buffer[SIZE];

    // Set local address information
    struct sockaddr_storage ss;
    struct sockaddr_in6 *local_addr = (struct sockaddr_in6 *)&ss;
    memset(local_addr, 0, sizeof *local_addr);
    local_addr->sin6_family = AF_INET6;
    local_addr->sin6_addr = in6addr_any;
    local_addr->sin6_port = PORT(port_number);

    int connected = 0;
    /* Check if a client is already present */
    if (bind(sockfd, (struct sockaddr *)local_addr, sizeof *local_addr) == -1) {
        if (errno == EINVAL || errno == EADDRINUSE) {
        
            CHECK(sendto(sockfd, "/HELO", sizeof("/HELO"), 0, (struct sockaddr *)local_addr, sizeof *local_addr));
        }
    } else {
        // Case when no client is present
        printf("Waiting for a client to connect...\n");

        char expectedMessage[] = "/HELO";
        struct sockaddr_storage sender_ss;
        struct sockaddr_in6 *sender_addr = (struct sockaddr_in6 *)&sender_ss;
        socklen_t sender_addr_len = sizeof(sender_ss);

        while (!connected) {
            // Wait for incoming message
            ssize_t bytes_received = recvfrom(sockfd, buffer, SIZE, 0, (struct sockaddr *)sender_addr, &sender_addr_len);
            CHECK(bytes_received);

            if (bytes_received > 0) {
                buffer[bytes_received] = '\0';
                if (strcmp(buffer, expectedMessage) == 0) 
                {
                    printf("New user me dit: %s \n", buffer);
                    memcpy(local_addr, sender_addr, sizeof(*sender_addr));
                    connected = 1;
                } 
                else 
                {
                    printf("Received message is not /HELO. Continuing to wait for the correct message...\n");
                }
            }
        }
    }

    // Prepare struct pollfd with stdin and socket for incoming data
    struct pollfd fds[2];
    fds[0].fd = 0;          
    fds[0].events = POLLIN; 
    fds[1].fd = sockfd;     
    fds[1].events = POLLIN; 

    // Main loop
    while (1) {
        int result = poll(fds, 2, -1); 
        CHECK(result);

        // user input
        if (fds[0].revents & POLLIN) 
        {
            fgets(buffer, SIZE, stdin);
            
            size_t len = strlen(buffer);
            if (len > 0 && buffer[len - 1] == '\n') {
                buffer[len - 1] = '\0';
            }

            sendto(sockfd, buffer, strlen(buffer), 0, (struct sockaddr *)local_addr, sizeof *local_addr);

            if (strcmp(buffer, "/QUIT") == 0) 
            {
                printf("I am exiting the chat.\n");
                break; 
            }
        }

        // socket 
        if (fds[1].revents & POLLIN) 
        {
            ssize_t bytes_received = recvfrom(sockfd, buffer, SIZE, 0, NULL, NULL);
            if (bytes_received > 0) {
        
                buffer[bytes_received] = '\0';
                printf("Message received: %s \n", buffer);

                if (strcmp(buffer, "/QUIT") == 0) 
                {
                    printf("The other user is exiting the chat.\n");
                    break; 
                }
            }
        }
    }

    // Close the socket
    close(sockfd);

    return 0;
}