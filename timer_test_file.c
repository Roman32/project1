/* Example: client.c sending and receiving datagrams using UDP */
#include <netdb.h>
#include <strings.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/time.h>
#include "globals.h"
#include <unistd.h>
#include "wrapper_funcs.h"
#include <time.h>
/*
typedef struct packet{
  uint8_t packet_type;
  uint32_t seq_num;
  uint64_t tv_sec;
  uint64_t tv_usec;
  //struct timeval t; doesnt seem to be working so just sending times
}packet;*/
/* client program called with host name and port number of server */

void send_packet(uint8_t packet_type,
uint32_t seq_num,
uint64_t tv_sec,
uint64_t tv_usec,
int sock,
struct sockaddr_in name){

	int buffSize = sizeof(uint8_t) + sizeof(uint32_t) + sizeof(uint64_t) + sizeof(uint64_t);
	printf("buffsize is %d\n",buffSize);

	char *cli_buf = malloc(buffSize);
    bzero(cli_buf,buffSize);

    seq_num = htonl(seq_num);
    tv_sec = htobe64(tv_sec);
    tv_usec = htobe64(tv_usec);

	memcpy(cli_buf,&packet_type,1);
	memcpy(cli_buf+1,&seq_num,4);
	memcpy(cli_buf+5,&tv_sec,8);
	memcpy(cli_buf+13,&tv_usec,8);

	int res = sendto(sock, cli_buf,buffSize, 0, (struct sockaddr *)&name, sizeof(name));
    printf("res is %d\n",res);
    if(res <0) {
	perror("sending datagram message");
	exit(4);
    }
}
main(int argc, char *argv[])
{
   /* struct packet *p;
    p = malloc(21);*/
    int sock, buflen;
    /*prepare a packet for
    struct timeval *tval = malloc(sizeof(struct timeval));
    tval -> tv_sec = 30;
    tval -> tv_usec = 0;*/
    /*
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

	*/



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
    /*
    int res = sendto(sock, cli_buf,buffSize, 0, (struct sockaddr *)&name, sizeof(name));
    printf("res is %d\n",res);
    if(res <0) {
	perror("sending datagram message");
	exit(4);
    }*/

    send_packet(6, 1, 30, 0,sock,name);
    //pckt_type, seq_num, tv_sec, tv_usec
    send_packet(6, 2, 35, 0,sock,name);

	//pckt_type, seq_num, tv_sec, tv_usec
    send_packet(6, 3, 31, 0,sock,name);

	//pckt_type, seq_num, tv_sec, tv_usec
    send_packet(6, 4, 29, 0,sock,name);

	//pckt_type, seq_num, tv_sec, tv_usec
    send_packet(6, 5, 28, 0,sock,name);

	//pckt_type, seq_num, tv_sec, tv_usec
    send_packet(6, 6, 28, 50000,sock,name);

	//pckt_type, seq_num, tv_sec, tv_usec
    send_packet(6, 7, 26, 0,sock,name);

	//pckt_type, seq_num, tv_sec, tv_usec
    send_packet(6, 8, 41, 0,sock,name);

	//pckt_type, seq_num, tv_sec, tv_usec
    send_packet(6, 9, 44, 0,sock,name);

	sleep(1);
	send_packet(7,7,12,12,sock,name);
	//pckt_type, seq_num, tv_sec, tv_usec
    send_packet(6, 10, 8, 0,sock,name);
	sleep(5);
send_packet(6, 11, 1, 50000,sock,name);
send_packet(6, 12, 0, 90000,sock,name);
send_packet(6, 13, 3, 25000,sock,name);
send_packet(6, 14, 0, 10000,sock,name);

    /*
    bzero(cli_buf,buffSize);
    packet_type = 6;
     seq_num = htonl(2);
    tv_sec = htobe64(35);
    tv_usec = htobe64(0);

    memcpy(cli_buf,&packet_type,1);
    memcpy(cli_buf+1,&seq_num,4);
    memcpy(cli_buf+5,&tv_sec,8);
    memcpy(cli_buf+13,&tv_usec,8);

     res = sendto(sock, cli_buf,buffSize, 0, (struct sockaddr *)&name, sizeof(name));
    printf("res is %d\n",res);
    if(res <0) {
    perror("sending datagram message");
    exit(4);
    }


    bzero(cli_buf,buffSize);
    packet_type = 6;
     seq_num = htonl(3);
    tv_sec = htobe64(34);
    tv_usec = htobe64(0);

    memcpy(cli_buf,&packet_type,1);
    memcpy(cli_buf+1,&seq_num,4);
    memcpy(cli_buf+5,&tv_sec,8);
    memcpy(cli_buf+13,&tv_usec,8);

     res = sendto(sock, cli_buf,buffSize, 0, (struct sockaddr *)&name, sizeof(name));
    printf("res is %d\n",res);
    if(res <0) {
    perror("sending datagram message");
    exit(4);
    }


    bzero(cli_buf,buffSize);
    packet_type = 6;
     seq_num = htonl(4);
    tv_sec = htobe64(33);
    tv_usec = htobe64(0);

    memcpy(cli_buf,&packet_type,1);
    memcpy(cli_buf+1,&seq_num,4);
    memcpy(cli_buf+5,&tv_sec,8);
    memcpy(cli_buf+13,&tv_usec,8);

     res = sendto(sock, cli_buf,buffSize, 0, (struct sockaddr *)&name, sizeof(name));
    printf("res is %d\n",res);
    if(res <0) {
    perror("sending datagram message");
    exit(4);
    }



    bzero(cli_buf,buffSize);
    packet_type = 6;
     seq_num = htonl(5);
    tv_sec = htobe64(38); //doesnt apply for cancel packets
    tv_usec = htobe64(0);

    memcpy(cli_buf,&packet_type,1);
    memcpy(cli_buf+1,&seq_num,4);
    memcpy(cli_buf+5,&tv_sec,8);
    memcpy(cli_buf+13,&tv_usec,8);

     res = sendto(sock, cli_buf,buffSize, 0, (struct sockaddr *)&name, sizeof(name));
    printf("res is %d\n",res);
    if(res <0) {
    perror("sending datagram message");
    exit(4);
    }
		// duplicate test
	bzero(cli_buf,buffSize);
    packet_type = 6;
     seq_num = htonl(5);
    tv_sec = htobe64(38); //doesnt apply for cancel packets
    tv_usec = htobe64(0);

    memcpy(cli_buf,&packet_type,1);
    memcpy(cli_buf+1,&seq_num,4);
    memcpy(cli_buf+5,&tv_sec,8);
    memcpy(cli_buf+13,&tv_usec,8);

     res = sendto(sock, cli_buf,buffSize, 0, (struct sockaddr *)&name, sizeof(name));
    printf("res is %d\n",res);
    if(res <0) {
    perror("sending datagram message");
    exit(4);
    }

    sleep(1);

    //Cancel test
    bzero(cli_buf,buffSize);
    packet_type = 7;
     seq_num = htonl(4);
    tv_sec = htobe64(38);//doesnt apply for cancel packets
    tv_usec = htobe64(0);//doesnt apply for cancel packets

    memcpy(cli_buf,&packet_type,1);
    memcpy(cli_buf+1,&seq_num,4);
    memcpy(cli_buf+5,&tv_sec,8);
    memcpy(cli_buf+13,&tv_usec,8);

     res = sendto(sock, cli_buf,buffSize, 0, (struct sockaddr *)&name, sizeof(name));
    printf("res is %d\n",res);
    if(res <0) {
    perror("sending datagram message");
    exit(4);
    }
     sleep(1);

    bzero(cli_buf,buffSize);
    packet_type = 6;
     seq_num = htonl(10);
    tv_sec = htobe64(40); //doesnt apply for cancel packets
    tv_usec = htobe64(0);

    memcpy(cli_buf,&packet_type,1);
    memcpy(cli_buf+1,&seq_num,4);
    memcpy(cli_buf+5,&tv_sec,8);
    memcpy(cli_buf+13,&tv_usec,8);

     res = sendto(sock, cli_buf,buffSize, 0, (struct sockaddr *)&name, sizeof(name));
    printf("res is %d\n",res);
    if(res <0) {
    perror("sending datagram message");
    exit(4);
    }

    bzero(cli_buf,buffSize);
    packet_type = 6;
     seq_num = htonl(11);
    tv_sec = htobe64(8); //doesnt apply for cancel packets
    tv_usec = htobe64(0);

    memcpy(cli_buf,&packet_type,1);
    memcpy(cli_buf+1,&seq_num,4);
    memcpy(cli_buf+5,&tv_sec,8);
    memcpy(cli_buf+13,&tv_usec,8);

     res = sendto(sock, cli_buf,buffSize, 0, (struct sockaddr *)&name, sizeof(name));
    printf("res is %d\n",res);
    if(res <0) {
    perror("sending datagram message");
    exit(4);
    }

    bzero(cli_buf,buffSize);
    packet_type = 6;
     seq_num = htonl(12);
    tv_sec = htobe64(39); //doesnt apply for cancel packets
    tv_usec = htobe64(0);

    memcpy(cli_buf,&packet_type,1);
    memcpy(cli_buf+1,&seq_num,4);
    memcpy(cli_buf+5,&tv_sec,8);
    memcpy(cli_buf+13,&tv_usec,8);

     res = sendto(sock, cli_buf,buffSize, 0, (struct sockaddr *)&name, sizeof(name));
    printf("res is %d\n",res);
    if(res <0) {
    perror("sending datagram message");
    exit(4);
    }

	bzero(cli_buf,buffSize);
    packet_type = 6;
     seq_num = htonl(13);
    tv_sec = htobe64(6); //doesnt apply for cancel packets
    tv_usec = htobe64(0);

    memcpy(cli_buf,&packet_type,1);
    memcpy(cli_buf+1,&seq_num,4);
    memcpy(cli_buf+5,&tv_sec,8);
    memcpy(cli_buf+13,&tv_usec,8);

     res = sendto(sock, cli_buf,buffSize, 0, (struct sockaddr *)&name, sizeof(name));
    printf("res is %d\n",res);
    if(res <0) {
    perror("sending datagram message");
    exit(4);
    }
    sleep(5);
	 // Cancel test
    bzero(cli_buf,buffSize);
    packet_type = 7;
     seq_num = htonl(13);
    tv_sec = htobe64(38);//doesnt apply for cancel packets
    tv_usec = htobe64(0);//doesnt apply for cancel packets

    memcpy(cli_buf,&packet_type,1);
    memcpy(cli_buf+1,&seq_num,4);
    memcpy(cli_buf+5,&tv_sec,8);
    memcpy(cli_buf+13,&tv_usec,8);

     res = sendto(sock, cli_buf,buffSize, 0, (struct sockaddr *)&name, sizeof(name));
    printf("res is %d\n",res);
    if(res <0) {
    perror("sending datagram message");
    exit(4);
    }
    sleep(2);
   // Cancel test
    bzero(cli_buf,buffSize);
    packet_type = 7;
     seq_num = htonl(10);
    tv_sec = htobe64(38);//doesnt apply for cancel packets
    tv_usec = htobe64(0);//doesnt apply for cancel packets

    memcpy(cli_buf,&packet_type,1);
    memcpy(cli_buf+1,&seq_num,4);
    memcpy(cli_buf+5,&tv_sec,8);
    memcpy(cli_buf+13,&tv_usec,8);

     res = sendto(sock, cli_buf,buffSize, 0, (struct sockaddr *)&name, sizeof(name));
    printf("res is %d\n",res);
    if(res <0) {
    perror("sending datagram message");
    exit(4);
    }
	// zero second timeout test
    sleep(40);
    bzero(cli_buf,buffSize);
    packet_type = 6;
     seq_num = htonl(14);
    tv_sec = htobe64(0); //doesnt apply for cancel packets
    tv_usec = htobe64(0);

    memcpy(cli_buf,&packet_type,1);
    memcpy(cli_buf+1,&seq_num,4);
    memcpy(cli_buf+5,&tv_sec,8);
    memcpy(cli_buf+13,&tv_usec,8);

     res = sendto(sock, cli_buf,buffSize, 0, (struct sockaddr *)&name, sizeof(name));
    printf("res is %d\n",res);
    if(res <0) {
    perror("sending datagram message");
    exit(4);
    }
    bzero(cli_buf,buffSize);
    packet_type = 6;
     seq_num = htonl(15);
    tv_sec = htobe64(0); //doesnt apply for cancel packets
    tv_usec = htobe64(50000);

    memcpy(cli_buf,&packet_type,1);
    memcpy(cli_buf+1,&seq_num,4);
    memcpy(cli_buf+5,&tv_sec,8);
    memcpy(cli_buf+13,&tv_usec,8);

     res = sendto(sock, cli_buf,buffSize, 0, (struct sockaddr *)&name, sizeof(name));
    printf("res is %d\n",res);
    if(res <0) {
    perror("sending datagram message");
    exit(4);
    }

    bzero(cli_buf,buffSize);
    packet_type = 6;
     seq_num = htonl(16);
    tv_sec = htobe64(0); //doesnt apply for cancel packets
    tv_usec = htobe64(394302);

    memcpy(cli_buf,&packet_type,1);
    memcpy(cli_buf+1,&seq_num,4);
    memcpy(cli_buf+5,&tv_sec,8);
    memcpy(cli_buf+13,&tv_usec,8);

     res = sendto(sock, cli_buf,buffSize, 0, (struct sockaddr *)&name, sizeof(name));
    printf("res is %d\n",res);
    if(res <0) {
    perror("sending datagram message");
    exit(4);
    }
    /* close connection */
    close(sock);
    exit(0);
}
