
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

// Fonction pour envoyer le message /FILEpath à un client
void sendFile(int sockfd, const char *command, struct sockaddr_in6 *local_addr) {

    // Extraction du chemin
    char filePath[SIZE];
    sscanf(command, "/FILE%s", filePath);

    // Informe le client de la commande Fichier
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
    // Informe le client de la fin du transfert des données
    CHECK(sendto(sockfd, "/FILE_END", strlen("/FILE_END"), 0, (struct sockaddr *)local_addr, sizeof *local_addr));
    close(file_fd);
}

// Fonction pour recevoir les données après une réception de type /FILE
void receiveFile(int sockfd) {

    ssize_t bytesReceived;
    char buffer[SIZE];
    
    printf("Receiving a file...\n");

    int new_file = open("received_file", O_WRONLY| O_CREAT | O_TRUNC, 0666);
    CHECK(new_file);

    CHECK((bytesReceived = recvfrom(sockfd, buffer, sizeof(buffer), 0, NULL, 0)));

    while ( bytesReceived  > 0) {

        // Condition d'arrêt
        if (strncmp(buffer, "/FILE_END", strlen("/FILE_END")) == 0) {
            printf("File received and saved as received_file \n");
            break;
        }

        int ld = write(new_file, buffer, bytesReceived);
        CHECK(ld);
        CHECK((bytesReceived = recvfrom(sockfd, buffer, sizeof(buffer), 0, NULL, 0)));
    }

    CHECK(close(new_file));
}   

#endif

#define MAX_USERS 10 // Nombre de clients maximum
#define SHM_NAME "/my_shared_memory" // Nom du segment de mémoire partagé

#ifdef USR
// Structure d'un utilisateur connecté
struct user{
    int id;
    struct sockaddr_in6 addr;
};
// Structure principale
struct Users {
    int nb_connected;
    struct user tab_users[];
};

// Fonction de Broadcast 
void broadcastMessage(int sockfd, const char *message, struct Users *shared_users, int index) {
    for (int i = 0; i < shared_users->nb_connected + 1; i++) {
        // On ne veut pas s'envoyer le message
        if (i != index){
            #ifdef FILEIO
            if ( strncmp(message,"/FILE",5) == 0)
                sendFile(sockfd,message, &shared_users->tab_users[i].addr); 
            else{
                sendto(sockfd, message, strlen(message), 0, (struct sockaddr *)&shared_users->tab_users[i].addr, sizeof shared_users->tab_users[i].addr);
            }
            #else
            sendto(sockfd, message, strlen(message), 0, (struct sockaddr *)&shared_users->tab_users[i].addr, sizeof shared_users->tab_users[i].addr);
            #endif
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

    // Création du segment
    int shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    CHECK(shm_fd);

    CHECK(ftruncate(shm_fd, sizeof(struct Users) + MAX_USERS * sizeof(struct user))); // On définit la taille du segment

    // On fait une projection du segment
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

    // On check si un client est déja connecté
    if (bind(sockfd, (struct sockaddr *)local_addr, sizeof *local_addr) == -1) {
        if (errno == EINVAL || errno == EADDRINUSE) {
            #ifdef USR
            // On informe 1 user et on incrémente 
            shared_users->nb_connected++;
            index = shared_users->nb_connected;
            #endif
            CHECK(sendto(sockfd, "/HELO", strlen("/HELO"), 0, (struct sockaddr *)local_addr, sizeof *local_addr));
        }
    } else {
        // Cas ou on est le premier connecté 
        
        #ifdef USR
        // Initialisation du premier client dans la mémoire partagée!
        shared_users->nb_connected = 0;
        if (shared_users->nb_connected < MAX_USERS) {
                        shared_users->tab_users[shared_users->nb_connected].id = shared_users->nb_connected; // index 0 dans le tableau users
                        memcpy(&shared_users->tab_users[shared_users->nb_connected].addr, local_addr,        // set de l'adresse
                               sizeof(struct sockaddr_in6));
                    }
        #endif

        char expectedMessage[] = "/HELO";
        struct sockaddr_storage sender_ss;
        struct sockaddr_in6 *sender_addr = (struct sockaddr_in6 *)&sender_ss;
        socklen_t sender_addr_len = sizeof(sender_ss);

        while (!connected) {
            // On attend un message de type /HELO
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
                    // Si on reçoit /HELO et qu'il y a de la place, on l'ajoute en tant qu'utilisateur connecté
                    if (shared_users->nb_connected < MAX_USERS) {
                        shared_users->tab_users[shared_users->nb_connected].id = shared_users->nb_connected;
                        memcpy(&shared_users->tab_users[shared_users->nb_connected].addr, sender_addr,
                               sizeof(struct sockaddr_in6));
                    }
                    #else
                    memcpy(local_addr, sender_addr, sizeof(*sender_addr));
                    #endif
                    
                    connected = 1; // sortie de la boucle
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

    // Main loop
    while (1) {

        int result = poll(fds, 2, -1); 
        CHECK(result);

        // Input de l'utilisateur 
        if (fds[0].revents & POLLIN) 
        {
            int r = read(0,buffer, SIZE);
            CHECK(r);
            buffer[r] = '\0';
            // Gestion de la compilation conditionnelle
            #ifdef FILEIO
                #ifdef USR      // Combinaison USR + FILEIO
                 if (strncmp(buffer,"/HELO",5) != 0 && strncmp(buffer, "/QUIT", 5) != 0)
                    broadcastMessage(sockfd,buffer, shared_users, index);       // On envoie à tout le monde
                #else           // Seulement FILEIO
                if ( strncmp(buffer,"/FILE",5) == 0){
                    sendFile(sockfd, buffer,local_addr);                        // On envoie au second utilisateur la commande /FILE
                }
                else{
                    // Cas DATA, on envoie juste les données tapées
                    CHECK(sendto(sockfd, buffer, strlen(buffer), 0, (struct sockaddr *)local_addr, sizeof *local_addr));
                }
                #endif  
            #else   // Seulement USR
                #ifdef USR
                if (strncmp(buffer,"/HELO",5) != 0 && strncmp(buffer, "/QUIT", 5) != 0) // DATA "normale"
                    broadcastMessage(sockfd,buffer,shared_users, index);           // On envoie à tout le monde le message
                #else
                // Cas aucune compilation conditionelle
                CHECK(sendto(sockfd, buffer, strlen(buffer), 0, (struct sockaddr *)local_addr, sizeof *local_addr));
                #endif
            #endif  
            
            // on sort de la boucle
            if (strncmp(buffer, "/QUIT", 5) == 0) 
                break; 

        }

        // Données dans le socket
        if (fds[1].revents & POLLIN) 
        {   

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
                    receiveFile(sockfd);    // Cas FILE, on reçoit et écrit les données
                }
                #endif
                else if (strncmp(buffer, "/HELO",5) == 0){ // Cas nouveau utilisateur détecté
                
                    int err = getnameinfo((struct sockaddr *)sender_addr, sender_addr_len, host2, NI_MAXHOST, serv2, NI_MAXSERV, NI_NUMERICHOST | NI_NUMERICSERV);
                    if (err != 0) {
                        fprintf(stderr, "getnameinfo: %s\n", gai_strerror(err));
                        return EXIT_FAILURE;
                    }

                    #ifdef USR
                    // On ajoute le nouvel utilisateur si il y a la place
                    if (shared_users->nb_connected < MAX_USERS) {
                        shared_users->tab_users[shared_users->nb_connected].id = shared_users->nb_connected;
                        memcpy(&shared_users->tab_users[shared_users->nb_connected].addr, sender_addr,
                               sizeof(struct sockaddr_in6));
                    }
                    #endif
                }
                else {
                    // Affichage des messages reçus et identification de la source avec le port
                    printf("Utilsateur port %d : %s", htons(sender_addr->sin6_port), buffer);
                }
            }
        }
    }

    // Close the socket / Memory
    close(sockfd);

    #ifdef USR

    // Unmap 
    CHECK(munmap(shared_users, sizeof(struct Users) + MAX_USERS * sizeof(struct user)));
    CHECK(close(shm_fd));

    #endif

    return 0;
}