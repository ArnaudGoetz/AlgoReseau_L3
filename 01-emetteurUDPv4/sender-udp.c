#include <sys/socket.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <netinet/in.h>
#include <unistd.h>

#define CHECK(op)   do { if ( (op) == -1) { perror (#op); exit (EXIT_FAILURE); } \
                    } while (0)

#define IP      0x100007f /* 127.0.0.1 */
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

    int udp_socket = socket(AF_INET, SOCK_DGRAM, 0);
    CHECK(udp_socket);

    /* complete sockaddr struct */
    struct sockaddr_storage ss;
    struct sockaddr_in *in = (struct sockaddr_in *) &ss;
    in->sin_addr.s_addr = IP;
    in->sin_family = AF_INET;
    in->sin_port = PORT(port_number);

    /* send message to remote peer */
    char buf[] = "hello world";
    CHECK(sendto(udp_socket,buf, sizeof(buf), 0, (const struct sockaddr *) in, sizeof((*in))));

    /* close socket */
    CHECK(close(udp_socket));

    return 0;
}
