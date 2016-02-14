/* Example: client.c sending and receiving datagrams using UDP */
#include <netdb.h>
#include <strings.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/time.h>
#include "globals.h"

typedef struct packet{
  uint8_t packet_type;
  uint32_t seq_num;
  struct timeval t;
}packet;
/* client program called with host name and port number of server */
main(int argc, char *argv[])
{
    struct packet p;
    int sock, buflen;
    //prepare a packet for
    struct timeval *tval = malloc(sizeof(struct timeval));
    tval -> tv_sec = 30;
    tval -> tv_usec = 0;

    memcpy(&p.t,&tval,sizeof(tval));
    p.seq_num = (uint32_t)1;
    p.packet_type = (uint8_t)6;

    int buffSize = sizeof(uint8_t) + sizeof(uint32_t) + sizeof(struct timeval);
    char *cli_buf = malloc(buffSize);
    bzero(cli_buf,buffSize);


    printf("p.seq is %d\n",p.seq_num);
    memcpy(&p.t,&tval,sizeof(tval));

    struct sockaddr_in name;
    struct hostent *hp, *gethostbyname();

    /* create socket for connecting to server */
    sock = socket(AF_INET, SOCK_DGRAM,0);
    if(sock < 0) {
	perror("opening datagram socket");
	exit(2);
    }

    /* construct name for connecting to server */
    name.sin_family = AF_INET;
    name.sin_port = htons(TIMERPORT);

    /* convert hostname to IP address and enter into name */
    hp = gethostbyname(clientIP);
    if(hp == 0) {
	fprintf(stderr, "%s:unknown host\n", hp);
	exit(3);
    }
    bcopy((char *)hp->h_addr, (char *)&name.sin_addr, hp->h_length);

    /* send message to server */

    int res = sendto(sock, (char *)&p, (sizeof(p.t) + 5), 0, (struct sockaddr *)&name, sizeof(name));
    printf("res is %d\n",res);
    if(res <0) {
	perror("sending datagram message");
	exit(4);
    }


    /* close connection */
    close(sock);
    exit(0);
}
