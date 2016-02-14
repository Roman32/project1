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
/*
typedef struct packet{
  uint8_t packet_type;
  uint32_t seq_num;
  uint64_t tv_sec;
  uint64_t tv_usec;
  //struct timeval t; doesnt seem to be working so just sending times
}packet;*/
/* client program called with host name and port number of server */
main(int argc, char *argv[])
{
   /* struct packet *p;
    p = malloc(21);*/
    int sock, buflen;
    /*prepare a packet for
    struct timeval *tval = malloc(sizeof(struct timeval));
    tval -> tv_sec = 30;
    tval -> tv_usec = 0;*/

    int buffSize = sizeof(uint8_t) + sizeof(uint32_t) + sizeof(uint64_t) + sizeof(uint64_t);
	printf("buffsize is %d\n",buffSize);

    char *cli_buf = malloc(buffSize);
    bzero(cli_buf,buffSize);

	uint8_t packet_type = 6;
    uint32_t seq_num = htonl(1);
    uint64_t tv_sec = htobe64(30);
    uint64_t tv_usec = htobe64(0);

	memcpy(cli_buf,&packet_type,1);
	memcpy(cli_buf+1,&seq_num,4);
	memcpy(cli_buf+5,&tv_sec,8);
	memcpy(cli_buf+13,&tv_usec,8);





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

    int res = sendto(sock, cli_buf,buffSize, 0, (struct sockaddr *)&name, sizeof(name));
    printf("res is %d\n",res);
    if(res <0) {
	perror("sending datagram message");
	exit(4);
    }


    /* close connection */
    close(sock);
    exit(0);
}
