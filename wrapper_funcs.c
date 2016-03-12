#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <unistd.h>

#include "wrapper_funcs.h"
#include "globals.h"

typedef struct Pkt_Info{
	int seq_num;
	int ack_flag;
	int pktStart;
	int sizeOfPkt;
}Pkt_Info;

#define MAX_BUFF 64000
int bytesInBuff;
Pkt_Info serWindow[20];
Pkt_Info cliWindow[20];
char cliBuffer[MAX_BUFF];
long fileSize;
int totalRecv;

/* NULL FUNCTION */
int CONNECT(int socket,const struct sockaddr *address,socklen_t address_len){
	return 0;
}

/* NULL FUNCTION */
int ACCEPT(int socket, struct sockaddr *address,socklen_t address_len){
	return 0;
}

int BIND(int socket,struct sockaddr *address, socklen_t address_len){
	return bind(socket,address,address_len);
}

/* This function returns a UDP socket */
int SOCKET(int domain,int type,int protocol){
	return socket(AF_INET,SOCK_DGRAM,0);
}
/*Sets up the SEND call properly */
ssize_t SEND(int sock,const void *buffer,size_t length,int flags){
	usleep(10000); /*Sleep for 10ms*/
	struct sockaddr_in sock_addr;
	sock_addr.sin_family = AF_INET;
	sock_addr.sin_port = htons(TCPDIN);
	sock_addr.sin_addr.s_addr = inet_addr(clientIP);
	char x[20]; //dont really care what gets received

	int sent = sendto(sock,buffer,length,flags,(struct sockaddr*)&sock_addr,sizeof(sock_addr));
	//set up connection to recv a packet from tcpd letting SEND() know it is ok to return
   	struct sockaddr_in wait;
	int wait_sock;
    	int addr_len = sizeof(wait);
	wait_sock = socket(AF_INET, SOCK_DGRAM,0);
	wait.sin_family = AF_INET;
	wait.sin_port = htons(CPTLSENDRECVPORT);
	wait.sin_addr.s_addr = 0;
	if(bind(wait_sock,(struct sockaddr*)&wait,sizeof(wait)) < 0){
		//perror("Failed to bind socket for wait comm SEND()\n");
	}
	//currently receiving byte but some bug 
   	//this will make SEND() blocking until it receives packet from TCPD
	//printf("waiting for bytes in SEND()\n");
    	//int wait_bytes = recvfrom(wait_sock,x,1,0,(struct sockaddr*)&wait,&addr_len);
    
	return sent;
	
}
/*Sets up RECV call properly */
ssize_t RECV(int socket,const void *buffer,size_t length,int flags){
	socklen_t leng;
	struct sockaddr_in sock_addr;
	sock_addr.sin_family = AF_INET;
	sock_addr.sin_port = htons(SERVERPORT);
	sock_addr.sin_addr.s_addr = 0;
	leng = sizeof(sock_addr);
	return recvfrom(socket,(char *)buffer,length,flags,(struct sockaddr*)&sock_addr,&leng);
}


int CLOSE(int socket){
	int windowsEmpty = 0;
	int i;
	for(i = 0; i < 19; i++){
		if(serWindow[i].ack_flag == 1 && cliWindow[i].ack_flag == 1){
			windowsEmpty = 1;
		}else{
			windowsEmpty = 0;
			break;
		}
	}
	if(fileSize == totalRecv && windowsEmpty == 1){
		printf("Closing the connection!\n");
		return close(socket);
	}
}
