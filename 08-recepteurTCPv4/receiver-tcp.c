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
#define QUEUE_LENGTH 1

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

    /* SO_REUSEADDR option allows re-starting the program without delay */
    int iSetOption = 1;
    CHECK (setsockopt (tcp_socket, SOL_SOCKET, SO_REUSEADDR, &iSetOption,
		       sizeof iSetOption));

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
        return 1;
    }
    /* link socket to local IP and PORT */
    int bd = bind(tcp_socket, list->ai_addr, (socklen_t)list->ai_addrlen);
    CHECK(bd);

    /* set queue limit for incoming connections */

    int ls = listen(tcp_socket,1);
    CHECK(ls);

    /* wait for incoming TCP connection */

    struct sockaddr_storage sender_addr;
    socklen_t sender_addr_len = sizeof(sender_addr);

    int new_sock = accept(tcp_socket, (struct sockaddr *)&sender_addr, &sender_addr_len);
    CHECK(new_sock);
    
    char host[SIZE], serv[SIZE];
    
    int ins = getnameinfo((struct sockaddr *)&sender_addr, sender_addr_len, host, SIZE, serv, SIZE, NI_NUMERICHOST | NI_NUMERICSERV);
    if (ins != 0) {
        fprintf(stderr, "getnameinfo: %s\n", gai_strerror(ins));
        return 1; 
    }

    /* print sender addr and port */
    printf("%s %s \n", host, serv);

    /* wait for incoming message */
    char buf[SIZE];
    int rec = recv(new_sock, buf, SIZE, 0);
    CHECK(rec);
    buf[rec] = '\0'; // memory error fix

    /* Close socket */
    CHECK(close(tcp_socket));

    /* Free memory */
    freeaddrinfo(list);

    /* Print received message */
    printf("%s", buf); 

    return 0;
}
