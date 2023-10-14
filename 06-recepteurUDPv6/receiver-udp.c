#include <sys/socket.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>

#define CHECK(op) do { if ( (op) == -1) { perror (#op); exit (EXIT_FAILURE); } \
                    } while (0)

#define IP   "::1"
#define SIZE 100

int main (int argc, char *argv[])
{
    /* test arg number */
    if (argc != 3) {
        fprintf(stderr, "usage: %s ip_addr port_number\n", argv[0]);
        return 1;
    }

    /* convert and check port number */
    unsigned short port_number = atoi(argv[2]);
    if (port_number < 10000 || port_number > 65000) {
        fprintf(stderr, "usage: %s ip_addr port_number\n", argv[0]);
        return 1;
    }

    /* create socket */
    int udp_socket = socket(AF_INET6, SOCK_DGRAM, 0);
    CHECK(udp_socket);

    /* complete struct sockaddr */
    struct addrinfo hints;
    hints.ai_family = AF_INET6;
    hints.ai_protocol = 0;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = 0;

    struct addrinfo *list;
    
    int inf = getaddrinfo(argv[1], argv[2], &hints, &list);
    if (inf != 0) {
        fprintf(stderr, "Name or service not known %s\n", gai_strerror(inf));
        return 1;
    }

    if (list == NULL) {
        fprintf(stderr, "pas de ports");
        return 1;
    }

    /* link socket to local IP and PORT */
    int bd = bind(udp_socket, list->ai_addr, list->ai_addrlen);
    CHECK(bd);

    /* wait for incoming message */
    char buf[SIZE];
    struct sockaddr_storage sender_addr;
    socklen_t sender_addr_len = sizeof(sender_addr);
    int rec = recvfrom(udp_socket, buf, SIZE, 0, (struct sockaddr *)&sender_addr, &sender_addr_len);
    CHECK(rec);
    buf[rec] = '\0'; // memory error fix

    printf("%s", buf);

    /* print sender addr and port */
    char host[SIZE];
    char serv[SIZE];
    int err = getnameinfo((struct sockaddr *)&sender_addr, sender_addr_len, host, SIZE, serv, SIZE, NI_NUMERICHOST | NI_NUMERICSERV);
    if (err != 0) {
        fprintf(stderr, "getnameinfo: %s\n", gai_strerror(err));
        return EXIT_FAILURE;
    }
   
    printf("%s %s\n", host, serv);

    /* close socket */
    CHECK(close(udp_socket));
    freeaddrinfo(list);
    return 0;
}
