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

    /* convert and check port number */

    /* create socket */

    /* complete struct sockaddr */

    /* link socket to local IP and PORT */

    /* wait for incoming message */

    /* close socket */

    /* free memory */

    /* print received message */

    return 0;
}
