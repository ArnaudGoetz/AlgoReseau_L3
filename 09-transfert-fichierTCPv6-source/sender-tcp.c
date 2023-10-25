#include <sys/socket.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <netdb.h>

#define CHECK(op)   do { if ( (op) == -1) { perror (#op); exit (EXIT_FAILURE); } \
                    } while (0)

#define IP   "::1"
#define SIZE 100

void cpy (int src, int dst)
{   
    char buffer[SIZE];
    int r = read(src, buffer, SIZE);
    CHECK(r);

    while(r > 0 ){
        CHECK(write(dst,buffer,r));
        CHECK(r = read(src, buffer, SIZE));
    }

    return;
}

int main (int argc, char *argv [])
{
    /* test arg number */
    if(argc != 4){
        fprintf(stderr, "usage: %s ip_addr port_number filename\n", argv[0]);
        return 1;
    }
    /* convert and check port number */
     unsigned short port_number = atoi(argv[2]);
    if ( port_number < 10000 || port_number > 65000){
        fprintf(stderr,"usage: %s ip_addr port_number\n", argv[0]);
        return 1;
    }

    /* open file to send */
    int fd = open(argv[3],O_RDONLY);
    CHECK(fd);

    /* create socket */
    int tcp_socket = socket(AF_INET6, SOCK_STREAM, 0);
    CHECK(tcp_socket);

    /* complete struct sockaddr */
    struct addrinfo st;
    st.ai_family = AF_INET6;
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

    /* send the file content */

    cpy(fd,tcp_socket);

    /* close socket*/
    int clo = close(tcp_socket);
    CHECK(clo);

    /* free memory */
    freeaddrinfo(list);

    /* close file */
    CHECK(close(fd));

    return 0;
}
