#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <unistd.h>

#include "wrapper_funcs.h"
#include "globals.h"



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
ssize_t SEND(int socket,const void *buffer,size_t length,int flags){
	usleep(10000); /*Sleep for 10ms*/
	struct sockaddr_in sock_addr;
	sock_addr.sin_family = AF_INET;
	sock_addr.sin_port = htons(TCPDIN);
	sock_addr.sin_addr.s_addr = inet_addr(clientIP);
	return sendto(socket,buffer,length,flags,(struct sockaddr*)&sock_addr,sizeof(sock_addr));
	
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
