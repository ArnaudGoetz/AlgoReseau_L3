#include <sys/socket.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <netinet/in.h>
#include <unistd.h>
#include <netdb.h>

#define CHECK(op)   do { if ( (op) == -1) { perror (#op); exit (EXIT_FAILURE); } \
                    } while (0)

#define IP "::1"
#define PORT(p) htons(p)

int main (int argc, char *argv [])
{
    /* test arg number */
    if(argc != 2){
        fprintf(stderr, "Usage : %s arg number", argv[0]);
        return EXIT_FAILURE;
    }

    /* convert and check port number */
    unsigned short port_number = atoi(argv[1]);
    if ( port_number < 10000 || port_number > 65000){
        fprintf(stderr,"Usage : %s port number", argv[0]);
        return EXIT_FAILURE;
    }

    /* create socket */
    int udp_socket = socket(AF_INET6, SOCK_DGRAM, 0);
    CHECK(udp_socket);

     /* IP and port*/
    struct addrinfo st;
    st.ai_family = AF_INET6;
    st.ai_protocol = 0;
    st.ai_socktype = SOCK_DGRAM;
    st.ai_flags = 0;

    struct addrinfo *list;
    
    int inf = getaddrinfo(IP,argv[1],&st,&list);
    CHECK(inf);

    if(list == NULL){
        fprintf(stderr, "pas de ports");
        return EXIT_FAILURE;
    }

    /* send message to remote peer */
    char buf[] = "hello world";
    CHECK(sendto(udp_socket, buf, 11, 0, list->ai_addr, list->ai_addrlen));

    /* close socket */
    CHECK(close(udp_socket));

    /* free memory */
    freeaddrinfo(list);
    return 0;
}
