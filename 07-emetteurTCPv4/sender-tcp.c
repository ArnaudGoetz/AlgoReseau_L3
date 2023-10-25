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
    if(argc != 3){
        fprintf(stderr, "usage: %s ip_addr port_number\n", argv[0]);
        return 1;
    }
    /* convert and check port number */
     unsigned short port_number = atoi(argv[2]);
    if ( port_number < 10000 || port_number > 65000){
        fprintf(stderr,"usage: %s ip_addr port_number\n", argv[0]);
        return 1;
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
    
    int inf = getaddrinfo(argv[1],argv[2],&st,&list);
    if (inf != 0) {
        fprintf(stderr, "%s\n", gai_strerror(inf)); 
        return 1; 
    }

    if(list == NULL){
        // fprintf(stderr, "pas de ports");
        return EXIT_FAILURE;
    }

    /* connect to the remote peer */
    int con = connect(tcp_socket,list->ai_addr,list->ai_addrlen);
    CHECK(con);

    /* send the message */
    char buf[] = "hello world";
    ssize_t s = send(tcp_socket, buf, strlen(buf), 0);
    CHECK(s); 

    /* close socket */
    int clo = close(tcp_socket);
    CHECK(clo);

    /* free memory */
    freeaddrinfo(list);

    return 0;
}
