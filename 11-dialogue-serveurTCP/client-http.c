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

#define SIZE 100

void cpy (int src, int dst)
{
    
    char buffer[SIZE];
    int r = read(src, buffer, SIZE);
    CHECK(r);

    while(r > 0){
        CHECK(write(dst,buffer,r));
        CHECK(r = read(src, buffer, SIZE));
    }

    return;
}

int main (int argc, char * argv[])
{
    /* test arg number */
    if(argc != 2){
        fprintf(stderr, "usage: %s server_name\n", argv[0]);
        return 1;
    }
    /* get the list of struct addrinfo */
    struct addrinfo st;
    st.ai_family = AF_UNSPEC;
    st.ai_protocol = 0;
    st.ai_socktype = SOCK_STREAM;
    st.ai_flags = 0;

    struct addrinfo *list;

    int inf = getaddrinfo(argv[1],"80",&st,&list);
    if (inf != 0) {
        fprintf(stderr, "%s\n", gai_strerror(inf)); 
        return 1; 
    }
    if(list == NULL){
        // fprintf(stderr, "pas de ports");
        return 1;
    }

    /* create socket */
    int sockfd = socket(list->ai_family, list->ai_socktype, 0);
    CHECK(sockfd);

    /* connect to the server */
    CHECK(connect(sockfd, list->ai_addr, list->ai_addrlen));

    /* prepare GET cmd */
    const char *http_request = "GET / HTTP/1.1\r\nHost: %s\r\nConnection: close\r\n\r\n";
    char request[SIZE];
    snprintf(request, sizeof(request), http_request, argv[1]);

    /* send GET cmd and get the response */

    CHECK(send(sockfd, request, strlen(request),0));

    char buffer[SIZE];
    ssize_t lec;

    while ((lec = read(sockfd, buffer, sizeof(buffer))) > 0) {
        write(1, buffer, lec);
    }

    /* close socket */
    CHECK(close(sockfd));

    /* free memory */
    freeaddrinfo(list);

    return 0;
}
