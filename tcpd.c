#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/ip.h>
#include <errno.h>
#include <linux/tcp.h>


#include "globals.h"

struct sockaddr_in client;
struct sockaddr_in server;
struct sockaddr_in troll;
struct sockaddr_in final;

struct tcphdr tcpHead;

typedef struct TrollHeader{
	struct sockaddr_in header;
	char body[1000];
}TrollHeader;

int main(int argc, char argv[]){
	
	int sockIn;
	int sockOut;
	int bytes =0;
	int bytesIn =0;
	int bytesToServ = 0;
	int bytesToTroll = 0;
	char buffer[1000];
	char bufferOut[1016];
	char buffS[1000];
	char trollBuf[1001];
	fd_set portUp;
	TrollHeader head;

	if(argc != 1){
		printf("Error! Proper use is ./tcpd");
		exit(1);
	}
	
	/*Set up for Client Socket*/
	sockIn = socket(AF_INET,SOCK_DGRAM,0);
	client.sin_family = AF_INET;
	client.sin_port = htons(TCPDIN);
	client.sin_addr.s_addr = inet_addr("0");
	if(bind(sockIn,(struct sockaddr*)&client,sizeof(client)) < 0){
		perror("Failed to bind socket for client comm.\n");
	}
	
		
	/*Set up for sending to Server*/
	sockOut = socket(AF_INET,SOCK_DGRAM,0);
	server.sin_family = AF_INET;
	server.sin_port = htons(TCPDOUT);
	server.sin_addr.s_addr = 0;
	if(bind(sockOut,(struct sockaddr*)&server,sizeof(server)) < 0){
		perror("Failed to bind socket for server comm.\n");
	}
	
	head.header.sin_family = htons(AF_INET);
	head.header.sin_addr.s_addr = inet_addr(serverIP);
	head.header.sin_port = htons(TCPDOUT);
	
	final.sin_family = AF_INET;
	final.sin_port = htons(SERVERPORT);
	final.sin_addr.s_addr = inet_addr(serverIP);
	
	troll.sin_family = AF_INET;
	troll.sin_addr.s_addr = inet_addr(clientIP);
	troll.sin_port = htons(TROLLPORT);
	
	FD_ZERO(&portUp);
	FD_SET(sockIn, &portUp);
	FD_SET(sockOut, &portUp);
	
	while(1){
		if(select(FD_SETSIZE,&portUp,NULL,NULL,NULL)){
			//perror("select");
		}
		if(FD_ISSET(sockIn,&portUp)){
			bzero(&buffer,sizeof(buffer));
			int addr_len =sizeof(client);
			bytesIn = recvfrom(sockIn,buffer,sizeof(buffer),0,(struct sockaddr *)&client,&addr_len);
			printf("Bytes from client :%d\n",bytesIn);
			//bzero(&head.body,sizeof(head.body));
			memcpy(&head.body,&buffer,1000);
			bytesToTroll = sendto(sockIn,(char *)&head,(sizeof(head.header)+bytesIn),0,(struct sockaddr*)&troll,sizeof(troll));
			printf("Sent to the Troll: %d\n",bytesToTroll);
			
		}
		if(FD_ISSET(sockOut,&portUp)){
			int addr_len = sizeof(server);
			bytes =recvfrom(sockOut,bufferOut,sizeof(bufferOut),0,(struct sockaddr*)&server,&addr_len);
			printf("Bytes recv from troll %d\n",bytes);
			bytesToServ = sendto(sockOut,bufferOut+16,bytes-16,0,(struct sockaddr*)&final,sizeof(final));
			printf("Bytes sent to server:%d\n",bytesToServ);
		}
		FD_ZERO(&portUp);
		FD_SET(sockIn, &portUp);
		FD_SET(sockOut, &portUp);		
	}	
	
	return 0;
}

