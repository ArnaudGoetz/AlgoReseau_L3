#include <sys/socket.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>

#define CHECK(op)   do { if ( (op) == -1) { perror (#op); exit (EXIT_FAILURE); } \
                    } while (0)

#define IP "127.0.0.1"

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
    int tcp_socket = socket(AF_INET, SOCK_STREAM, 0);
    CHECK(tcp_socket);

    /* complete struct sockaddr */
    struct addrinfo st;
    st.ai_family = AF_INET;
    st.ai_protocol = 0;
    st.ai_socktype = SOCK_STREAM;
    st.ai_flags = 0;

    struct addrinfo *list;
    
    int inf = getaddrinfo(IP,argv[1],&st,&list);
    CHECK(inf);

    if(list == NULL){
        fprintf(stderr, "pas de ports");
        return EXIT_FAILURE;
    }

    /* connect to the remote peer */
    int con = connect(tcp_socket,list->ai_addr,list->ai_addrlen);
    CHECK(con);

    /* send the message */
    char buf[] = "hello world";
    int send = sendto(tcp_socket, buf, strlen(buf), 0, list->ai_addr, list->ai_addrlen);
    CHECK(send);

    /* close socket */
    int clo = close(tcp_socket);
    CHECK(clo);

    /* free memory */
    freeaddrinfo(list);

    return 0;
}
