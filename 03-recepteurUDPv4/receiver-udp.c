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

#define IP   "127.0.0.1"
#define SIZE 100

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
    int udp_socket = socket(AF_INET, SOCK_DGRAM, 0);
    CHECK(udp_socket);

    /* complete struct sockaddr */
    struct addrinfo hints;
    hints.ai_family = AF_INET;
    hints.ai_protocol = 0;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = 0;

    struct addrinfo *list;
    
    int inf = getaddrinfo(IP,argv[1],&hints,&list);
    if(inf != 0){
        fprintf(stderr, "getaddrinfo :%s\n", gai_strerror(inf));
        return EXIT_FAILURE;
    }

    if(list == NULL){
        fprintf(stderr, "pas de ports");
        return EXIT_FAILURE;
    }

    /* link socket to local IP and PORT */
    int bd = bind(udp_socket,list->ai_addr , (socklen_t)list->ai_addrlen);
    CHECK(bd);


    /* wait for incoming message */
    char *buf = malloc(sizeof(char)*SIZE);
    int rec = recvfrom(udp_socket,buf,SIZE,0,NULL,NULL);
    CHECK(rec);

    /* close socket */
    CHECK(close(udp_socket));

    /* free memory */
    freeaddrinfo(list);
    
    /* print received message */
    printf("%s", buf);
    free(buf);
    return 0;
}
