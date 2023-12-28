
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <poll.h>
#include <netdb.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>

#define CHECK(op) do { if ((op) == -1) { perror(#op); exit(EXIT_FAILURE); } } while (0)

#define PORT(p) htons(p)
#define SIZE 100



#ifdef FILEIO
//--- FICHIER ---

// Function to send a file
void sendFile(int sockfd, const char *command, struct sockaddr_in6 *local_addr) {

    // Extraction du chemin
    char filePath[SIZE];
    sscanf(command, "/FILE%s", filePath);

    // Inform receiver /FILE command
    CHECK(sendto(sockfd, "/FILE", strlen("/FILE"), 0, (struct sockaddr *)local_addr, sizeof *local_addr));

    int file_fd = open(filePath, O_RDONLY, 0777);
    CHECK(file_fd);

    ssize_t bytesRead;
    char buffer[SIZE];
    CHECK(bytesRead = read(file_fd, buffer, sizeof(buffer)));
    
    while (bytesRead  > 0) {
        CHECK(sendto(sockfd, buffer, bytesRead, 0, (struct sockaddr *)local_addr, sizeof *local_addr));
        CHECK(bytesRead = read(file_fd, buffer, sizeof(buffer)));

    }

    CHECK(sendto(sockfd, "/FILE_END", strlen("/FILE_END"), 0, (struct sockaddr *)local_addr, sizeof *local_addr));
    close(file_fd);
}

// Function to receive a file
void receiveFile(int sockfd) {

    ssize_t bytesReceived;
    char buffer[SIZE];
    
    // Assume the file transfer starts with "/FILE" command
    printf("Receiving a file...\n");

    int new_file = open("received_file", O_WRONLY| O_CREAT | O_TRUNC, 0666);
    CHECK(new_file);

    CHECK((bytesReceived = recvfrom(sockfd, buffer, sizeof(buffer), 0, NULL, 0)));

    while ( bytesReceived  > 0) {

        if (strncmp(buffer, "/FILE_END", strlen("/FILE_END")) == 0) {
            printf("File received and saved as received_file.txt\n");
            break;
        }

        int ld = write(new_file, buffer, bytesReceived);
        CHECK(ld);
        CHECK((bytesReceived = recvfrom(sockfd, buffer, sizeof(buffer), 0, NULL, 0)));
    }

    CHECK(close(new_file));
}   

#endif

#define MAX_USERS 10 // Maximum number of users
#define SHM_NAME "/my_shared_memory"

#ifdef USR
struct user{
    int id;
    struct sockaddr_in6 addr;
};

struct Users {
    int nb_connected;
    struct user tab_users[];
};

// Broadcast function
void broadcastMessage(int sockfd, const char *message, struct Users *shared_users, int index) {
    for (int i = 0; i < shared_users->nb_connected + 1; i++) {
        // Pas s'envoyer à soi-même le message
        if (i != index){
            sendto(sockfd, message, strlen(message), 0, (struct sockaddr *)&shared_users->tab_users[i].addr, sizeof shared_users->tab_users[i].addr);
        }
    }
}
#endif

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

    // Mémoire partagée pour les clients connectés

    #ifdef USR
    int index = 0;
    int shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    CHECK(shm_fd);

    CHECK(ftruncate(shm_fd, sizeof(struct Users) + MAX_USERS * sizeof(struct user))); // Set the size of the shared memory segment

    struct Users *shared_users = mmap(NULL, sizeof(struct Users) + MAX_USERS * sizeof(struct user), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    #endif

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
            #ifdef USR
            // besoin d'envoyer a un seul client ! ici local
            shared_users->nb_connected++;
            index = shared_users->nb_connected;
            #endif
            CHECK(sendto(sockfd, "/HELO", strlen("/HELO"), 0, (struct sockaddr *)local_addr, sizeof *local_addr));
        }
    } else {
        // Case when no client is present

        // Initialisation du premier !
        #ifdef USR
        shared_users->nb_connected = 0;
        if (shared_users->nb_connected < MAX_USERS) {
                        shared_users->tab_users[shared_users->nb_connected].id = shared_users->nb_connected;
                        memcpy(&shared_users->tab_users[shared_users->nb_connected].addr, local_addr,
                               sizeof(struct sockaddr_in6));
                    }
        #endif

        char expectedMessage[] = "/HELO";
        struct sockaddr_storage sender_ss;
        struct sockaddr_in6 *sender_addr = (struct sockaddr_in6 *)&sender_ss;
        socklen_t sender_addr_len = sizeof(sender_ss);

        while (!connected) {
            // Wait for incoming message
            ssize_t bytes_received = recvfrom(sockfd, buffer, SIZE - 1, 0, (struct sockaddr *)sender_addr, &sender_addr_len);
            CHECK(bytes_received);

            if (bytes_received > 0) {
                buffer[bytes_received] = '\0';
                if (strncmp(buffer, expectedMessage, 5) == 0) {


                    char host[NI_MAXHOST];
                    char serv[NI_MAXSERV];
                    int err = getnameinfo((struct sockaddr *)sender_addr, sender_addr_len, host, NI_MAXHOST, serv, NI_MAXSERV, NI_NUMERICHOST | NI_NUMERICSERV);
                    if (err != 0) {
                        fprintf(stderr, "getnameinfo: %s\n", gai_strerror(err));
                        return EXIT_FAILURE;
                    }

                    printf("%s %s\n",host,serv);

                    #ifdef USR
                    // Add the user to the shared memory segment
                    if (shared_users->nb_connected < MAX_USERS) {
                        shared_users->tab_users[shared_users->nb_connected].id = shared_users->nb_connected;
                        memcpy(&shared_users->tab_users[shared_users->nb_connected].addr, sender_addr,
                               sizeof(struct sockaddr_in6));
                        //printf("nb_connected attente 1: %d\n", shared_users->nb_connected);
                    }
                    #else
                    memcpy(local_addr, sender_addr, sizeof(*sender_addr));
                    #endif
                    
                    connected = 1;
                } 
                else {
                    printf("Received message is not /HELO. Continuing to wait for the correct message...\n");
                    connected = 0;
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

    //char buffer2[SIZE];
    // Main loop
    while (1) {

        int result = poll(fds, 2, -1); 
        CHECK(result);

        // user input
        if (fds[0].revents & POLLIN) 
        {
            int r = read(0,buffer, SIZE);
            CHECK(r);
            buffer[r] = '\0';
            #ifdef FILEIO
            if ( strncmp(buffer,"/FILE",5) == 0){
                sendFile(sockfd, buffer,local_addr);
            }
            else{
                CHECK(sendto(sockfd, buffer, strlen(buffer), 0, (struct sockaddr *)local_addr, sizeof *local_addr));
            }
            #else
                #ifdef USR
                if (strncmp(buffer,"/HELO",5) != 0)
                    broadcastMessage(sockfd,buffer,shared_users, index);
                #else

                CHECK(sendto(sockfd, buffer, strlen(buffer), 0, (struct sockaddr *)local_addr, sizeof *local_addr));
                #endif
            #endif
            

            if (strncmp(buffer, "/QUIT", 5) == 0) 
                break; 

        }

        // socket 
        if (fds[1].revents & POLLIN) 
        {   
            //printf("Entrée écoute\n");

            struct sockaddr_storage sender_ss;
            struct sockaddr_in6 *sender_addr = (struct sockaddr_in6 *)&sender_ss;
            socklen_t sender_addr_len = sizeof(sender_ss);

            char host2[NI_MAXHOST];
            char serv2[NI_MAXSERV];

            ssize_t bytes_received = recvfrom(sockfd, buffer, SIZE-1, 0, (struct sockaddr *)sender_addr, &sender_addr_len);
            if (bytes_received > 0) {
                
                buffer[bytes_received] = '\0'; 
                if (strncmp(buffer, "/QUIT", 5) == 0) 
                {
                    break; 
                }
                #ifdef FILEIO
                else if (strncmp(buffer, "/FILE", 5) == 0) {
                    receiveFile(sockfd);
                }
                #endif
                else if (strncmp(buffer, "/HELO",5) == 0){
                
                    int err = getnameinfo((struct sockaddr *)sender_addr, sender_addr_len, host2, NI_MAXHOST, serv2, NI_MAXSERV, NI_NUMERICHOST | NI_NUMERICSERV);
                    if (err != 0) {
                        fprintf(stderr, "getnameinfo: %s\n", gai_strerror(err));
                        return EXIT_FAILURE;
                    }

                    #ifdef USR
                    // Add the user to the shared memory segment
                    if (shared_users->nb_connected < MAX_USERS) {
                        shared_users->tab_users[shared_users->nb_connected].id = shared_users->nb_connected;
                        memcpy(&shared_users->tab_users[shared_users->nb_connected].addr, sender_addr,
                               sizeof(struct sockaddr_in6));
                        printf("nb_connected socket : %d\n", shared_users->nb_connected);
                        
                    }
                    #endif
                }
                else {
                    printf("Utilsateur port %d : %s", htons(sender_addr->sin6_port), buffer);
                }
            }
        }
    }

    // Close the socket
    close(sockfd);

    #ifdef USR

    // Unmap 
    CHECK(munmap(shared_users, sizeof(struct Users) + MAX_USERS * sizeof(struct user)));
    CHECK(close(shm_fd));
    // Unlink 
    CHECK(shm_unlink(SHM_NAME));

    #endif

    return 0;
}